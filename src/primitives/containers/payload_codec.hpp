/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2026.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.
*/

// base64 and zlib decoding for Qiskit Runtime primitive payloads

#ifndef __qiskitcpp_primitives_payload_codec_hpp__
#define __qiskitcpp_primitives_payload_codec_hpp__

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <zlib.h>

namespace Qiskit {
namespace primitives {

/// @class PayloadCodec
/// @brief Decode the base64 + zlib envelope of a Qiskit Runtime payload.
/// @details Both a QPY circuit payload and an ndarray parameter payload are
///   carried as the base64 encoding of a zlib stream, so the two decoding steps
///   live here and are shared by ``QpyCircuitPayload`` and ``ParameterValues``.
/// @note Only the decoding direction is shared. The encoding direction is
///   duplicated in those two classes, which predates this header; the
///   duplication is left alone rather than refactored alongside an unrelated
///   change.
class PayloadCodec {
public:
    /// @brief The largest buffer ``decompress`` will produce.
    /// @return the cap in bytes
    /// @details A zlib stream declares no output size, so a payload is expanded
    ///   until it ends. The cap stops a corrupt or hostile payload from being
    ///   expanded without bound; it is far above any real circuit or parameter
    ///   array.
    static std::size_t max_decompressed_size(void)
    {
        return (std::size_t)512 * 1024 * 1024;
    }

    /// @brief Decode a standard base64 string.
    /// @param text the base64 text, optionally padded with '='
    /// @return the decoded bytes
    /// @throws std::invalid_argument if the text is not valid base64
    /// @details Reads the standard alphabet of RFC 4648 section 4, the inverse
    ///   of ``base64_encode``. Whitespace is skipped, so text that was wrapped
    ///   across lines still decodes.
    static std::vector<std::uint8_t> base64_decode(const std::string &text)
    {
        std::vector<std::uint8_t> decoded;
        decoded.reserve((text.size() / 4) * 3);

        // Holds up to 4 six-bit groups before they are turned into 3 bytes.
        std::uint32_t accumulator = 0;
        std::size_t collected = 0;
        std::size_t padding = 0;

        for (std::size_t i = 0; i < text.size(); i++) {
            const unsigned char ch = (unsigned char)text[i];
            if (std::isspace(ch)) {
                continue;
            }

            if (ch == '=') {
                // Padding only belongs at the end, and at most two characters
                // of it. Anything after it would change what the earlier
                // characters decode to.
                padding++;
                if (padding > 2) {
                    throw std::invalid_argument(
                        "Payload : the base64 data has more than two padding characters.");
                }
                collected++;
                accumulator <<= 6;
                if (collected == 4) {
                    emit_group(decoded, accumulator, 4 - padding);
                    accumulator = 0;
                    collected = 0;
                }
                continue;
            }

            if (padding > 0) {
                throw std::invalid_argument(
                    "Payload : the base64 data has a character after its padding.");
            }

            const int value = base64_value(ch);
            if (value < 0) {
                std::stringstream message;
                message << "Payload : '" << text[i]
                        << "' is not a base64 character.";
                throw std::invalid_argument(message.str());
            }

            accumulator = (accumulator << 6) | (std::uint32_t)value;
            collected++;
            if (collected == 4) {
                emit_group(decoded, accumulator, 4);
                accumulator = 0;
                collected = 0;
            }
        }

        if (collected != 0) {
            // A base64 stream is a whole number of 4-character groups once the
            // padding is counted. One leftover character cannot encode even a
            // single byte, so the text is truncated either way.
            throw std::invalid_argument(
                "Payload : the base64 data is truncated; its length is not a multiple of 4.");
        }
        return decoded;
    }

