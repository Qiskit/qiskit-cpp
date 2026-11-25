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

// Parameter values payload for Qiskit Runtime primitive inputs

#ifndef __qiskitcpp_primitives_parameter_values_hpp__
#define __qiskitcpp_primitives_parameter_values_hpp__

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <zlib.h>

#include "primitives/containers/payload_codec.hpp"
#include "utils/types.hpp"

namespace Qiskit {
namespace primitives {

/// @class ParameterValues
/// @brief The parameter values of a primitive PUB, bound on the server.
/// @details Qiskit Runtime takes the parameter values of a PUB as the object
///   ``{"__type__": "ndarray", "__value__": <data>}``, where ``data`` is the
///   base64 encoding of the zlib-compressed NumPy ``.npy`` serialization of the
///   value array. This matches ``NdarrayWrapperModel`` in the
///   ``ibm-quantum-schemas`` package, which defines the wire format for both
///   ``SamplerPubModel`` and ``EstimatorPubModel``.
///
///   The values are supplied as one flat list per binding set, in the order the
///   circuit's parameters are bound. A single binding set is written as a 1-D
///   array of length ``num_parameters``; several sets are written as a 2-D
///   array of shape ``(num_sets, num_parameters)``, which Qiskit sweeps over.
///   The last dimension always indexes the circuit parameters.
/// @note The binding order is positional. Qiskit orders a circuit's parameters
///   alphabetically by name (see ``QuantumCircuit.parameters``), which is the
///   same order ``QuantumCircuit::parameter_symbols()`` returns, so that list
///   gives the positions to fill. Insertion order into the circuit does not
///   determine it.
class ParameterValues {
public:
    /// @brief Build the payload for a set of parameter values.
    /// @param values the value sets, each of length ``num_parameters``
    /// @param num_parameters the number of parameters in the circuit
    /// @return the ``{"__type__", "__value__"}`` object to place in a PUB
    /// @throws std::invalid_argument if a set does not hold exactly
    ///   ``num_parameters`` values
    /// @throws std::runtime_error if the payload cannot be compressed
    static nlohmann::ordered_json
    to_json(const std::vector<std::vector<double>> &values, const uint_t num_parameters)
    {
        validate(values, num_parameters);

        std::vector<std::size_t> shape;
        std::vector<double> flat;
        if (values.empty()) {
            // No values to bind: an empty 1-D array, which is what Qiskit
            // Runtime expects for a circuit with no parameters.
            shape.push_back(0);
        } else if (values.size() == 1) {
            // One binding set is a 1-D array indexed by parameter.
            shape.push_back((std::size_t)num_parameters);
            flat = values[0];
        } else {
            // Several sets form a sweep: the leading dimension indexes the set
            // and the last indexes the parameter.
            shape.push_back(values.size());
            shape.push_back((std::size_t)num_parameters);
            flat.reserve(values.size() * (std::size_t)num_parameters);
            for (std::size_t i = 0; i < values.size(); i++) {
                flat.insert(flat.end(), values[i].begin(), values[i].end());
            }
        }

        nlohmann::ordered_json ret;
        ret["__type__"] = "ndarray";
        ret["__value__"] = base64_encode(compress(to_npy(flat, shape)));
        return ret;
    }

    /// @brief Build the payload for a single set of parameter values.
    /// @param values the values, in the order the parameters are bound
    /// @param num_parameters the number of parameters in the circuit
    /// @return the ``{"__type__", "__value__"}`` object to place in a PUB
    static nlohmann::ordered_json
    to_json(const std::vector<double> &values, const uint_t num_parameters)
    {
        if (values.empty() && num_parameters == 0) {
            return to_json(std::vector<std::vector<double>>(), num_parameters);
        }
        return to_json(std::vector<std::vector<double>>(1, values), num_parameters);
    }

    /// @brief Check that the value sets match the circuit's parameter count.
    /// @param values the value sets
    /// @param num_parameters the number of parameters in the circuit
    /// @throws std::invalid_argument on a mismatch
    static void validate(const std::vector<std::vector<double>> &values,
                         const uint_t num_parameters)
    {
        if (values.empty()) {
            if (num_parameters != 0) {
                std::stringstream message;
                message << "The circuit has " << num_parameters
                        << " parameters but no parameter values were given.";
                throw std::invalid_argument(message.str());
            }
            return;
        }

        for (std::size_t i = 0; i < values.size(); i++) {
            if (values[i].size() != (std::size_t)num_parameters) {
                std::stringstream message;
                message << "Parameter value set " << i << " holds " << values[i].size()
                        << " values but the circuit has " << num_parameters
                        << " parameters.";
                throw std::invalid_argument(message.str());
            }
        }
    }

