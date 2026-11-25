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

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common.hpp"

#include "circuit/quantumcircuit.hpp"
#include "primitives/containers/circuit_serialization.hpp"
#include "primitives/containers/estimator_pub.hpp"
#include "primitives/containers/parameter_values.hpp"
#include "primitives/containers/payload_codec.hpp"
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

// Run `action`, which is required to reject its input, and report whether it
// did so for the stated reason.
//
// A test whose success condition is "this input is rejected" cannot report Ok
// merely because something was thrown. There are around twenty distinct
// std::invalid_argument sites behind these calls, so an exception of the right
// type raised by the wrong check -- or raised while building the test fixture
// rather than by the call under test -- is indistinguishable from the rejection
// being tested. Such a test passes while proving nothing, and keeps passing
// after the behaviour it was written for has gone.
//
// So a rejection counts only when `expected_in_message` appears in the
// exception's message, which identifies *which* rejection happened. Everything
// else is a failure with its own message saying which of the four outcomes
// occurred: rejected for the stated reason, rejected for a different reason,
// failed in some other way, or accepted.
//
// Matched on the message rather than on the exception type deliberately. These
// calls legitimately raise two unrelated hierarchies -- std::invalid_argument
// from qiskit-cpp's own checks and nlohmann::json::exception from JSON access
// -- so requiring a type would pin down which layer happened to reject the
// input, which is an implementation detail, while the message states the actual
// reason. std::exception is the common base of both and is what gets caught.
//
// Catching broadly also keeps an unexpected exception from escaping the test
// function: one that reaches the driver's main terminates the process, so it
// would end the whole run instead of failing a single subtest.
template <typename Action>
static int expect_rejected(const char *test_name, const std::string &what,
                           const std::string &expected_in_message, Action action) {
    try {
        action();
    } catch (const std::exception &e) {
        const std::string message(e.what());
        if (message.find(expected_in_message) != std::string::npos) {
            // Logged so that a pass earned by rejecting an input is visible in
            // the run, rather than silent and so indistinguishable from a test
            // that never reached the check at all.
            std::cerr << "    " << test_name << " : rejected " << what << " : "
                << message << std::endl;
            return Ok;
        }
        std::cerr << "  " << test_name << " test : " << what
            << " was rejected, but for a different reason than expected.\n"
            << "    expected the message to mention: " << expected_in_message << "\n"
            << "    actual message:                  " << message << std::endl;
        return EqualityError;
    } catch (...) {
        std::cerr << "  " << test_name << " test : " << what
            << " threw something that does not derive from std::exception"
            << std::endl;
        return EqualityError;
    }
    std::cerr << "  " << test_name << " test : " << what
        << " was accepted, but should have been rejected (expected the message "
        << "to mention: " << expected_in_message << ")" << std::endl;
    return EqualityError;
}

