// This code is part of Qiskit.
//
// (C) Copyright IBM 2026.
//
// This code is licensed under the Apache License, Version 2.0. You may
// obtain a copy of this license in the LICENSE.txt file in the root directory
// of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "common.hpp"

#include "circuit/quantumcircuit.hpp"
#include "primitives/containers/circuit_serialization.hpp"
#include "primitives/containers/estimator_pub.hpp"
#include "primitives/containers/sampler_pub.hpp"

#if QISKIT_CPP_HAS_QPY
#include <zlib.h>
#endif

using namespace Qiskit;
using namespace Qiskit::circuit;
using namespace Qiskit::primitives;

static QuantumCircuit make_bell_circuit(void) {
    auto circ = QuantumCircuit(2, 2);
    circ.h(0);
    circ.cx(0, 1);
    circ.measure(0, 0);
    circ.measure(1, 1);
    return circ;
}

#if QISKIT_CPP_HAS_QPY
// Decode a standard base64 string. Only needed by the tests, to check the
// payload qiskit-cpp produces can be read back the way the service reads it.
static bool base64_decode(const std::string &encoded, std::vector<std::uint8_t> &decoded) {
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int lookup[256];
    for (int i = 0; i < 256; i++) {
        lookup[i] = -1;
    }
    for (int i = 0; i < 64; i++) {
        lookup[(unsigned char)alphabet[i]] = i;
    }

    if (encoded.size() % 4 != 0) {
        return false;
    }
    decoded.clear();
    decoded.reserve((encoded.size() / 4) * 3);

    for (std::size_t i = 0; i < encoded.size(); i += 4) {
        int values[4];
        std::size_t padding = 0;
        for (std::size_t j = 0; j < 4; j++) {
            const char ch = encoded[i + j];
            if (ch == '=') {
                // Padding is only legal in the final group's last two places.
                if (i + 4 != encoded.size() || j < 2) {
                    return false;
                }
                values[j] = 0;
                padding++;
            } else {
                values[j] = lookup[(unsigned char)ch];
                if (values[j] < 0 || padding > 0) {
                    return false;
                }
            }
        }
        const std::uint32_t triple = ((std::uint32_t)values[0] << 18)
            | ((std::uint32_t)values[1] << 12) | ((std::uint32_t)values[2] << 6)
            | (std::uint32_t)values[3];
        decoded.push_back((std::uint8_t)((triple >> 16) & 0xff));
        if (padding < 2) {
            decoded.push_back((std::uint8_t)((triple >> 8) & 0xff));
        }
        if (padding < 1) {
            decoded.push_back((std::uint8_t)(triple & 0xff));
        }
    }
    return true;
}

// Inflate a zlib stream, as Python's zlib.decompress does.
static bool zlib_decompress(const std::vector<std::uint8_t> &compressed,
                            std::vector<std::uint8_t> &decompressed) {
    z_stream stream;
    std::memset(&stream, 0, sizeof(stream));
    if (inflateInit(&stream) != Z_OK) {
        return false;
    }

    stream.next_in = (Bytef *)compressed.data();
    stream.avail_in = (uInt)compressed.size();

    decompressed.clear();
    std::vector<std::uint8_t> chunk(16384);
    int ret;
    do {
        stream.next_out = chunk.data();
        stream.avail_out = (uInt)chunk.size();
        ret = inflate(&stream, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&stream);
            return false;
        }
        decompressed.insert(decompressed.end(), chunk.begin(),
                            chunk.begin() + (chunk.size() - stream.avail_out));
    } while (ret != Z_STREAM_END);

    inflateEnd(&stream);
    return true;
}