    /// @brief Serialize an array of doubles in the NumPy ``.npy`` format.
    /// @param data the elements, in C order
    /// @param shape the dimensions of the array
    /// @return the ``.npy`` bytes
    /// @details Writes version 1.0 of the format: the magic string
    ///   ``\x93NUMPY``, the version, a little-endian ``uint16`` header length,
    ///   an ASCII header describing the dtype and shape padded so that the
    ///   total header is a multiple of 64 bytes, then the raw elements. The
    ///   dtype is ``<f8``, little-endian IEEE 754 double.
    static std::vector<std::uint8_t> to_npy(const std::vector<double> &data,
                                            const std::vector<std::size_t> &shape)
    {
        // The shape is written as Python's repr of a tuple, which NumPy's
        // reader parses: a 1-D shape carries a trailing comma so it stays a
        // tuple, and elements of a longer shape are separated by ", ".
        std::stringstream header;
        header << "{'descr': '<f8', 'fortran_order': False, 'shape': (";
        for (std::size_t i = 0; i < shape.size(); i++) {
            if (i > 0) {
                header << ", ";
            }
            header << shape[i];
        }
        if (shape.size() == 1) {
            header << ",";
        }
        header << "), }";

        std::string header_str = header.str();
        // The magic, version and length field occupy 10 bytes. The header is
        // padded with spaces and newline-terminated so the whole prefix is a
        // multiple of 64 bytes, which keeps the data aligned. This mirrors
        // NumPy's own arithmetic in _wrap_header: the pad is always between 1
        // and 64, never 0, so the output matches np.save byte for byte.
        const std::size_t prefix = 10;
        const std::size_t align = 64;
        const std::size_t unpadded_len = header_str.size() + 1; // + the newline
        const std::size_t pad = align - ((prefix + unpadded_len) % align);
        header_str.append(pad, ' ');
        header_str.append(1, '\n');

        std::vector<std::uint8_t> npy;
        npy.reserve(prefix + header_str.size() + data.size() * sizeof(double));

        const char magic[] = {'\x93', 'N', 'U', 'M', 'P', 'Y'};
        npy.insert(npy.end(), magic, magic + sizeof(magic));
        npy.push_back(1); // major version
        npy.push_back(0); // minor version

        const std::size_t header_len = header_str.size();
        npy.push_back((std::uint8_t)(header_len & 0xff));
        npy.push_back((std::uint8_t)((header_len >> 8) & 0xff));

        npy.insert(npy.end(), header_str.begin(), header_str.end());

        for (std::size_t i = 0; i < data.size(); i++) {
            const std::vector<std::uint8_t> bytes = double_to_le(data[i]);
            npy.insert(npy.end(), bytes.begin(), bytes.end());
        }
        return npy;
    }

    /// @brief Compress a buffer with zlib, in the format Qiskit Runtime expects.
    /// @param data the bytes to compress
    /// @return the compressed bytes
    /// @throws std::runtime_error if compression fails
    /// @details Produces a zlib stream (RFC 1950), which is what Python's
    ///   ``zlib.compress`` writes and ``zlib.decompress`` reads.
    static std::vector<std::uint8_t> compress(const std::vector<std::uint8_t> &data)
    {
        uLongf destination_size = compressBound((uLong)data.size());
        std::vector<std::uint8_t> compressed(destination_size > 0 ? destination_size : 1);

        // zlib takes a non-null source pointer even for an empty input.
        const Bytef dummy = 0;
        const Bytef *source = data.empty() ? &dummy : (const Bytef *)data.data();

        const int ret = compress2(compressed.data(), &destination_size, source,
                                  (uLong)data.size(), Z_DEFAULT_COMPRESSION);
        if (ret != Z_OK) {
            std::stringstream message;
            message << "Parameter values : zlib compression failed (" << ret << ")";
            throw std::runtime_error(message.str());
        }

        compressed.resize(destination_size);
        return compressed;
    }