// Run `action`, which is expected to throw nothing, and report whether it did.
// Used where a test needs to carry on after the call rather than return, so the
// handler in `run()` would lose the context of which step failed.
template <typename Action>
static int expect_no_throw(const char *test_name, const std::string &what,
                           Action action) {
    try {
        action();
    } catch (const std::exception &e) {
        std::cerr << "  " << test_name << " test : " << what << " threw \""
            << e.what() << "\"" << std::endl;
        return EqualityError;
    }
    return Ok;
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

// ---------------------------------------------------------------------------
// SamplerPub::from_json
// ---------------------------------------------------------------------------

static int test_payload_base64_decode_round_trip(void) {
    // Every byte value, so the 3-byte grouping and both padding lengths are
    // all exercised against the encoder they have to invert.
    static const char *const name = "payload_base64_decode_round_trip";

    for (std::size_t length = 0; length <= 260; length++) {
        std::vector<std::uint8_t> data(length);
        for (std::size_t i = 0; i < length; i++) {
            data[i] = (std::uint8_t)((i * 7 + 13) & 0xff);
        }
        const std::string encoded = ParameterValues::base64_encode(data);

        // Checked rather than called directly so that a throw names the length
        // it happened at. The handler in `run()` would catch it either way, but
        // it can only report the subtest, not which of the 261 lengths.
        std::vector<std::uint8_t> decoded;
        const int result = expect_no_throw(
            name, "decoding length " + std::to_string(length),
            [&decoded, &encoded]() {
                decoded = PayloadCodec::base64_decode(encoded);
            });
        if (result != Ok) {
            return result;
        }

        if (decoded != data) {
            std::cerr << "  " << name << " test : length " << length
                << " did not round-trip" << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_payload_base64_decode_known_vectors(void) {
    // RFC 4648 section 10, which pins the padding behaviour independently of
    // this library's own encoder.
    struct Vector {
        const char *text;
        const char *encoded;
    };
    static const Vector vectors[] = {
        {"", ""},
        {"f", "Zg=="},
        {"fo", "Zm8="},
        {"foo", "Zm9v"},
        {"foob", "Zm9vYg=="},
        {"fooba", "Zm9vYmE="},
        {"foobar", "Zm9vYmFy"},
    };

    for (std::size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        const std::vector<std::uint8_t> decoded =
            PayloadCodec::base64_decode(vectors[i].encoded);
        const std::string actual((const char *)decoded.data(), decoded.size());
        if (actual != vectors[i].text) {
            std::cerr << "  payload_base64_decode_known_vectors test : \""
                << vectors[i].encoded << "\" decoded to \"" << actual
                << "\", expected \"" << vectors[i].text << "\"" << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_payload_base64_decode_rejects_bad_input(void) {
    // Each case names the rejection it must trigger, so a case cannot pass by
    // tripping one of the other checks.
    struct Case {
        const char *text;
        const char *expected;
    };
    static const Case bad[] = {
        {"Zm9", "its length is not a multiple of 4"},
        {"Zm9v!", "is not a base64 character"},
        {"Zg===", "more than two padding characters"},
        {"Zg==Zg==", "a character after its padding"},
    };
    for (std::size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        const std::string text(bad[i].text);
        const int result = expect_rejected(
            "payload_base64_decode_rejects_bad_input", "\"" + text + "\"",
            bad[i].expected,
            [&text]() { PayloadCodec::base64_decode(text); });
        if (result != Ok) {
            return result;
        }
    }
    return Ok;
}

static int test_payload_decompress_round_trip(void) {
    // Including a highly compressible buffer large enough to need several
    // inflate passes through the 64 KiB window.
    std::vector<std::uint8_t> data(500000, 0x5a);
    for (std::size_t i = 0; i < data.size(); i += 997) {
        data[i] = (std::uint8_t)(i & 0xff);
    }
    const std::vector<std::uint8_t> restored =
        PayloadCodec::decompress(ParameterValues::compress(data));
    if (restored != data) {
        std::cerr << "  payload_decompress_round_trip test : the data did not round-trip"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_payload_decompress_rejects_bad_input(void) {
    static const char *const name = "payload_decompress_rejects_bad_input";

    int result = expect_rejected(name, "empty data",
                                 "the compressed data is empty", []() {
        PayloadCodec::decompress(std::vector<std::uint8_t>());
    });
    if (result != Ok) {
        return result;
    }

    // Not a zlib stream at all.
    std::vector<std::uint8_t> garbage;
    for (int i = 0; i < 64; i++) {
        garbage.push_back((std::uint8_t)(i * 3 + 1));
    }
    result = expect_rejected(name, "garbage", "zlib decompression failed",
                             [&garbage]() {
        PayloadCodec::decompress(garbage);
    });
    if (result != Ok) {
        return result;
    }

    // A valid stream with its tail cut off must be reported rather than
    // returning the partial output or looping. Compressed outside the checked
    // call so that a failure to compress cannot be mistaken for the rejection
    // being tested -- the whole point of naming the expected message.
    std::vector<std::uint8_t> truncated =
        ParameterValues::compress(std::vector<std::uint8_t>(4096, 0x33));
    truncated.resize(truncated.size() / 2);
    return expect_rejected(name, "a truncated stream",
                           "the compressed data is truncated", [&truncated]() {
        PayloadCodec::decompress(truncated);
    });
}

static int test_parameter_values_from_json_round_trip(void) {
    // One set, several sets, and none, which are the three shapes to_json
    // writes.
    std::vector<std::vector<double>> single;
    single.push_back(std::vector<double>());
    single[0].push_back(0.5);
    single[0].push_back(-1.25);
    single[0].push_back(3.0);

    const auto single_back = ParameterValues::from_json(
        ParameterValues::to_json(single, 3));
    if (single_back != single) {
        std::cerr << "  parameter_values_from_json_round_trip test : one set did not "
            << "round-trip" << std::endl;
        return EqualityError;
    }

    std::vector<std::vector<double>> sweep;
    for (std::size_t i = 0; i < 4; i++) {
        std::vector<double> set;
        set.push_back((double)i);
        set.push_back((double)i * -0.5);
        sweep.push_back(set);
    }
    const auto sweep_back = ParameterValues::from_json(
        ParameterValues::to_json(sweep, 2));
    if (sweep_back != sweep) {
        std::cerr << "  parameter_values_from_json_round_trip test : the sweep did not "
            << "round-trip" << std::endl;
        return EqualityError;
    }

    const auto none_back = ParameterValues::from_json(
        ParameterValues::to_json(std::vector<std::vector<double>>(), 0));
    if (!none_back.empty()) {
        std::cerr << "  parameter_values_from_json_round_trip test : no sets came back as "
            << none_back.size() << " sets" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_parameter_values_from_json_special_values(void) {
    // The bit pattern has to survive, not just the approximate value, so the
    // byte order of le_to_double is actually pinned.
    std::vector<std::vector<double>> values;
    values.push_back(std::vector<double>());
    values[0].push_back(0.0);
    values[0].push_back(-0.0);
    values[0].push_back(1.0 / 3.0);
    values[0].push_back(-1.7976931348623157e308);
    values[0].push_back(5e-324); // the smallest subnormal

    const auto back = ParameterValues::from_json(
        ParameterValues::to_json(values, values[0].size()));
    if (back.size() != 1 || back[0].size() != values[0].size()) {
        std::cerr << "  parameter_values_from_json_special_values test : wrong shape"
            << std::endl;
        return EqualityError;
    }
    for (std::size_t i = 0; i < values[0].size(); i++) {
        // Compared as bits so that -0.0 is distinguished from 0.0.
        std::uint64_t expected = 0;
        std::uint64_t actual = 0;
        std::memcpy(&expected, &values[0][i], sizeof(expected));
        std::memcpy(&actual, &back[0][i], sizeof(actual));
        if (expected != actual) {
            std::cerr << "  parameter_values_from_json_special_values test : element "
                << i << " changed" << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_parameter_values_from_json_rejects_bad_input(void) {
    // Not an ndarray payload.
    nlohmann::ordered_json wrong_type;
    wrong_type["__type__"] = "QuantumCircuit";
    wrong_type["__value__"] = "";

    nlohmann::ordered_json missing_value;
    missing_value["__type__"] = "ndarray";

    struct Case {
        nlohmann::ordered_json input;
        const char *expected;
    };
    const Case cases[] = {
        {wrong_type, "\"__type__\" must be \"ndarray\""},
        {missing_value, "\"__value__\" must be a base64 string"},
        {nlohmann::ordered_json("a string"), "expected an object holding"},
        {nlohmann::ordered_json::array(), "expected an object holding"},
    };
    for (std::size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const nlohmann::ordered_json &input = cases[i].input;
        const int result = expect_rejected(
            "parameter_values_from_json_rejects_bad_input",
            "case " + std::to_string(i) + " (" + input.dump() + ")",
            cases[i].expected,
            [&input]() { ParameterValues::from_json(input); });
        if (result != Ok) {
            return result;
        }
    }
    return Ok;
}

static int test_parameter_values_from_npy_rejects_bad_dtype(void) {
    // A float32 array would otherwise be read as half as many doubles of
    // nonsense, so the dtype has to be checked rather than assumed.
    std::vector<std::uint8_t> npy =
        ParameterValues::to_npy(std::vector<double>(2, 1.0),
                                std::vector<std::size_t>(1, 2));
    const std::string header((const char *)npy.data() + 10, 20);
    const std::size_t at = header.find("<f8");
    if (at == std::string::npos) {
        std::cerr << "  parameter_values_from_npy_rejects_bad_dtype test : the test could "
            << "not find the dtype in the header" << std::endl;
        return EqualityError;
    }
    // Rewrite "<f8" as "<f4" in place, leaving the header length unchanged.
    npy[10 + at + 2] = '4';

    return expect_rejected("parameter_values_from_npy_rejects_bad_dtype",
                           "a float32 array",
                           "only \"<f8\" (little-endian double) can be read",
                           [&npy]() {
        std::vector<std::size_t> shape;
        ParameterValues::from_npy(npy, shape);
    });
}

static int test_pub_circuit_serialization_detects_form(void) {
    auto circ = make_bell_circuit();

    const nlohmann::ordered_json as_qasm3 =
        serialize_pub_circuit(circ, CircuitSerialization::qasm3);
    if (pub_circuit_serialization(as_qasm3) != CircuitSerialization::qasm3) {
        std::cerr << "  pub_circuit_serialization_detects_form test : an OpenQASM 3 "
            << "string was not detected" << std::endl;
        return EqualityError;
    }

#if QISKIT_CPP_HAS_QPY
    const nlohmann::ordered_json as_qpy =
        serialize_pub_circuit(circ, CircuitSerialization::qpy);
    if (pub_circuit_serialization(as_qpy) != CircuitSerialization::qpy) {
        std::cerr << "  pub_circuit_serialization_detects_form test : a QPY payload was "
            << "not detected" << std::endl;
        return EqualityError;
    }
#endif

    // Neither form.
    return expect_rejected("pub_circuit_serialization_detects_form", "a number",
                           "expected a QPY payload object or an OpenQASM 3 string",
                           []() {
        pub_circuit_serialization(nlohmann::ordered_json(42));
    });
}

static int test_deserialize_pub_circuit_selects_form(void) {
    // The caller chooses the form, mirroring serialize_pub_circuit, so a
    // circuit written one way can be read back by naming that way.
    auto circ = make_bell_circuit();

    const nlohmann::ordered_json as_qasm3 =
        serialize_pub_circuit(circ, CircuitSerialization::qasm3);
    auto from_qasm3 =
        deserialize_pub_circuit(as_qasm3, CircuitSerialization::qasm3);
    if (from_qasm3.num_qubits() != 2 || from_qasm3.num_clbits() != 2) {
        std::cerr << "  deserialize_pub_circuit_selects_form test : the OpenQASM 3 "
            << "circuit came back as " << from_qasm3.num_qubits() << " qubits and "
            << from_qasm3.num_clbits() << " clbits, expected 2 and 2" << std::endl;
        return EqualityError;
    }

#if QISKIT_CPP_HAS_QPY
    const nlohmann::ordered_json as_qpy =
        serialize_pub_circuit(circ, CircuitSerialization::qpy);
    auto from_qpy = deserialize_pub_circuit(as_qpy, CircuitSerialization::qpy);
    if (from_qpy.num_qubits() != 2 || from_qpy.num_clbits() != 2) {
        std::cerr << "  deserialize_pub_circuit_selects_form test : the QPY circuit came "
            << "back as " << from_qpy.num_qubits() << " qubits and "
            << from_qpy.num_clbits() << " clbits, expected 2 and 2" << std::endl;
        return EqualityError;
    }
#endif
    return Ok;
}

static int test_deserialize_pub_circuit_rejects_wrong_form(void) {
    // Selecting the form the payload is not in is reported rather than guessed
    // at. Before the serialization became an input this could not happen: the
    // form was taken from the payload, so a caller could not say what it
    // expected.
    auto circ = make_bell_circuit();

    static const char *const name = "deserialize_pub_circuit_rejects_wrong_form";

    const nlohmann::ordered_json as_qasm3 =
        serialize_pub_circuit(circ, CircuitSerialization::qasm3);
    // The fragments are kept short enough to sit inside a single string
    // literal in circuit_serialization.hpp. The messages there are split
    // across source lines, so a longer fragment would be correct at run time
    // but impossible to confirm by searching the source.
    const int result = expect_rejected(
        name, "an OpenQASM 3 string read as a QPY payload",
#if QISKIT_CPP_HAS_QPY
        "not an object",
#else
        "has no QPY support",
#endif
        [&as_qasm3]() {
            deserialize_pub_circuit(as_qasm3, CircuitSerialization::qpy);
        });
    if (result != Ok) {
        return result;
    }

#if QISKIT_CPP_HAS_QPY
    const nlohmann::ordered_json as_qpy =
        serialize_pub_circuit(circ, CircuitSerialization::qpy);
    return expect_rejected(
        name, "a QPY payload read as an OpenQASM 3 string", "not a string",
        [&as_qpy]() {
            deserialize_pub_circuit(as_qpy, CircuitSerialization::qasm3);
        });
#else
    return Ok;
#endif
}

static int test_sampler_pub_from_json_ignores_target_serialization(void) {
    // from_json takes the form from the payload, not from the pub it is read
    // into, so reading an OpenQASM 3 pub into a pub left on the QPY default
    // still works.
    auto circ = make_bell_circuit();
    auto source = SamplerPub(circ, 64);
    source.set_serialization(CircuitSerialization::qasm3);

    SamplerPub target(circ, 1);
    target.set_serialization(CircuitSerialization::qpy);
    target.from_json(source.to_json());

    if (target.serialization() != CircuitSerialization::qasm3) {
        std::cerr << "  sampler_pub_from_json_ignores_target_serialization test : the "
            << "form was not taken from the payload" << std::endl;
        return EqualityError;
    }
    if (target.shots() != 64) {
        std::cerr << "  sampler_pub_from_json_ignores_target_serialization test : shots "
            << "came back as " << target.shots() << ", expected 64" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_pub_from_json_round_trip(void) {
    auto circ = make_bell_circuit();
    auto pub = SamplerPub(circ, 128);

    SamplerPub restored;
    restored.from_json(pub.to_json());

    if (restored.shots() != 128) {
        std::cerr << "  sampler_pub_from_json_round_trip test : shots came back as "
            << restored.shots() << ", expected 128" << std::endl;
        return EqualityError;
    }
    if (restored.serialization() != pub.serialization()) {
        std::cerr << "  sampler_pub_from_json_round_trip test : the serialization changed"
            << std::endl;
        return EqualityError;
    }
    if (restored.circuit().num_qubits() != 2 || restored.circuit().num_clbits() != 2) {
        std::cerr << "  sampler_pub_from_json_round_trip test : the circuit came back as "
            << restored.circuit().num_qubits() << " qubits and "
            << restored.circuit().num_clbits() << " clbits, expected 2 and 2"
            << std::endl;
        return EqualityError;
    }
    // Re-serializing must give the same payload, which is the strongest
    // statement that nothing was lost.
    if (restored.to_json() != pub.to_json()) {
        std::cerr << "  sampler_pub_from_json_round_trip test : the pub did not "
            << "re-serialize identically" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_pub_from_json_no_shots(void) {
    // to_json omits the third element for a zero shot count, so from_json has
    // to accept a two-element pub.
    auto circ = make_bell_circuit();
    auto pub = SamplerPub(circ);

    const auto payload = pub.to_json();
    if (payload.size() != 2) {
        std::cerr << "  sampler_pub_from_json_no_shots test : the pub has "
            << payload.size() << " elements, expected 2" << std::endl;
        return EqualityError;
    }

    SamplerPub restored;
    restored.from_json(payload);
    if (restored.shots() != 0) {
        std::cerr << "  sampler_pub_from_json_no_shots test : shots came back as "
            << restored.shots() << ", expected 0" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_pub_from_json_parameterized(void) {
    auto circ = QuantumCircuit(2, 2);
    auto theta = Parameter("theta");
    auto phi = Parameter("phi");
    circ.rx(theta, 0);
    circ.rz(phi, 1);
    circ.measure(0, 0);

    std::vector<double> values;
    values.push_back(0.5);
    values.push_back(1.5);
    auto pub = SamplerPub(circ, values, 64);

    SamplerPub restored;
    restored.from_json(pub.to_json());

    if (restored.num_parameter_sets() != 1) {
        std::cerr << "  sampler_pub_from_json_parameterized test : "
            << restored.num_parameter_sets() << " sets, expected 1" << std::endl;
        return EqualityError;
    }
    if (restored.parameter_values() != pub.parameter_values()) {
        std::cerr << "  sampler_pub_from_json_parameterized test : the values changed"
            << std::endl;
        return EqualityError;
    }
    if (restored.circuit().num_parameters() != 2) {
        std::cerr << "  sampler_pub_from_json_parameterized test : the circuit came back "
            << "with " << restored.circuit().num_parameters()
            << " parameters, expected 2" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_pub_from_json_sweep(void) {
    auto circ = QuantumCircuit(1, 1);
    auto theta = Parameter("theta");
    circ.rx(theta, 0);
    circ.measure(0, 0);

    std::vector<std::vector<double>> sweep;
    for (std::size_t i = 0; i < 3; i++) {
        sweep.push_back(std::vector<double>(1, (double)i * 0.25));
    }
    auto pub = SamplerPub(circ, sweep, 32);

    SamplerPub restored;
    restored.from_json(pub.to_json());

    if (restored.num_parameter_sets() != 3) {
        std::cerr << "  sampler_pub_from_json_sweep test : "
            << restored.num_parameter_sets() << " sets, expected 3" << std::endl;
        return EqualityError;
    }
    if (restored.parameter_values() != sweep) {
        std::cerr << "  sampler_pub_from_json_sweep test : the sweep changed" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_pub_from_json_qasm3(void) {
    // The circuit form is taken from the JSON, so an OpenQASM 3 pub reads back
    // as OpenQASM 3 without being told.
    auto circ = make_bell_circuit();
    auto pub = SamplerPub(circ, 64);
    pub.set_serialization(CircuitSerialization::qasm3);

    SamplerPub restored;
    restored.from_json(pub.to_json());

    if (restored.serialization() != CircuitSerialization::qasm3) {
        std::cerr << "  sampler_pub_from_json_qasm3 test : the serialization was not "
            << "recovered as OpenQASM 3" << std::endl;
        return EqualityError;
    }
    if (!restored.to_json()[0].is_string()) {
        std::cerr << "  sampler_pub_from_json_qasm3 test : the re-serialized circuit is "
            << "not a string" << std::endl;
        return EqualityError;
    }
    if (restored.shots() != 64) {
        std::cerr << "  sampler_pub_from_json_qasm3 test : shots came back as "
            << restored.shots() << ", expected 64" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_pub_from_json_static(void) {
    auto circ = make_bell_circuit();
    auto pub = SamplerPub(circ, 100);

    const auto restored = SamplerPub::from_pub_json(pub.to_json());
    if (restored.shots() != 100) {
        std::cerr << "  sampler_pub_from_json_static test : shots came back as "
            << restored.shots() << ", expected 100" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_pub_from_json_rejects_bad_input(void) {
    auto circ = make_bell_circuit();
    auto pub = SamplerPub(circ, 8);
    const auto good = pub.to_json();

    // Each case names the rejection it must trigger. Without that a case could
    // pass by tripping a different check, or by the fixture above throwing,
    // and the test would prove nothing.
    struct Case {
        nlohmann::ordered_json input;
        const char *expected;
    };
    std::vector<Case> cases;

    // Not an array. Checked before any indexing: nlohmann's const operator[]
    // with a numeric index throws its own type_error for a non-array, and
    // reads out of bounds for an array that is too short, so from_json has to
    // reject the shape itself before it looks at any element.
    cases.push_back(Case{nlohmann::ordered_json::object(),
                         "a pub must be a JSON array"});
    cases.push_back(Case{nlohmann::ordered_json("a string"),
                         "a pub must be a JSON array"});
    // Too few and too many elements.
    cases.push_back(Case{nlohmann::ordered_json::array(),
                         "a pub must hold 2 or 3 elements"});
    {
        nlohmann::ordered_json one = nlohmann::ordered_json::array();
        one.push_back(good[0]);
        cases.push_back(Case{one, "a pub must hold 2 or 3 elements"});
    }
    {
        nlohmann::ordered_json four = good;
        four.push_back(1);
        cases.push_back(Case{four, "a pub must hold 2 or 3 elements"});
    }
    // A circuit element that is neither a payload object nor a string.
    {
        nlohmann::ordered_json bad_circuit = good;
        bad_circuit[0] = 42;
        cases.push_back(Case{bad_circuit, "the circuit was to be read as"});
    }
    // A negative shot count.
    {
        nlohmann::ordered_json bad_shots = good;
        bad_shots[2] = -1;
        cases.push_back(Case{bad_shots,
                             "shot count must be a non-negative whole number"});
    }

    for (std::size_t i = 0; i < cases.size(); i++) {
        const nlohmann::ordered_json &input = cases[i].input;
        const int result = expect_rejected(
            "sampler_pub_from_json_rejects_bad_input",
            "case " + std::to_string(i), cases[i].expected, [&input]() {
                SamplerPub target;
                target.from_json(input);
            });
        if (result != Ok) {
            return result;
        }
    }
    return Ok;
}

static int test_sampler_pub_mismatched_values_rejected_on_write(void) {
    // A pub whose values do not match its own circuit is caught on the way
    // out, not on the way in: from_json reads the values without checking them
    // against the circuit, and to_json rechecks the count before anything is
    // submitted. This pins where the check lives, so that moving it is a
    // deliberate change rather than a silent one.
    //
    // Built by splicing a two-parameter value array onto a one-parameter
    // circuit.
    auto one_param = QuantumCircuit(1, 1);
    auto theta = Parameter("theta");
    one_param.rx(theta, 0);
    auto pub = SamplerPub(one_param, std::vector<double>(1, 0.5), 16);
    nlohmann::ordered_json spliced = pub.to_json();

    std::vector<std::vector<double>> two_values;
    two_values.push_back(std::vector<double>(2, 0.25));
    spliced[1] = ParameterValues::to_json(two_values, 2);

    SamplerPub target;
    const int read = expect_no_throw(
        "sampler_pub_mismatched_values_rejected_on_write",
        "reading a pub whose values do not match its circuit",
        [&target, &spliced]() { target.from_json(spliced); });
    if (read != Ok) {
        return read;
    }
    if (target.num_parameter_sets() != 1
        || target.parameter_values()[0].size() != 2) {
        std::cerr << "  sampler_pub_mismatched_values_rejected_on_write test : the "
            << "values were not read as given" << std::endl;
        return EqualityError;
    }

    return expect_rejected("sampler_pub_mismatched_values_rejected_on_write",
                           "re-serializing it",
                           "holds 2 values but the circuit has 1 parameters",
                           [&target]() { target.to_json(); });
}

static int test_sampler_pub_from_json_leaves_target_intact(void) {
    // A pub rejected for its circuit leaves the pub it was read into alone,
    // because the circuit is the first thing read and nothing is assigned
    // until it succeeds.
    //
    // Note this is not atomicity in general: from_json assigns the circuit
    // before it reads the parameter values, so a pub rejected for its *values*
    // does leave the new circuit in place. Only the first-element case is
    // claimed here, which is the one the implementation guarantees.
    auto circ = make_bell_circuit();
    SamplerPub target(circ, 256);

    nlohmann::ordered_json broken = nlohmann::ordered_json::array();
    broken.push_back(42); // not a circuit
    broken.push_back(nlohmann::ordered_json());

    // The rejection is expected; what this test is about is the state of
    // `target` afterwards.
    const int rejected = expect_rejected(
        "sampler_pub_from_json_leaves_target_intact", "a pub with no circuit",
        "the circuit was to be read as",
        [&target, &broken]() { target.from_json(broken); });
    if (rejected != Ok) {
        return rejected;
    }

    if (target.shots() != 256) {
        std::cerr << "  sampler_pub_from_json_leaves_target_intact test : shots became "
            << target.shots() << ", expected the original 256" << std::endl;
        return EqualityError;
    }
    if (target.circuit().num_qubits() != 2) {
        std::cerr << "  sampler_pub_from_json_leaves_target_intact test : the circuit was "
            << "replaced" << std::endl;
        return EqualityError;
    }
    return Ok;
}

// ---------------------------------------------------------------------------
// EstimatorPub::from_json
// ---------------------------------------------------------------------------

static int test_estimator_pub_from_json_round_trip(void) {
    auto circ = QuantumCircuit(2, 0);
    std::vector<std::pair<std::string, double>> terms;
    terms.push_back(std::make_pair(std::string("ZI"), 1.0));
    terms.push_back(std::make_pair(std::string("IZ"), -0.5));
    auto pub = EstimatorPub(circ, terms, 0.02);

    EstimatorPub restored;
    restored.from_json(pub.to_json());

    if (!restored.has_precision()
        || std::abs(restored.precision() - 0.02) > 1e-12) {
        std::cerr << "  estimator_pub_from_json_round_trip test : precision came back as "
            << restored.precision() << " (set: " << restored.has_precision()
            << "), expected 0.02" << std::endl;
        return EqualityError;
    }
    if (restored.observables() != pub.observables()) {
        std::cerr << "  estimator_pub_from_json_round_trip test : the observables changed:\n"
            << restored.observables().dump() << std::endl;
        return EqualityError;
    }
    if (restored.serialization() != pub.serialization()) {
        std::cerr << "  estimator_pub_from_json_round_trip test : the serialization changed"
            << std::endl;
        return EqualityError;
    }
    if (restored.circuit().num_qubits() != 2) {
        std::cerr << "  estimator_pub_from_json_round_trip test : the circuit came back "
            << "as " << restored.circuit().num_qubits() << " qubits, expected 2"
            << std::endl;
        return EqualityError;
    }
    // Re-serializing must give the same payload, which is the strongest
    // statement that nothing was lost.
    if (restored.to_json() != pub.to_json()) {
        std::cerr << "  estimator_pub_from_json_round_trip test : the pub did not "
            << "re-serialize identically" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_pub_from_json_no_precision(void) {
    // to_json omits the fourth element when no precision was set, so from_json
    // has to accept a three-element pub.
    auto circ = QuantumCircuit(1, 0);
    auto pub = EstimatorPub(circ, std::string("Z"));

    const auto payload = pub.to_json();
    if (payload.size() != 3) {
        std::cerr << "  estimator_pub_from_json_no_precision test : the pub has "
            << payload.size() << " elements, expected 3" << std::endl;
        return EqualityError;
    }

    EstimatorPub restored;
    restored.from_json(payload);
    if (restored.has_precision()) {
        std::cerr << "  estimator_pub_from_json_no_precision test : a precision was "
            << "invented" << std::endl;
        return EqualityError;
    }
    if (restored.to_json().size() != 3) {
        std::cerr << "  estimator_pub_from_json_no_precision test : the re-serialized pub "
            << "has " << restored.to_json().size() << " elements, expected 3" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_pub_from_json_null_values(void) {
    // An unparameterized circuit carries a null third element, which must come
    // back as no binding sets rather than as an error.
    auto circ = QuantumCircuit(1, 0);
    auto pub = EstimatorPub(circ, std::string("Z"), 0.01);

    const auto payload = pub.to_json();
    if (!payload[2].is_null()) {
        std::cerr << "  estimator_pub_from_json_null_values test : the values element is "
            << "not null" << std::endl;
        return EqualityError;
    }

    EstimatorPub restored;
    restored.from_json(payload);
    if (restored.num_parameter_sets() != 0) {
        std::cerr << "  estimator_pub_from_json_null_values test : "
            << restored.num_parameter_sets() << " sets, expected 0" << std::endl;
        return EqualityError;
    }
    if (!restored.parameter_values().is_null()) {
        std::cerr << "  estimator_pub_from_json_null_values test : the values payload is "
            << "no longer null" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_pub_from_json_parameterized(void) {
    auto circ = QuantumCircuit(1, 0);
    auto theta = Parameter("theta");
    circ.rx(theta, 0);
    auto pub = EstimatorPub(circ, std::string("Z"), 0.05);
    pub.set_parameter_values(std::vector<double>(1, 0.75));

    EstimatorPub restored;
    restored.from_json(pub.to_json());

    if (restored.num_parameter_sets() != 1) {
        std::cerr << "  estimator_pub_from_json_parameterized test : "
            << restored.num_parameter_sets() << " sets, expected 1" << std::endl;
        return EqualityError;
    }
    if (restored.circuit().num_parameters() != 1) {
        std::cerr << "  estimator_pub_from_json_parameterized test : the circuit came "
            << "back with " << restored.circuit().num_parameters()
            << " parameters, expected 1" << std::endl;
        return EqualityError;
    }
    // The payload is kept as it arrived, so it must be byte-identical.
    if (restored.parameter_values() != pub.parameter_values()) {
        std::cerr << "  estimator_pub_from_json_parameterized test : the values payload "
            << "changed" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_pub_from_json_sweep(void) {
    // num_parameter_sets cannot be read off the encoded payload, so recovering
    // it is the part worth pinning: a result needs it to split the values.
    auto circ = QuantumCircuit(1, 0);
    auto theta = Parameter("theta");
    circ.rx(theta, 0);
    auto pub = EstimatorPub(circ, std::string("Z"));

    std::vector<std::vector<double>> sweep;
    for (std::size_t i = 0; i < 5; i++) {
        sweep.push_back(std::vector<double>(1, (double)i * 0.125));
    }
    pub.set_parameter_values(sweep);

    EstimatorPub restored;
    restored.from_json(pub.to_json());

    if (restored.num_parameter_sets() != 5) {
        std::cerr << "  estimator_pub_from_json_sweep test : "
            << restored.num_parameter_sets() << " sets, expected 5" << std::endl;
        return EqualityError;
    }
    // And the values themselves must decode back to the sweep that went in.
    const auto decoded = ParameterValues::from_json(restored.parameter_values());
    if (decoded != sweep) {
        std::cerr << "  estimator_pub_from_json_sweep test : the sweep changed" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_pub_from_json_observable_forms(void) {
    // The three observable shapes the schema allows: a bare label, a map of
    // labels to coefficients, and an array of either.
    auto circ = QuantumCircuit(2, 0);

    nlohmann::ordered_json map_form;
    map_form["ZI"] = 1.0;
    map_form["IZ"] = -0.5;

    nlohmann::ordered_json array_form = nlohmann::ordered_json::array();
    array_form.push_back("ZZ");
    array_form.push_back(map_form);

    const nlohmann::ordered_json forms[] = {
        nlohmann::ordered_json("XY"), map_form, array_form,
    };

    for (std::size_t i = 0; i < sizeof(forms) / sizeof(forms[0]); i++) {
        auto pub = EstimatorPub(circ, forms[i]);
        EstimatorPub restored;
        restored.from_json(pub.to_json());

        if (restored.observables() != forms[i]) {
            std::cerr << "  estimator_pub_from_json_observable_forms test : form " << i
                << " changed:\n    expected: " << forms[i].dump()
                << "\n    actual:   " << restored.observables().dump() << std::endl;
            return EqualityError;
        }
        if (restored.num_observables() != pub.num_observables()) {
            std::cerr << "  estimator_pub_from_json_observable_forms test : form " << i
                << " came back with " << restored.num_observables()
                << " observables, expected " << pub.num_observables() << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_estimator_pub_from_json_qasm3(void) {
    auto circ = QuantumCircuit(2, 0);
    auto pub = EstimatorPub(circ, std::string("ZI"), 0.01);
    pub.set_serialization(CircuitSerialization::qasm3);

    EstimatorPub restored;
    restored.from_json(pub.to_json());

    if (restored.serialization() != CircuitSerialization::qasm3) {
        std::cerr << "  estimator_pub_from_json_qasm3 test : the serialization was not "
            << "recovered as OpenQASM 3" << std::endl;
        return EqualityError;
    }
    if (!restored.to_json()[0].is_string()) {
        std::cerr << "  estimator_pub_from_json_qasm3 test : the re-serialized circuit is "
            << "not a string" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_pub_from_json_static(void) {
    auto circ = QuantumCircuit(1, 0);
    auto pub = EstimatorPub(circ, std::string("Z"), 0.25);

    const auto restored = EstimatorPub::from_pub_json(pub.to_json());
    if (!restored.has_precision()
        || std::abs(restored.precision() - 0.25) > 1e-12) {
        std::cerr << "  estimator_pub_from_json_static test : precision came back as "
            << restored.precision() << ", expected 0.25" << std::endl;
        return EqualityError;
    }
    if (restored.num_observables() != 1) {
        std::cerr << "  estimator_pub_from_json_static test : "
            << restored.num_observables() << " observables, expected 1" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_pub_from_json_rejects_bad_input(void) {
    auto circ = QuantumCircuit(1, 0);
    auto pub = EstimatorPub(circ, std::string("Z"), 0.1);
    const auto good = pub.to_json();

    // Each case names the rejection it must trigger, so a case cannot pass by
    // tripping one of the other checks -- or by the fixture above throwing,
    // which would otherwise look identical and make all eleven cases vacuous.
    struct Case {
        nlohmann::ordered_json input;
        const char *expected;
    };
    std::vector<Case> cases;

    // Not an array.
    cases.push_back(Case{nlohmann::ordered_json::object(),
                         "a pub must be a JSON array"});
    cases.push_back(Case{nlohmann::ordered_json("a string"),
                         "a pub must be a JSON array"});
    // Too few and too many elements.
    cases.push_back(Case{nlohmann::ordered_json::array(),
                         "a pub must hold 3 or 4 elements"});
    {
        nlohmann::ordered_json two = nlohmann::ordered_json::array();
        two.push_back(good[0]);
        two.push_back(good[1]);
        cases.push_back(Case{two, "a pub must hold 3 or 4 elements"});
    }
    {
        nlohmann::ordered_json five = good;
        five.push_back(1);
        cases.push_back(Case{five, "a pub must hold 3 or 4 elements"});
    }
    // A circuit element that is neither a payload object nor a string.
    {
        nlohmann::ordered_json bad_circuit = good;
        bad_circuit[0] = 42;
        cases.push_back(Case{bad_circuit,
                             "expected a QPY payload object or an OpenQASM 3 string"});
    }
    // Observables that are not a label, map, or array of those.
    {
        nlohmann::ordered_json bad_observable = good;
        bad_observable[1] = 7;
        cases.push_back(Case{bad_observable,
                             "must be a Pauli label, Pauli map, or array of observables"});
    }
    // An observable whose width does not match the circuit.
    {
        nlohmann::ordered_json wrong_width = good;
        wrong_width[1] = "ZZZ";
        cases.push_back(Case{wrong_width,
                             "observable width must match the circuit width"});
    }
    // An observable outside the Pauli alphabet.
    {
        nlohmann::ordered_json bad_pauli = good;
        bad_pauli[1] = "A";
        cases.push_back(Case{bad_pauli,
                             "only support Pauli labels containing I, X, Y, or Z"});
    }
    // A negative precision.
    {
        nlohmann::ordered_json bad_precision = good;
        bad_precision[3] = -0.5;
        cases.push_back(Case{bad_precision,
                             "precision must be finite and non-negative"});
    }
    // A precision that is not a number.
    {
        nlohmann::ordered_json bad_precision = good;
        bad_precision[3] = "fine";
        cases.push_back(Case{bad_precision, "the precision must be a number"});
    }

    for (std::size_t i = 0; i < cases.size(); i++) {
        const nlohmann::ordered_json &input = cases[i].input;
        const int result = expect_rejected(
            "estimator_pub_from_json_rejects_bad_input",
            "case " + std::to_string(i), cases[i].expected, [&input]() {
                EstimatorPub target;
                target.from_json(input);
            });
        if (result != Ok) {
            return result;
        }
    }
    return Ok;
}

static int test_estimator_pub_from_json_rejects_mismatched_values(void) {
    // Values that do not match the circuit's parameter count are rejected
    // where they are read, not at submission.
    auto circ = QuantumCircuit(1, 0);
    auto theta = Parameter("theta");
    circ.rx(theta, 0);
    auto pub = EstimatorPub(circ, std::string("Z"));
    pub.set_parameter_values(std::vector<double>(1, 0.5));

    nlohmann::ordered_json spliced = pub.to_json();
    std::vector<std::vector<double>> two_values;
    two_values.push_back(std::vector<double>(2, 0.25));
    spliced[2] = ParameterValues::to_json(two_values, 2);

    return expect_rejected(
        "estimator_pub_from_json_rejects_mismatched_values",
        "two values for a one-parameter circuit",
        "holds 2 values but the circuit has 1 parameters", [&spliced]() {
            EstimatorPub target;
            target.from_json(spliced);
        });
}

static int test_estimator_pub_from_json_leaves_target_intact(void) {
    // A failed read must not half-overwrite the pub it was called on.
    auto circ = QuantumCircuit(2, 0);
    EstimatorPub target(circ, std::string("ZI"), 0.125);

    nlohmann::ordered_json broken = nlohmann::ordered_json::array();
    broken.push_back(42); // not a circuit
    broken.push_back("ZI");
    broken.push_back(nlohmann::ordered_json());

    // The rejection is expected; what this test is about is the state of
    // `target` afterwards.
    const int rejected = expect_rejected(
        "estimator_pub_from_json_leaves_target_intact", "a pub with no circuit",
        "expected a QPY payload object or an OpenQASM 3 string",
        [&target, &broken]() { target.from_json(broken); });
    if (rejected != Ok) {
        return rejected;
    }

    if (!target.has_precision() || std::abs(target.precision() - 0.125) > 1e-12) {
        std::cerr << "  estimator_pub_from_json_leaves_target_intact test : precision "
            << "became " << target.precision() << ", expected the original 0.125"
            << std::endl;
        return EqualityError;
    }
    if (target.observables() != nlohmann::ordered_json("ZI")) {
        std::cerr << "  estimator_pub_from_json_leaves_target_intact test : the "
            << "observables were replaced" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_pub_from_json_accepts_missing_values(void) {
    // A parameterized circuit with no values is a legal pub state, the same as
    // it is straight from a constructor, so reading one must be allowed. It is
    // to_json and submission that require the values.
    auto circ = QuantumCircuit(1, 0);
    auto theta = Parameter("theta");
    circ.rx(theta, 0);

    // Built by hand, because to_json would refuse to write this.
    nlohmann::ordered_json payload = nlohmann::ordered_json::array();
    payload.push_back(serialize_pub_circuit(circ, default_circuit_serialization()));
    payload.push_back("Z");
    payload.push_back(nlohmann::ordered_json());

    EstimatorPub restored;
    restored.from_json(payload);

    if (restored.num_parameter_sets() != 0) {
        std::cerr << "  estimator_pub_from_json_accepts_missing_values test : "
            << restored.num_parameter_sets() << " sets, expected 0" << std::endl;
        return EqualityError;
    }
    // And it must still refuse to be submitted, which is where the missing
    // values are caught.
    std::string message;
    if (restored.validate_for_submission(&message)) {
        std::cerr << "  estimator_pub_from_json_accepts_missing_values test : the pub "
            << "claims it can be submitted with no values" << std::endl;
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

    num_failed += RUN_TEST(test_payload_base64_decode_round_trip);
    num_failed += RUN_TEST(test_payload_base64_decode_known_vectors);
    num_failed += RUN_TEST(test_payload_base64_decode_rejects_bad_input);
    num_failed += RUN_TEST(test_payload_decompress_round_trip);
    num_failed += RUN_TEST(test_payload_decompress_rejects_bad_input);
    num_failed += RUN_TEST(test_parameter_values_from_json_round_trip);
    num_failed += RUN_TEST(test_parameter_values_from_json_special_values);
    num_failed += RUN_TEST(test_parameter_values_from_json_rejects_bad_input);
    num_failed += RUN_TEST(test_parameter_values_from_npy_rejects_bad_dtype);
    num_failed += RUN_TEST(test_pub_circuit_serialization_detects_form);
    num_failed += RUN_TEST(test_deserialize_pub_circuit_selects_form);
    num_failed += RUN_TEST(test_deserialize_pub_circuit_rejects_wrong_form);
    num_failed += RUN_TEST_EXCEPTION(test_sampler_pub_from_json_ignores_target_serialization);
    num_failed += RUN_TEST(test_sampler_pub_from_json_round_trip);
    num_failed += RUN_TEST(test_sampler_pub_from_json_no_shots);
    num_failed += RUN_TEST(test_sampler_pub_from_json_parameterized);
    num_failed += RUN_TEST(test_sampler_pub_from_json_sweep);
    num_failed += RUN_TEST_EXCEPTION(test_sampler_pub_from_json_qasm3);
    num_failed += RUN_TEST(test_sampler_pub_from_json_static);
    num_failed += RUN_TEST(test_sampler_pub_from_json_rejects_bad_input);
    num_failed += RUN_TEST(test_sampler_pub_mismatched_values_rejected_on_write);
    num_failed += RUN_TEST(test_sampler_pub_from_json_leaves_target_intact);

    num_failed += RUN_TEST(test_estimator_pub_from_json_round_trip);
    num_failed += RUN_TEST(test_estimator_pub_from_json_no_precision);
    num_failed += RUN_TEST(test_estimator_pub_from_json_null_values);
    num_failed += RUN_TEST(test_estimator_pub_from_json_parameterized);
    num_failed += RUN_TEST(test_estimator_pub_from_json_sweep);
    num_failed += RUN_TEST(test_estimator_pub_from_json_observable_forms);
    num_failed += RUN_TEST(test_estimator_pub_from_json_qasm3);
    num_failed += RUN_TEST(test_estimator_pub_from_json_static);
    num_failed += RUN_TEST(test_estimator_pub_from_json_rejects_bad_input);
    num_failed += RUN_TEST(test_estimator_pub_from_json_rejects_mismatched_values);
    num_failed += RUN_TEST(test_estimator_pub_from_json_leaves_target_intact);
    num_failed += RUN_TEST(test_estimator_pub_from_json_accepts_missing_values);

    std::cerr << "=== Number of failed subtests: " << num_failed << std::endl;
    return num_failed;
}
