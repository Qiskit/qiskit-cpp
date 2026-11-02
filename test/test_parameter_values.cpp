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
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <zlib.h>

#include "common.hpp"

#include "circuit/quantumcircuit.hpp"
#include "primitives/containers/estimator_pub.hpp"
#include "primitives/containers/parameter_values.hpp"
#include "primitives/containers/sampler_pub.hpp"

using namespace Qiskit;
using namespace Qiskit::circuit;
using namespace Qiskit::primitives;

// Decode standard base64, so the payload can be checked the way the service
// reads it.
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
    for (std::size_t i = 0; i < encoded.size(); i += 4) {
        int values[4];
        std::size_t padding = 0;
        for (std::size_t j = 0; j < 4; j++) {
            const char ch = encoded[i + j];
            if (ch == '=') {
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

// Walk a parameter-values payload back to the .npy bytes it encodes.
static bool decode_payload(const nlohmann::ordered_json &payload,
                           std::vector<std::uint8_t> &npy) {
    if (!payload.is_object() || !payload.contains("__type__")
        || payload["__type__"] != "ndarray" || !payload.contains("__value__")
        || !payload["__value__"].is_string()) {
        return false;
    }
    std::vector<std::uint8_t> compressed;
    if (!base64_decode(payload["__value__"].get<std::string>(), compressed)) {
        return false;
    }
    return zlib_decompress(compressed, npy);
}

// Parse a .npy v1.0 buffer holding a little-endian f64 array.
static bool parse_npy(const std::vector<std::uint8_t> &npy, std::string &header,
                      std::vector<double> &data) {
    const char magic[] = {'\x93', 'N', 'U', 'M', 'P', 'Y'};
    if (npy.size() < 10) {
        return false;
    }
    for (std::size_t i = 0; i < sizeof(magic); i++) {
        if (npy[i] != (std::uint8_t)magic[i]) {
            return false;
        }
    }
    if (npy[6] != 1 || npy[7] != 0) {
        return false;
    }
    const std::size_t header_len = (std::size_t)npy[8] | ((std::size_t)npy[9] << 8);
    if (10 + header_len > npy.size()) {
        return false;
    }
    // The whole prefix must be 64-byte aligned, and the header must end with a
    // newline, as NumPy's reader requires.
    if ((10 + header_len) % 64 != 0 || npy[10 + header_len - 1] != '\n') {
        return false;
    }
    header.assign((const char *)&npy[10], header_len);

    const std::size_t payload = npy.size() - (10 + header_len);
    if (payload % sizeof(double) != 0) {
        return false;
    }
    data.resize(payload / sizeof(double));
    for (std::size_t i = 0; i < data.size(); i++) {
        std::uint64_t bits = 0;
        for (std::size_t b = 0; b < 8; b++) {
            bits |= (std::uint64_t)npy[10 + header_len + i * 8 + b] << (8 * b);
        }
        std::memcpy(&data[i], &bits, sizeof(double));
    }
    return true;
}

static QuantumCircuit make_parameterized_circuit(void) {
    auto circ = QuantumCircuit(1, 1);
    auto theta = Parameter("theta");
    auto phi = Parameter("phi");
    circ.rx(theta, 0);
    circ.rz(phi, 0);
    circ.measure(0, 0);
    return circ;
}

static int test_npy_single_set(void) {
    std::vector<double> values;
    values.push_back(0.5);
    values.push_back(-1.25);

    const auto payload = ParameterValues::to_json(values, 2);
    std::vector<std::uint8_t> npy;
    if (!decode_payload(payload, npy)) {
        std::cerr << "  npy_single_set test : the payload could not be decoded" << std::endl;
        return EqualityError;
    }

    std::string header;
    std::vector<double> decoded;
    if (!parse_npy(npy, header, decoded)) {
        std::cerr << "  npy_single_set test : the .npy buffer is malformed" << std::endl;
        return EqualityError;
    }
    // A single binding set is a 1-D array, which NumPy writes with a trailing
    // comma so the shape stays a tuple.
    if (header.find("'descr': '<f8'") == std::string::npos
        || header.find("'fortran_order': False") == std::string::npos
        || header.find("'shape': (2,)") == std::string::npos) {
        std::cerr << "  npy_single_set test : unexpected header \"" << header << "\""
            << std::endl;
        return EqualityError;
    }
    if (decoded.size() != 2 || decoded[0] != 0.5 || decoded[1] != -1.25) {
        std::cerr << "  npy_single_set test : the values did not round-trip" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_npy_sweep(void) {
    std::vector<std::vector<double>> sets;
    std::vector<double> a;
    a.push_back(1.0);
    a.push_back(2.0);
    std::vector<double> b;
    b.push_back(3.0);
    b.push_back(4.0);
    std::vector<double> c;
    c.push_back(5.0);
    c.push_back(6.0);
    sets.push_back(a);
    sets.push_back(b);
    sets.push_back(c);

    const auto payload = ParameterValues::to_json(sets, 2);
    std::vector<std::uint8_t> npy;
    std::string header;
    std::vector<double> decoded;
    if (!decode_payload(payload, npy) || !parse_npy(npy, header, decoded)) {
        std::cerr << "  npy_sweep test : the payload could not be decoded" << std::endl;
        return EqualityError;
    }
    // A sweep is 2-D: the leading dimension indexes the set, the last indexes
    // the parameter.
    if (header.find("'shape': (3, 2)") == std::string::npos) {
        std::cerr << "  npy_sweep test : unexpected header \"" << header << "\"" << std::endl;
        return EqualityError;
    }
    // Flattened in C order, so set 0 comes before set 1.
    const double expected[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    if (decoded.size() != 6) {
        std::cerr << "  npy_sweep test : " << decoded.size()
            << " values, expected 6" << std::endl;
        return EqualityError;
    }
    for (std::size_t i = 0; i < 6; i++) {
        if (decoded[i] != expected[i]) {
            std::cerr << "  npy_sweep test : element " << i << " is " << decoded[i]
                << ", expected " << expected[i] << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_npy_empty(void) {
    // An unparameterized circuit still needs a well-formed empty array, which
    // is what the schema's own fixture uses.
    const auto payload = ParameterValues::to_json(std::vector<std::vector<double>>(), 0);
    std::vector<std::uint8_t> npy;
    std::string header;
    std::vector<double> decoded;
    if (!decode_payload(payload, npy) || !parse_npy(npy, header, decoded)) {
        std::cerr << "  npy_empty test : the payload could not be decoded" << std::endl;
        return EqualityError;
    }
    if (header.find("'shape': (0,)") == std::string::npos) {
        std::cerr << "  npy_empty test : unexpected header \"" << header << "\"" << std::endl;
        return EqualityError;
    }
    if (!decoded.empty()) {
        std::cerr << "  npy_empty test : " << decoded.size()
            << " values, expected none" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_npy_alignment(void) {
    // The header pad has to keep the prefix 64-byte aligned for every shape, so
    // a few sizes are checked rather than just one.
    for (std::size_t n = 1; n <= 40; n++) {
        std::vector<double> values(n, 1.0);
        const auto payload = ParameterValues::to_json(values, (uint_t)n);
        std::vector<std::uint8_t> npy;
        std::string header;
        std::vector<double> decoded;
        if (!decode_payload(payload, npy) || !parse_npy(npy, header, decoded)) {
            std::cerr << "  npy_alignment test : a " << n
                << "-element array did not decode" << std::endl;
            return EqualityError;
        }
        if (decoded.size() != n) {
            std::cerr << "  npy_alignment test : a " << n << "-element array decoded to "
                << decoded.size() << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_npy_special_values(void) {
    std::vector<double> values;
    values.push_back(0.0);
    values.push_back(-0.0);
    values.push_back(3.141592653589793);
    values.push_back(1e300);
    values.push_back(-1e-300);

    const auto payload = ParameterValues::to_json(values, 5);
    std::vector<std::uint8_t> npy;
    std::string header;
    std::vector<double> decoded;
    if (!decode_payload(payload, npy) || !parse_npy(npy, header, decoded)) {
        std::cerr << "  npy_special_values test : the payload could not be decoded"
            << std::endl;
        return EqualityError;
    }
    if (decoded.size() != values.size()) {
        std::cerr << "  npy_special_values test : wrong element count" << std::endl;
        return EqualityError;
    }
    for (std::size_t i = 0; i < values.size(); i++) {
        // Exact comparison: the encoding must be bit-preserving.
        if (decoded[i] != values[i]) {
            std::cerr << "  npy_special_values test : element " << i << " is "
                << decoded[i] << ", expected " << values[i] << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_parameter_values_validate(void) {
    std::vector<std::vector<double>> sets;
    std::vector<double> two;
    two.push_back(1.0);
    two.push_back(2.0);
    sets.push_back(two);

    // Matching counts are accepted.
    try {
        ParameterValues::validate(sets, 2);
    } catch (const std::invalid_argument &exc) {
        std::cerr << "  parameter_values_validate test : a matching set was rejected : "
            << exc.what() << std::endl;
        return EqualityError;
    }

    // Too few values for the circuit.
    bool threw = false;
    try {
        ParameterValues::validate(sets, 3);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  parameter_values_validate test : a short set was accepted" << std::endl;
        return EqualityError;
    }

    // No values at all for a parameterized circuit.
    threw = false;
    try {
        ParameterValues::validate(std::vector<std::vector<double>>(), 1);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  parameter_values_validate test : an empty set list was accepted "
            << "for a parameterized circuit" << std::endl;
        return EqualityError;
    }

    // One short set inside an otherwise valid sweep is still rejected.
    sets.push_back(std::vector<double>(1, 9.0));
    threw = false;
    try {
        ParameterValues::validate(sets, 2);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  parameter_values_validate test : a ragged sweep was accepted"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_pub_parameters(void) {
    auto circ = make_parameterized_circuit();
    if (circ.num_parameters() != 2) {
        std::cerr << "  sampler_pub_parameters test : the circuit has "
            << circ.num_parameters() << " parameters, expected 2" << std::endl;
        return EqualityError;
    }

    std::vector<double> values;
    values.push_back(0.1);
    values.push_back(0.2);
    auto pub = SamplerPub(circ, values, 128);

    if (pub.num_parameter_sets() != 1) {
        std::cerr << "  sampler_pub_parameters test : " << pub.num_parameter_sets()
            << " sets, expected 1" << std::endl;
        return EqualityError;
    }

    const auto payload = pub.to_json();
    if (payload.size() != 3) {
        std::cerr << "  sampler_pub_parameters test : PUB has " << payload.size()
            << " elements, expected 3" << std::endl;
        return EqualityError;
    }

    std::vector<std::uint8_t> npy;
    std::string header;
    std::vector<double> decoded;
    if (!decode_payload(payload[1], npy) || !parse_npy(npy, header, decoded)) {
        std::cerr << "  sampler_pub_parameters test : element 1 is not a parameter payload"
            << std::endl;
        return EqualityError;
    }
    if (decoded.size() != 2 || decoded[0] != 0.1 || decoded[1] != 0.2) {
        std::cerr << "  sampler_pub_parameters test : the values did not survive" << std::endl;
        return EqualityError;
    }
    if (payload[2].get<uint_t>() != 128) {
        std::cerr << "  sampler_pub_parameters test : shots did not survive" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_pub_rejects_wrong_count(void) {
    auto circ = make_parameterized_circuit();

    // One value for a two-parameter circuit.
    bool threw = false;
    try {
        SamplerPub bad(circ, std::vector<double>(1, 0.5), 10);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  sampler_pub_rejects_wrong_count test : a short value list was accepted"
            << std::endl;
        return EqualityError;
    }

    // Three values for a two-parameter circuit.
    threw = false;
    try {
        SamplerPub bad(circ, std::vector<double>(3, 0.5), 10);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  sampler_pub_rejects_wrong_count test : a long value list was accepted"
            << std::endl;
        return EqualityError;
    }

    // A parameterized circuit with no values at all must fail validation, and
    // to_json must refuse to build a payload that cannot run.
    auto pub = SamplerPub(circ, 10);
    if (pub.validate()) {
        std::cerr << "  sampler_pub_rejects_wrong_count test : a pub with no values validated"
            << std::endl;
        return EqualityError;
    }
    threw = false;
    try {
        pub.to_json();
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  sampler_pub_rejects_wrong_count test : to_json built a payload "
            << "with no values" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_pub_unparameterized(void) {
    // A circuit with no parameters still gets a well-formed empty array, and
    // must not need any values.
    auto circ = QuantumCircuit(1, 1);
    circ.h(0);
    circ.measure(0, 0);

    auto pub = SamplerPub(circ, 64);
    if (!pub.validate()) {
        std::cerr << "  sampler_pub_unparameterized test : the pub did not validate"
            << std::endl;
        return EqualityError;
    }

    const auto payload = pub.to_json();
    std::vector<std::uint8_t> npy;
    std::string header;
    std::vector<double> decoded;
    if (!decode_payload(payload[1], npy) || !parse_npy(npy, header, decoded)) {
        std::cerr << "  sampler_pub_unparameterized test : element 1 is not a parameter payload"
            << std::endl;
        return EqualityError;
    }
    if (!decoded.empty() || header.find("'shape': (0,)") == std::string::npos) {
        std::cerr << "  sampler_pub_unparameterized test : expected an empty array, got \""
            << header << "\"" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_pub_sweep(void) {
    auto circ = make_parameterized_circuit();

    std::vector<std::vector<double>> sets;
    std::vector<double> first;
    first.push_back(0.0);
    first.push_back(1.0);
    std::vector<double> second;
    second.push_back(2.0);
    second.push_back(3.0);
    sets.push_back(first);
    sets.push_back(second);

    auto pub = SamplerPub(circ, sets, 32);
    if (pub.num_parameter_sets() != 2) {
        std::cerr << "  sampler_pub_sweep test : " << pub.num_parameter_sets()
            << " sets, expected 2" << std::endl;
        return EqualityError;
    }

    const auto payload = pub.to_json();
    std::vector<std::uint8_t> npy;
    std::string header;
    std::vector<double> decoded;
    if (!decode_payload(payload[1], npy) || !parse_npy(npy, header, decoded)) {
        std::cerr << "  sampler_pub_sweep test : element 1 is not a parameter payload"
            << std::endl;
        return EqualityError;
    }
    if (header.find("'shape': (2, 2)") == std::string::npos || decoded.size() != 4) {
        std::cerr << "  sampler_pub_sweep test : unexpected shape \"" << header << "\""
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_pub_values_survive_copy(void) {
    auto circ = make_parameterized_circuit();
    std::vector<double> values;
    values.push_back(0.25);
    values.push_back(0.75);

    auto pub = SamplerPub(circ, values, 16);
    SamplerPub copied(pub);

    if (copied.num_parameter_sets() != 1
        || copied.parameter_values()[0].size() != 2
        || copied.parameter_values()[0][0] != 0.25) {
        std::cerr << "  sampler_pub_values_survive_copy test : the values were lost on copy"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_pub_parameters(void) {
    auto circ = QuantumCircuit(1, 0);
    auto theta = Parameter("theta");
    circ.rx(theta, 0);

    auto pub = EstimatorPub(circ, std::string("Z"), 0.01);

    // Constructing the pub is allowed, since values can only be set afterwards,
    // but it must not pass the submission check until they are.
    if (pub.validate_for_submission()) {
        std::cerr << "  estimator_pub_parameters test : a pub with no values passed the "
            << "submission check" << std::endl;
        return EqualityError;
    }

    pub.set_parameter_values(std::vector<double>(1, 0.5));
    if (!pub.validate_for_submission()) {
        std::cerr << "  estimator_pub_parameters test : the pub did not validate after "
            << "setting values" << std::endl;
        return EqualityError;
    }

    const auto payload = pub.to_json();
    std::vector<std::uint8_t> npy;
    std::string header;
    std::vector<double> decoded;
    if (!decode_payload(payload[2], npy) || !parse_npy(npy, header, decoded)) {
        std::cerr << "  estimator_pub_parameters test : element 2 is not a parameter payload"
            << std::endl;
        return EqualityError;
    }
    if (decoded.size() != 1 || decoded[0] != 0.5) {
        std::cerr << "  estimator_pub_parameters test : the value did not survive" << std::endl;
        return EqualityError;
    }

    // A wrong count is rejected where the values are given.
    bool threw = false;
    try {
        pub.set_parameter_values(std::vector<double>(2, 0.5));
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  estimator_pub_parameters test : a wrong value count was accepted"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_pub_unparameterized(void) {
    // An unparameterized estimator pub keeps sending null, which the schema
    // accepts for the optional parameter-values slot.
    auto circ = QuantumCircuit(1, 0);
    circ.h(0);

    auto pub = EstimatorPub(circ, std::string("Z"), 0.01);
    if (!pub.validate()) {
        std::cerr << "  estimator_pub_unparameterized test : the pub did not validate"
            << std::endl;
        return EqualityError;
    }
    const auto payload = pub.to_json();
    if (!payload[2].is_null()) {
        std::cerr << "  estimator_pub_unparameterized test : element 2 is not null"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

#if defined(_WIN32)
int test_parameter_values(int argc, char** const argv) {
#else
int test_parameter_values(int argc, char** argv) {
#endif
    int num_failed = 0;
    num_failed += RUN_TEST(test_npy_single_set);
    num_failed += RUN_TEST(test_npy_sweep);
    num_failed += RUN_TEST(test_npy_empty);
    num_failed += RUN_TEST(test_npy_alignment);
    num_failed += RUN_TEST(test_npy_special_values);
    num_failed += RUN_TEST(test_parameter_values_validate);
    num_failed += RUN_TEST(test_sampler_pub_parameters);
    num_failed += RUN_TEST(test_sampler_pub_rejects_wrong_count);
    num_failed += RUN_TEST(test_sampler_pub_unparameterized);
    num_failed += RUN_TEST(test_sampler_pub_sweep);
    num_failed += RUN_TEST(test_sampler_pub_values_survive_copy);
    num_failed += RUN_TEST(test_estimator_pub_parameters);
    num_failed += RUN_TEST(test_estimator_pub_unparameterized);

    std::cerr << "=== Number of failed subtests: " << num_failed << std::endl;
    return num_failed;
}
