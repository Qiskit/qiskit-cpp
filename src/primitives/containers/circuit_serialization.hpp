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

// Circuit serialization for Qiskit Runtime primitive inputs

#ifndef __qiskitcpp_primitives_circuit_serialization_hpp__
#define __qiskitcpp_primitives_circuit_serialization_hpp__

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "circuit/quantumcircuit.hpp"
#include "utils/capi_features.hpp"

#if QISKIT_CPP_HAS_QPY
#include <zlib.h>
#endif

namespace Qiskit {
namespace primitives {

/// @class QpyCircuitPayload
/// @brief Serialize a circuit as a Qiskit Runtime QPY circuit payload.
/// @details Qiskit Runtime accepts a primitive PUB circuit either as an
///   OpenQASM 3 string or as a QPY payload. The QPY form is the object
///   ``{"__type__": "QuantumCircuit", "__value__": <data>}``, where ``data`` is
///   the base64 encoding of the zlib-compressed QPY serialization of a single
///   circuit. This matches ``TypedQpyCircuitModel`` in the ``ibm-quantum-schemas``
///   package, which defines the wire format.
class QpyCircuitPayload {
public:
#if QISKIT_CPP_HAS_QPY
    /// @brief The QPY format version written for Qiskit Runtime.
    /// @return the pinned format version
    /// @details Pinned rather than left to Qiskit's default because the two ends
    ///   only overlap at this one version. Qiskit's C-API refuses to write
    ///   anything below ``qk_qpy_write_min_version()`` (17 as of Qiskit 2.6) and
    ///   writes 18 by default, while Qiskit Runtime's ``SamplerPubModel`` and
    ///   ``EstimatorPubModel`` validate the payload against
    ///   ``TypedQpyCircuitModelV13to17``, which rejects anything above 17.
    ///   Sending Qiskit's default would therefore be refused by the service.
    /// @note A function rather than a static data member so that the header
    ///   stays self-contained: an out-of-line definition would need a .cpp,
    ///   which this header-only library does not have.
    static std::uint8_t runtime_qpy_version(void)
    {
        return 17;
    }

    /// @brief Serialize a circuit as a Qiskit Runtime QPY circuit payload.
    /// @param circuit the circuit to serialize
    /// @return the ``{"__type__", "__value__"}`` object to place in a PUB
    /// @throws std::runtime_error if the circuit cannot be serialized or
    ///   compressed
    static nlohmann::ordered_json to_json(circuit::QuantumCircuit &circuit)
    {
        // A QPY payload always holds a list of circuits; the Runtime schema
        // requires that list to hold exactly one, which to_qpy gives us.
        const std::vector<std::uint8_t> qpy = circuit.to_qpy(runtime_qpy_version());

        nlohmann::ordered_json ret;
        ret["__type__"] = "QuantumCircuit";
        ret["__value__"] = base64_encode(compress(qpy));
        return ret;
    }

    /// @brief Compress a buffer with zlib, in the format Qiskit Runtime expects.
    /// @param data the bytes to compress
    /// @return the compressed bytes
    /// @throws std::runtime_error if compression fails
    /// @details Produces a zlib stream (RFC 1950), which is what Python's
    ///   ``zlib.compress`` writes and ``zlib.decompress`` reads. Note this is
    ///   neither a raw deflate stream nor a gzip stream.
    static std::vector<std::uint8_t> compress(const std::vector<std::uint8_t> &data)
    {
        // compressBound gives the worst-case size, so one pass always suffices.
        uLongf destination_size = compressBound((uLong)data.size());
        std::vector<std::uint8_t> compressed(destination_size > 0 ? destination_size : 1);

        // zlib takes a non-null source pointer even for an empty input.
        const Bytef dummy = 0;
        const Bytef *source = data.empty() ? &dummy : (const Bytef *)data.data();

        const int ret = compress2(compressed.data(), &destination_size, source,
                                  (uLong)data.size(), Z_DEFAULT_COMPRESSION);
        if (ret != Z_OK) {
            std::stringstream message;
            message << "QPY payload : zlib compression failed (" << ret << ")";
            throw std::runtime_error(message.str());
        }

        compressed.resize(destination_size);
        return compressed;
    }

    /// @brief Encode a buffer as standard base64.
    /// @param data the bytes to encode
    /// @return the base64 encoding, padded with '=' to a multiple of 4 characters
    /// @details Uses the standard alphabet of RFC 4648 section 4, matching
    ///   Python's ``base64.b64encode``. No line breaks are inserted.
    static std::string base64_encode(const std::vector<std::uint8_t> &data)
    {
        static const char alphabet[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string encoded;
        encoded.reserve(((data.size() + 2) / 3) * 4);

        std::size_t i = 0;
        // Encode whole 3-byte groups as 4 characters.
        for (; i + 2 < data.size(); i += 3) {
            const std::uint32_t triple = ((std::uint32_t)data[i] << 16)
                | ((std::uint32_t)data[i + 1] << 8)
                | (std::uint32_t)data[i + 2];
            encoded += alphabet[(triple >> 18) & 0x3f];
            encoded += alphabet[(triple >> 12) & 0x3f];
            encoded += alphabet[(triple >> 6) & 0x3f];
            encoded += alphabet[triple & 0x3f];
        }

        // Encode the trailing 1 or 2 bytes, padding the output to 4 characters.
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
#endif // QISKIT_CPP_HAS_QPY

    /// @brief Whether circuits can be serialized as QPY.
    /// @return true if the loaded Qiskit provides the QPY C-API
    static bool available(void)
    {
#if QISKIT_CPP_HAS_QPY
        return true;
#else
        return false;
#endif
    }
};

/// @brief How a circuit is serialized into a primitive PUB.
enum class CircuitSerialization {
    /// Serialize as a QPY payload. Preferred: QPY is a binary format that
    /// round-trips the circuit losslessly, including symbolic parameters.
    qpy,
    /// Serialize as an OpenQASM 3 string.
    qasm3,
};

/// @brief Return the serialization used for PUB circuits by default.
/// @return CircuitSerialization::qpy when the loaded Qiskit provides the QPY
///   C-API, otherwise CircuitSerialization::qasm3
inline CircuitSerialization default_circuit_serialization(void)
{
    return QpyCircuitPayload::available() ? CircuitSerialization::qpy
                                          : CircuitSerialization::qasm3;
}

/// @brief Serialize a circuit for a primitive PUB.
/// @param circuit the circuit to serialize
/// @param serialization the serialization to use
/// @return the QPY payload object, or the OpenQASM 3 string
/// @details Qiskit Runtime accepts either form for a PUB circuit, so this is
///   the single place that chooses between them for both the sampler and the
///   estimator.
inline nlohmann::ordered_json serialize_pub_circuit(circuit::QuantumCircuit &circuit,
                                                    const CircuitSerialization serialization)
{
#if QISKIT_CPP_HAS_QPY
    if (serialization == CircuitSerialization::qpy) {
        return QpyCircuitPayload::to_json(circuit);
    }
#endif
    return circuit.to_qasm3();
}

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_circuit_serialization_hpp__