static int test_base64_encode(void) {
    // Vectors from RFC 4648 section 10, which exercise all three padding cases.
    const std::string inputs[] = {"", "f", "fo", "foo", "foob", "fooba", "foobar"};
    const std::string expected[] = {"", "Zg==", "Zm8=", "Zm9v", "Zm9vYg==",
                                    "Zm9vYmE=", "Zm9vYmFy"};

    for (std::size_t i = 0; i < 7; i++) {
        const std::vector<std::uint8_t> data(inputs[i].begin(), inputs[i].end());
        const std::string actual = QpyCircuitPayload::base64_encode(data);
        if (actual != expected[i]) {
            std::cerr << "  base64_encode test : \"" << inputs[i] << "\" encoded as \""
                << actual << "\", expected \"" << expected[i] << "\"" << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_base64_encode_binary(void) {
    // All byte values, to catch an alphabet or shift error that ASCII input
    // would not reach.
    std::vector<std::uint8_t> data(256);
    for (std::size_t i = 0; i < 256; i++) {
        data[i] = (std::uint8_t)i;
    }

    const std::string encoded = QpyCircuitPayload::base64_encode(data);
    if (encoded.size() != 344) {
        std::cerr << "  base64_encode_binary test : encoded to " << encoded.size()
            << " characters, expected 344" << std::endl;
        return EqualityError;
    }

    std::vector<std::uint8_t> decoded;
    if (!base64_decode(encoded, decoded)) {
        std::cerr << "  base64_encode_binary test : the encoding could not be decoded"
            << std::endl;
        return EqualityError;
    }
    if (decoded != data) {
        std::cerr << "  base64_encode_binary test : the decoding differs from the input"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_zlib_round_trip(void) {
    std::vector<std::uint8_t> data(4096);
    for (std::size_t i = 0; i < data.size(); i++) {
        data[i] = (std::uint8_t)(i * 7 + (i >> 3));
    }

    const std::vector<std::uint8_t> compressed = QpyCircuitPayload::compress(data);
    // A zlib stream starts with a two-byte header whose first byte encodes the
    // compression method; 0x78 is deflate with a 32K window, what zlib writes.
    if (compressed.size() < 2 || compressed[0] != 0x78) {
        std::cerr << "  zlib_round_trip test : output does not start with a zlib header"
            << std::endl;
        return EqualityError;
    }

    std::vector<std::uint8_t> decompressed;
    if (!zlib_decompress(compressed, decompressed)) {
        std::cerr << "  zlib_round_trip test : the output could not be inflated" << std::endl;
        return EqualityError;
    }
    if (decompressed != data) {
        std::cerr << "  zlib_round_trip test : the inflated data differs from the input"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_qpy_payload_shape(void) {
    auto circ = make_bell_circuit();
    const nlohmann::ordered_json payload = QpyCircuitPayload::to_json(circ);

    if (!payload.is_object() || !payload.contains("__type__")
        || !payload.contains("__value__")) {
        std::cerr << "  qpy_payload_shape test : payload is not a {__type__, __value__} object"
            << std::endl;
        return EqualityError;
    }
    if (payload["__type__"] != "QuantumCircuit") {
        std::cerr << "  qpy_payload_shape test : __type__ is \""
            << payload["__type__"].get<std::string>() << "\", expected \"QuantumCircuit\""
            << std::endl;
        return EqualityError;
    }
    if (!payload["__value__"].is_string()) {
        std::cerr << "  qpy_payload_shape test : __value__ is not a string" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_qpy_payload_decodes_to_qpy(void) {
    // Walk the payload back the way Qiskit Runtime does: base64 decode, zlib
    // decompress, then read the QPY file header.
    auto circ = make_bell_circuit();
    const nlohmann::ordered_json payload = QpyCircuitPayload::to_json(circ);

    std::vector<std::uint8_t> compressed;
    if (!base64_decode(payload["__value__"].get<std::string>(), compressed)) {
        std::cerr << "  qpy_payload_decodes test : __value__ is not valid base64" << std::endl;
        return EqualityError;
    }

    std::vector<std::uint8_t> qpy;
    if (!zlib_decompress(compressed, qpy)) {
        std::cerr << "  qpy_payload_decodes test : __value__ is not valid zlib data"
            << std::endl;
        return EqualityError;
    }

    // The QPY header is b"QISKIT", a version byte, the 3-byte Qiskit version,
    // then the program count as a big-endian u64.
    const char magic[] = {'Q', 'I', 'S', 'K', 'I', 'T'};
    if (qpy.size() < 18) {
        std::cerr << "  qpy_payload_decodes test : only " << qpy.size()
            << " bytes, too short for a QPY header" << std::endl;
        return EqualityError;
    }
    for (std::size_t i = 0; i < 6; i++) {
        if (qpy[i] != (std::uint8_t)magic[i]) {
            std::cerr << "  qpy_payload_decodes test : missing the QPY magic" << std::endl;
            return EqualityError;
        }
    }

    // The version must be the one Qiskit Runtime accepts. Qiskit writes 18 by
    // default, which the service rejects, so this guards the explicit pin.
    const std::uint8_t version = qpy[6];
    if (version != QpyCircuitPayload::runtime_qpy_version()) {
        std::cerr << "  qpy_payload_decodes test : QPY version " << (unsigned)version
            << ", expected " << (unsigned)QpyCircuitPayload::runtime_qpy_version() << std::endl;
        return EqualityError;
    }
    if (version > 17 || version < 13) {
        std::cerr << "  qpy_payload_decodes test : QPY version " << (unsigned)version
            << " is outside the range Qiskit Runtime accepts (13-17)" << std::endl;
        return EqualityError;
    }

    // The service requires exactly one circuit in the payload.
    std::uint64_t num_programs = 0;
    for (std::size_t i = 0; i < 8; i++) {
        num_programs = (num_programs << 8) | qpy[10 + i];
    }
    if (num_programs != 1) {
        std::cerr << "  qpy_payload_decodes test : payload holds " << num_programs
            << " circuits, expected 1" << std::endl;
        return EqualityError;
    }

    // The payload must load back as the circuit that went in.
    const auto loaded = QuantumCircuit::from_qpy(qpy);
    if (loaded.size() != 1 || !(loaded[0] == circ)) {
        std::cerr << "  qpy_payload_decodes test : the payload did not round-trip"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_pub_uses_qpy(void) {
    auto circ = make_bell_circuit();
    auto pub = SamplerPub(circ, 128);

    if (pub.serialization() != CircuitSerialization::qpy) {
        std::cerr << "  sampler_pub_uses_qpy test : QPY is not the default" << std::endl;
        return EqualityError;
    }

    const auto payload = pub.to_json();
    if (!payload.is_array() || payload.size() != 3) {
        std::cerr << "  sampler_pub_uses_qpy test : PUB has " << payload.size()
            << " elements, expected 3" << std::endl;
        return EqualityError;
    }
    if (!payload[0].is_object() || payload[0]["__type__"] != "QuantumCircuit") {
        std::cerr << "  sampler_pub_uses_qpy test : the circuit is not a QPY payload"
            << std::endl;
        return EqualityError;
    }
    if (!payload[2].is_number() || payload[2].get<uint_t>() != 128) {
        std::cerr << "  sampler_pub_uses_qpy test : shots did not survive" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_pub_uses_qpy(void) {
    auto circ = QuantumCircuit(1, 0);
    auto pub = EstimatorPub(circ, std::string("Z"), 0.01);

    if (pub.serialization() != CircuitSerialization::qpy) {
        std::cerr << "  estimator_pub_uses_qpy test : QPY is not the default" << std::endl;
        return EqualityError;
    }

    const auto payload = pub.to_json();
    if (!payload.is_array() || payload.size() != 4) {
        std::cerr << "  estimator_pub_uses_qpy test : PUB has " << payload.size()
            << " elements, expected 4" << std::endl;
        return EqualityError;
    }
    if (!payload[0].is_object() || payload[0]["__type__"] != "QuantumCircuit") {
        std::cerr << "  estimator_pub_uses_qpy test : the circuit is not a QPY payload"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_qpy_payload_parameterized(void) {
    // QPY carries symbolic parameters, which is the main reason to prefer it
    // over the OpenQASM 3 exporter.
    auto circ = QuantumCircuit(1, 0);
    auto theta = Parameter("theta");
    circ.rx(theta, 0);

    const nlohmann::ordered_json payload = QpyCircuitPayload::to_json(circ);
    std::vector<std::uint8_t> compressed;
    std::vector<std::uint8_t> qpy;
    if (!base64_decode(payload["__value__"].get<std::string>(), compressed)
        || !zlib_decompress(compressed, qpy)) {
        std::cerr << "  qpy_payload_parameterized test : the payload could not be decoded"
            << std::endl;
        return EqualityError;
    }

    const auto loaded = QuantumCircuit::from_qpy(qpy);
    if (loaded.size() != 1 || loaded[0].num_parameters() != 1) {
        std::cerr << "  qpy_payload_parameterized test : the parameter did not survive"
            << std::endl;
        return EqualityError;
    }
    const auto symbols = loaded[0].parameter_symbols();
    if (symbols.size() != 1 || symbols[0] != "theta") {
        std::cerr << "  qpy_payload_parameterized test : the parameter symbol did not survive"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}
#endif // QISKIT_CPP_HAS_QPY

static int test_sampler_pub_qasm3_fallback(void) {
    // OpenQASM 3 stays available and is still accepted by Qiskit Runtime.
    auto circ = make_bell_circuit();
    auto pub = SamplerPub(circ, 64);
    pub.set_serialization(CircuitSerialization::qasm3);

    const auto payload = pub.to_json();
    if (!payload[0].is_string()) {
        std::cerr << "  sampler_pub_qasm3_fallback test : the circuit is not a string"
            << std::endl;
        return EqualityError;
    }
    if (payload[0].get<std::string>().find("OPENQASM 3.0;") != 0) {
        std::cerr << "  sampler_pub_qasm3_fallback test : the circuit is not OpenQASM 3"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_serialization_survives_copy(void) {
    // The pubs are copied into the payload builders, so the selection has to
    // survive a copy or a fallback would silently revert to QPY.
    auto circ = make_bell_circuit();
    auto pub = SamplerPub(circ, 64);
    pub.set_serialization(CircuitSerialization::qasm3);

    SamplerPub copied(pub);
    if (copied.serialization() != CircuitSerialization::qasm3) {
        std::cerr << "  serialization_survives_copy test : the selection was lost on copy"
            << std::endl;
        return EqualityError;
    }
    if (!copied.to_json()[0].is_string()) {
        std::cerr << "  serialization_survives_copy test : the copy did not use OpenQASM 3"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

#if defined(_WIN32)
int test_circuit_serialization(int argc, char** const argv) {
#else
int test_circuit_serialization(int argc, char** argv) {
#endif
    int num_failed = 0;
#if QISKIT_CPP_HAS_QPY
    num_failed += RUN_TEST(test_base64_encode);
    num_failed += RUN_TEST(test_base64_encode_binary);
    num_failed += RUN_TEST(test_zlib_round_trip);
    num_failed += RUN_TEST(test_qpy_payload_shape);
    num_failed += RUN_TEST(test_qpy_payload_decodes_to_qpy);
    num_failed += RUN_TEST(test_qpy_payload_parameterized);
    num_failed += RUN_TEST(test_sampler_pub_uses_qpy);
    num_failed += RUN_TEST(test_estimator_pub_uses_qpy);
#endif
    num_failed += RUN_TEST(test_sampler_pub_qasm3_fallback);
    num_failed += RUN_TEST(test_serialization_survives_copy);

    std::cerr << "=== Number of failed subtests: " << num_failed << std::endl;
    return num_failed;
}
