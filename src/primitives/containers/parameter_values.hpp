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

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <zlib.h>

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

protected:
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