    /// @brief Encode a buffer as standard base64.
    /// @param data the bytes to encode
    /// @return the base64 encoding, padded with '=' to a multiple of 4
    /// @details Uses the standard alphabet of RFC 4648 section 4, matching
    ///   Python's ``base64.b64encode``. No line breaks are inserted.
    static std::string base64_encode(const std::vector<std::uint8_t> &data)
    {
        static const char alphabet[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string encoded;
        encoded.reserve(((data.size() + 2) / 3) * 4);

        std::size_t i = 0;
        for (; i + 2 < data.size(); i += 3) {
            const std::uint32_t triple = ((std::uint32_t)data[i] << 16)
                | ((std::uint32_t)data[i + 1] << 8)
                | (std::uint32_t)data[i + 2];
            encoded += alphabet[(triple >> 18) & 0x3f];
            encoded += alphabet[(triple >> 12) & 0x3f];
            encoded += alphabet[(triple >> 6) & 0x3f];
            encoded += alphabet[triple & 0x3f];
        }

        const std::size_t remaining = data.size() - i;
        if (remaining == 1) {
            const std::uint32_t triple = (std::uint32_t)data[i] << 16;
            encoded += alphabet[(triple >> 18) & 0x3f];
            encoded += alphabet[(triple >> 12) & 0x3f];
            encoded += "==";
        } else if (remaining == 2) {
            const std::uint32_t triple = ((std::uint32_t)data[i] << 16)
                | ((std::uint32_t)data[i + 1] << 8);
            encoded += alphabet[(triple >> 18) & 0x3f];
            encoded += alphabet[(triple >> 12) & 0x3f];
            encoded += alphabet[(triple >> 6) & 0x3f];
            encoded += "=";
        }

        return encoded;
    }

    /// @brief Read parameter values back from a Qiskit Runtime ndarray payload.
    /// @param input the ``{"__type__", "__value__"}`` object written by
    ///   ``to_json``
    /// @return one list of values per binding set
    /// @throws std::invalid_argument if the object is not an ndarray payload,
    ///   or holds an array this cannot represent
    /// @details The inverse of ``to_json``: the ``__value__`` string is base64
    ///   decoded, decompressed, and read as a NumPy ``.npy`` array. A 1-D array
    ///   gives one binding set and a 2-D array one set per row, matching the
    ///   shapes ``to_json`` writes.
    /// @note An empty 1-D array comes back as no sets rather than as one empty
    ///   set. ``to_json`` writes shape ``(0,)`` for both, so the two cannot be
    ///   told apart; no sets is the form ``set_parameter_values`` stores for a
    ///   circuit with no parameters.
    static std::vector<std::vector<double>>
    from_json(const nlohmann::ordered_json &input)
    {
        if (!input.is_object()) {
            throw std::invalid_argument(
                "Parameter values : expected an object holding \"__type__\" and \"__value__\".");
        }
        if (!input.contains("__type__") || !input["__type__"].is_string()
            || input["__type__"].get<std::string>() != "ndarray") {
            throw std::invalid_argument(
                "Parameter values : \"__type__\" must be \"ndarray\".");
        }
        if (!input.contains("__value__") || !input["__value__"].is_string()) {
            throw std::invalid_argument(
                "Parameter values : \"__value__\" must be a base64 string.");
        }

        const std::vector<std::uint8_t> npy = PayloadCodec::decompress(
            PayloadCodec::base64_decode(input["__value__"].get<std::string>()));

        std::vector<std::size_t> shape;
        const std::vector<double> flat = from_npy(npy, shape);

        if (shape.size() == 1) {
            if (shape[0] == 0) {
                // No values to bind.
                return std::vector<std::vector<double>>();
            }
            return std::vector<std::vector<double>>(1, flat);
        }

        if (shape.size() == 2) {
            const std::size_t num_sets = shape[0];
            const std::size_t num_parameters = shape[1];
            if (num_sets == 0 || num_parameters == 0) {
                return std::vector<std::vector<double>>();
            }
            std::vector<std::vector<double>> values(num_sets);
            for (std::size_t i = 0; i < num_sets; i++) {
                // C order, so each set is a contiguous run of the flat data.
                values[i].assign(flat.begin() + (i * num_parameters),
                                 flat.begin() + ((i + 1) * num_parameters));
            }
            return values;
        }

        std::stringstream message;
        message << "Parameter values : the array has " << shape.size()
                << " dimensions, but only a 1-D or 2-D array of values can be read.";
        throw std::invalid_argument(message.str());
    }

    /// @brief Read an array of doubles from the NumPy ``.npy`` format.
    /// @param npy the ``.npy`` bytes
    /// @param shape output for the dimensions of the array
    /// @return the elements, in C order
    /// @throws std::invalid_argument if the bytes are not a ``.npy`` array of
    ///   little-endian doubles in C order
    /// @details The inverse of ``to_npy``. Accepts format version 1 (a
    ///   ``uint16`` header length) as well as 2 and 3 (a ``uint32`` one), even
    ///   though ``to_npy`` only writes version 1, since NumPy switches versions
    ///   on the header length alone.
    static std::vector<double> from_npy(const std::vector<std::uint8_t> &npy,
                                        std::vector<std::size_t> &shape)
    {
        const char magic[] = {'\x93', 'N', 'U', 'M', 'P', 'Y'};
        const std::size_t magic_len = sizeof(magic);
        if (npy.size() < magic_len + 2
            || std::memcmp(npy.data(), magic, magic_len) != 0) {
            throw std::invalid_argument(
                "Parameter values : the data is not a NumPy .npy array.");
        }

        const std::uint8_t major = npy[magic_len];
        std::size_t offset = magic_len + 2; // past the two version bytes
        std::size_t header_len = 0;

        if (major == 1) {
            if (npy.size() < offset + 2) {
                throw std::invalid_argument(
                    "Parameter values : the .npy header length is truncated.");
            }
            header_len = (std::size_t)npy[offset]
                | ((std::size_t)npy[offset + 1] << 8);
            offset += 2;
        } else if (major == 2 || major == 3) {
            if (npy.size() < offset + 4) {
                throw std::invalid_argument(
                    "Parameter values : the .npy header length is truncated.");
            }
            header_len = (std::size_t)npy[offset]
                | ((std::size_t)npy[offset + 1] << 8)
                | ((std::size_t)npy[offset + 2] << 16)
                | ((std::size_t)npy[offset + 3] << 24);
            offset += 4;
        } else {
            std::stringstream message;
            message << "Parameter values : .npy format version " << (int)major
                    << " is not supported.";
            throw std::invalid_argument(message.str());
        }

        if (npy.size() < offset + header_len) {
            throw std::invalid_argument(
                "Parameter values : the .npy header is truncated.");
        }
        const std::string header((const char *)npy.data() + offset, header_len);
        offset += header_len;

        // The dtype must be a little-endian double. '|' and '=' are not
        // accepted for it: a double is never byte-order agnostic, and '=' means
        // whatever the writing host was, which is not recorded.
        const std::string descr = header_field(header, "descr");
        if (descr != "<f8") {
            throw std::invalid_argument(
                "Parameter values : the .npy array has dtype \"" + descr
                + "\", but only \"<f8\" (little-endian double) can be read.");
        }
        const std::string fortran_order = header_field(header, "fortran_order");
        if (fortran_order != "False") {
            throw std::invalid_argument(
                "Parameter values : the .npy array is in Fortran order, but only "
                "C order can be read.");
        }

        shape = header_shape(header);

        std::size_t count = 1;
        for (std::size_t i = 0; i < shape.size(); i++) {
            count *= shape[i];
        }
        if (shape.empty()) {
            // A 0-d array holds one element, which is not a list of parameter
            // values. Rejected here rather than silently read as one value.
            throw std::invalid_argument(
                "Parameter values : the .npy array is zero-dimensional.");
        }

        const std::size_t available = npy.size() - offset;
        if (available < count * sizeof(double)) {
            std::stringstream message;
            message << "Parameter values : the .npy array declares " << count
                    << " elements but holds only " << (available / sizeof(double))
                    << ".";
            throw std::invalid_argument(message.str());
        }

        std::vector<double> data(count);
        for (std::size_t i = 0; i < count; i++) {
            data[i] = le_to_double(npy.data() + offset + (i * sizeof(double)));
        }
        return data;
    }

protected:
    /// @brief Read a field out of a ``.npy`` header dictionary.
    /// @param header the header text
    /// @param key the key to read, without quotes
    /// @return the value, with surrounding quotes and whitespace removed
    /// @throws std::invalid_argument if the key is absent
    /// @details The header is a Python dict literal. Only the three keys NumPy
    ///   always writes are needed, and each has a simple scalar value, so the
    ///   field is located by key rather than by parsing Python.
    static std::string header_field(const std::string &header,
                                    const std::string &key)
    {
        const std::string quoted = "'" + key + "'";
        std::size_t at = header.find(quoted);
        if (at == std::string::npos) {
            // NumPy writes single quotes, but accept double quotes as well
            // rather than failing on a header some other writer produced.
            const std::string alternative = "\"" + key + "\"";
            at = header.find(alternative);
            if (at == std::string::npos) {
                throw std::invalid_argument(
                    "Parameter values : the .npy header has no \"" + key + "\" field.");
            }
            at += alternative.size();
        } else {
            at += quoted.size();
        }

        at = header.find(':', at);
        if (at == std::string::npos) {
            throw std::invalid_argument(
                "Parameter values : the .npy header field \"" + key
                + "\" has no value.");
        }
        at++;

        // The value runs to the next comma that is not inside the shape tuple.
        const std::size_t end = header.find(',', at);
        std::string value = header.substr(at, end == std::string::npos
                                                  ? std::string::npos
                                                  : end - at);
        return trim_header_value(value);
    }

    /// @brief Read the shape tuple out of a ``.npy`` header dictionary.
    /// @param header the header text
    /// @return the dimensions
    /// @throws std::invalid_argument if the shape is absent or malformed
    static std::vector<std::size_t> header_shape(const std::string &header)
    {
        std::size_t at = header.find("'shape'");
        std::size_t key_len = 7;
        if (at == std::string::npos) {
            at = header.find("\"shape\"");
            if (at == std::string::npos) {
                throw std::invalid_argument(
                    "Parameter values : the .npy header has no \"shape\" field.");
            }
        }
        at = header.find('(', at + key_len);
        if (at == std::string::npos) {
            throw std::invalid_argument(
                "Parameter values : the .npy header shape is not a tuple.");
        }
        const std::size_t end = header.find(')', at);
        if (end == std::string::npos) {
            throw std::invalid_argument(
                "Parameter values : the .npy header shape is not closed.");
        }

        std::vector<std::size_t> shape;
        const std::string inner = header.substr(at + 1, end - at - 1);
        std::string digits;
        for (std::size_t i = 0; i <= inner.size(); i++) {
            const char ch = i < inner.size() ? inner[i] : ',';
            if (ch >= '0' && ch <= '9') {
                digits += ch;
                continue;
            }
            if (ch == ',') {
                // A trailing comma keeps a 1-D shape a Python tuple and leaves
                // nothing to read, so an empty run is skipped rather than
                // becoming a zero dimension.
                if (!digits.empty()) {
                    shape.push_back((std::size_t)std::strtoull(digits.c_str(), nullptr, 10));
                    digits.clear();
                }
                continue;
            }
            if (std::isspace((unsigned char)ch)) {
                continue;
            }
            throw std::invalid_argument(
                "Parameter values : the .npy header shape holds a non-numeric entry.");
        }
        return shape;
    }

    /// @brief Strip whitespace and quotes from a header value.
    static std::string trim_header_value(const std::string &value)
    {
        std::size_t begin = 0;
        std::size_t end = value.size();
        while (begin < end && std::isspace((unsigned char)value[begin])) {
            begin++;
        }
        while (end > begin && std::isspace((unsigned char)value[end - 1])) {
            end--;
        }
        if (end - begin >= 2
            && (value[begin] == '\'' || value[begin] == '"')
            && value[end - 1] == value[begin]) {
            begin++;
            end--;
        }
        return value.substr(begin, end - begin);
    }

    /// @brief Read a double from its little-endian bytes.
    /// @param bytes the 8 bytes, least significant first
    /// @return the value
    /// @details The inverse of ``double_to_le``. Reassembles through an integer
    ///   so the result is correct on a big-endian host too, where a plain
    ///   memcpy would read the bytes in the wrong order.
    static double le_to_double(const std::uint8_t *bytes)
    {
        std::uint64_t bits = 0;
        for (std::size_t i = 0; i < sizeof(bits); i++) {
            bits |= ((std::uint64_t)bytes[i]) << (8 * i);
        }
        double value = 0.0;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    /// @brief Return the little-endian bytes of a double.
    /// @param value the value to convert
    /// @return its 8 bytes, least significant first
    /// @details Copies through an integer so the bytes can be ordered
    ///   explicitly; this is correct on a big-endian host as well, where a
    ///   plain memcpy of the double would produce the wrong order.
    static std::vector<std::uint8_t> double_to_le(const double value)
    {
        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));

        std::vector<std::uint8_t> bytes(sizeof(bits));
        for (std::size_t i = 0; i < sizeof(bits); i++) {
            bytes[i] = (std::uint8_t)((bits >> (8 * i)) & 0xff);
        }
        return bytes;
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_parameter_values_hpp__