    /// @brief Decompress a zlib stream.
    /// @param data the compressed bytes
    /// @return the decompressed bytes
    /// @throws std::invalid_argument if the data is not a valid zlib stream, is
    ///   truncated, or expands beyond ``max_decompressed_size()``
    /// @throws std::runtime_error if zlib cannot be initialized
    /// @details Reads a zlib stream (RFC 1950), which is what ``compress``
    ///   writes and what Python's ``zlib.compress`` produces. Streamed through
    ///   ``inflate`` rather than ``uncompress`` because the decompressed size
    ///   is not known in advance.
    static std::vector<std::uint8_t> decompress(const std::vector<std::uint8_t> &data)
    {
        if (data.empty()) {
            throw std::invalid_argument(
                "Payload : the compressed data is empty, which is not a zlib stream.");
        }

        z_stream stream;
        std::memset(&stream, 0, sizeof(stream));
        if (inflateInit(&stream) != Z_OK) {
            throw std::runtime_error("Payload : could not initialize zlib.");
        }

        // next_in is not const in zlib's interface even though inflate only
        // reads through it.
        stream.next_in = (Bytef *)data.data();
        stream.avail_in = (uInt)data.size();

        std::vector<std::uint8_t> decompressed;
        std::vector<std::uint8_t> buffer(65536);
        int ret = Z_OK;

        do {
            stream.next_out = buffer.data();
            stream.avail_out = (uInt)buffer.size();
            ret = inflate(&stream, Z_NO_FLUSH);

            if (ret != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR) {
                inflateEnd(&stream);
                std::stringstream message;
                message << "Payload : zlib decompression failed (" << ret << ")";
                throw std::invalid_argument(message.str());
            }

            const std::size_t produced = buffer.size() - stream.avail_out;
            if (decompressed.size() + produced > max_decompressed_size()) {
                inflateEnd(&stream);
                throw std::invalid_argument(
                    "Payload : the compressed data expands beyond the size limit.");
            }
            decompressed.insert(decompressed.end(), buffer.begin(),
                                buffer.begin() + produced);

            if (ret == Z_BUF_ERROR && produced == 0) {
                // Z_BUF_ERROR with nothing produced means inflate can make no
                // further progress, so the stream ended before it was complete.
                // This is what terminates the loop on a truncated payload.
                //
                // A plain Z_OK with nothing produced is not treated as an
                // error: zlib may consume input without emitting any, and it
                // guarantees progress on every call that does not return
                // Z_BUF_ERROR, so the loop still terminates.
                inflateEnd(&stream);
                throw std::invalid_argument(
                    "Payload : the compressed data is truncated.");
            }
        } while (ret != Z_STREAM_END);

        inflateEnd(&stream);
        return decompressed;
    }

protected:
    /// @brief Return the six-bit value of a base64 character.
    /// @param ch the character
    /// @return its value, or -1 if it is not in the alphabet
    static int base64_value(const unsigned char ch)
    {
        if (ch >= 'A' && ch <= 'Z') {
            return ch - 'A';
        }
        if (ch >= 'a' && ch <= 'z') {
            return ch - 'a' + 26;
        }
        if (ch >= '0' && ch <= '9') {
            return ch - '0' + 52;
        }
        if (ch == '+') {
            return 62;
        }
        if (ch == '/') {
            return 63;
        }
        return -1;
    }

    /// @brief Append the bytes of one 4-character base64 group.
    /// @param out the buffer to append to
    /// @param accumulator the group's 24 bits, most significant first
    /// @param characters how many of the four characters were data rather than
    ///   padding
    /// @details Four data characters give 3 bytes, three give 2, and two give
    ///   1. One would give none, which ``base64_decode`` rejects before
    ///   reaching here.
    static void emit_group(std::vector<std::uint8_t> &out,
                           const std::uint32_t accumulator,
                           const std::size_t characters)
    {
        out.push_back((std::uint8_t)((accumulator >> 16) & 0xff));
        if (characters >= 3) {
            out.push_back((std::uint8_t)((accumulator >> 8) & 0xff));
        }
        if (characters >= 4) {
            out.push_back((std::uint8_t)(accumulator & 0xff));
        }
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_payload_codec_hpp__
