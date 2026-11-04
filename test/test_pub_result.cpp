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

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "common.hpp"

#include "circuit/quantumcircuit.hpp"
#include "primitives/containers/estimator_pub.hpp"
#include "primitives/containers/estimator_pub_result.hpp"
#include "primitives/containers/sampler_pub.hpp"
#include "primitives/containers/sampler_pub_result.hpp"

using namespace Qiskit;
using namespace Qiskit::circuit;
using namespace Qiskit::primitives;

// A two-parameter circuit measuring into one named register.
static QuantumCircuit make_circuit(void) {
    auto qreg = QuantumRegister(2);
    auto creg = ClassicalRegister(2, std::string("meas"));
    QuantumCircuit circ(qreg, creg);
    auto a = Parameter("a");
    auto b = Parameter("b");
    circ.rx(a, 0);
    circ.ry(b, 1);
    circ.measure(qreg, creg);
    return circ;
}

// Build the raw sampler result JSON for a pub with no parameter sweep: the
// samples are a flat array of hex strings, one per shot.
static nlohmann::ordered_json make_sampler_json(const std::vector<std::string>& samples,
                                                const uint_t num_bits) {
    nlohmann::ordered_json creg;
    creg["samples"] = samples;
    creg["num_bits"] = num_bits;

    nlohmann::ordered_json data;
    data["meas"] = creg;

    nlohmann::ordered_json input;
    input["data"] = data;
    return input;
}

// Build the raw sampler result JSON for a swept pub: the samples are nested,
// one inner array of hex strings per parameter value set. This is the shape the
// service actually returns, as captured from a real job.
static nlohmann::ordered_json make_swept_sampler_json(
    const std::vector<std::vector<std::string>>& samples, const uint_t num_bits) {
    nlohmann::ordered_json creg;
    creg["samples"] = samples;
    creg["num_bits"] = num_bits;

    nlohmann::ordered_json data;
    data["meas"] = creg;

    nlohmann::ordered_json input;
    input["data"] = data;
    return input;
}

static int test_sampler_result_single_set(void) {
    // A pub with one value set keeps exactly one entry, so the plain data()
    // accessors behave as before.
    auto circ = make_circuit();
    auto pub = SamplerPub(circ, std::vector<double>({0.1, 0.2}), 4);
    SamplerPubResult result(pub);

    if (result.size() != 1) {
        std::cerr << "  sampler_result_single_set test : " << result.size()
            << " sets, expected 1" << std::endl;
        return EqualityError;
    }

    std::vector<std::string> samples;
    samples.push_back("0x0");
    samples.push_back("0x1");
    samples.push_back("0x2");
    samples.push_back("0x3");
    auto json = make_sampler_json(samples, 2);
    if (!result.from_json(json)) {
        std::cerr << "  sampler_result_single_set test : from_json failed" << std::endl;
        return RuntimeError;
    }

    if (result.data("meas").num_shots() != 4) {
        std::cerr << "  sampler_result_single_set test : " << result.data("meas").num_shots()
            << " shots, expected 4" << std::endl;
        return EqualityError;
    }
    // Indexing set 0 must give the same data as the plain accessor.
    if (result[0].data("meas").num_shots() != 4) {
        std::cerr << "  sampler_result_single_set test : set 0 holds "
            << result[0].data("meas").num_shots() << " shots, expected 4" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_result_splits_sets(void) {
    // A pub with three value sets and two shots each returns six shots, which
    // must be split into three entries of two.
    auto circ = make_circuit();
    std::vector<std::vector<double>> params;
    params.push_back(std::vector<double>({0.0, 0.0}));
    params.push_back(std::vector<double>({0.5, 0.5}));
    params.push_back(std::vector<double>({1.0, 1.0}));

    auto pub = SamplerPub(circ, params, 2);
    SamplerPubResult result(pub);

    if (result.size() != 3) {
        std::cerr << "  sampler_result_splits_sets test : " << result.size()
            << " sets, expected 3" << std::endl;
        return EqualityError;
    }

    // Distinct samples per set so the split can be checked by value.
    std::vector<std::vector<std::string>> samples;
    samples.push_back(std::vector<std::string>({"0x0", "0x0"}));
    samples.push_back(std::vector<std::string>({"0x1", "0x1"}));
    samples.push_back(std::vector<std::string>({"0x3", "0x3"}));
    auto json = make_swept_sampler_json(samples, 2);
    if (!result.from_json(json)) {
        std::cerr << "  sampler_result_splits_sets test : from_json failed" << std::endl;
        return RuntimeError;
    }

    const char* expected[] = {"00", "01", "11"};
    for (uint_t set = 0; set < 3; set++) {
        auto bits = result[set].data("meas");
        if (bits.num_shots() != 2) {
            std::cerr << "  sampler_result_splits_sets test : set " << set << " holds "
                << bits.num_shots() << " shots, expected 2" << std::endl;
            return EqualityError;
        }
        const auto strings = bits.get_bitstrings();
        for (uint_t i = 0; i < strings.size(); i++) {
            if (strings[i] != expected[set]) {
                std::cerr << "  sampler_result_splits_sets test : set " << set
                    << " shot " << i << " is \"" << strings[i] << "\", expected \""
                    << expected[set] << "\"" << std::endl;
                return EqualityError;
            }
        }
    }
    return Ok;
}

static int test_sampler_result_counts_union(void) {
    // get_counts over the whole result unions every set, matching Qiskit's
    // get_counts() with no location.
    auto circ = make_circuit();
    std::vector<std::vector<double>> params;
    params.push_back(std::vector<double>({0.0, 0.0}));
    params.push_back(std::vector<double>({1.0, 1.0}));

    auto pub = SamplerPub(circ, params, 2);
    SamplerPubResult result(pub);

    std::vector<std::vector<std::string>> samples;
    samples.push_back(std::vector<std::string>({"0x0", "0x0"}));
    samples.push_back(std::vector<std::string>({"0x0", "0x3"}));
    auto json = make_swept_sampler_json(samples, 2);
    if (!result.from_json(json)) {
        std::cerr << "  sampler_result_counts_union test : from_json failed" << std::endl;
        return RuntimeError;
    }

    // Per-set counts: set 0 is all "00", set 1 is one "00" and one "11".
    const auto first = result[0].data("meas").get_counts();
    if (first.size() != 1 || first.find("00") == first.end() || first.at("00") != 2) {
        std::cerr << "  sampler_result_counts_union test : set 0 counts are wrong"
            << std::endl;
        return EqualityError;
    }

    const auto unioned = result.get_counts("meas");
    if (unioned.size() != 2 || unioned.at("00") != 3 || unioned.at("11") != 1) {
        std::cerr << "  sampler_result_counts_union test : unioned counts are wrong"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_result_rejects_set_mismatch(void) {
    // A result holding a different number of value sets than the pub submitted
    // does not belong to that pub, which must be reported.
    auto circ = make_circuit();
    std::vector<std::vector<double>> params;
    params.push_back(std::vector<double>({0.0, 0.0}));
    params.push_back(std::vector<double>({1.0, 1.0}));

    auto pub = SamplerPub(circ, params, 2);
    SamplerPubResult result(pub);

    // Three sets returned for a two-set pub.
    std::vector<std::vector<std::string>> samples;
    samples.push_back(std::vector<std::string>({"0x0", "0x0"}));
    samples.push_back(std::vector<std::string>({"0x1", "0x1"}));
    samples.push_back(std::vector<std::string>({"0x2", "0x2"}));
    auto json = make_swept_sampler_json(samples, 2);
    if (result.from_json(json)) {
        std::cerr << "  sampler_result_rejects_set_mismatch test : 3 sets for a 2-set pub "
            << "was accepted" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_result_real_payload(void) {
    // The shape captured from a real two-set job: samples nested one array per
    // parameter value set, 100 shots each, num_bits 2.
    auto circ = make_circuit();
    std::vector<std::vector<double>> params;
    params.push_back(std::vector<double>({0.5, 0.6}));
    params.push_back(std::vector<double>({0.3, 0.9}));

    auto pub = SamplerPub(circ, params, 100);
    SamplerPubResult result(pub);

    std::vector<std::vector<std::string>> samples(2);
    for (uint_t set = 0; set < 2; set++) {
        for (uint_t shot = 0; shot < 100; shot++) {
            // Set 0 is all "00", set 1 all "11", so the split is checkable.
            samples[set].push_back(set == 0 ? "0x0" : "0x3");
        }
    }
    auto json = make_swept_sampler_json(samples, 2);
    if (!result.from_json(json)) {
        std::cerr << "  sampler_result_real_payload test : from_json failed" << std::endl;
        return RuntimeError;
    }

    if (result.size() != 2) {
        std::cerr << "  sampler_result_real_payload test : " << result.size()
            << " sets, expected 2" << std::endl;
        return EqualityError;
    }
    for (uint_t set = 0; set < 2; set++) {
        auto bits = result[set].data("meas");
        if (bits.num_shots() != 100) {
            std::cerr << "  sampler_result_real_payload test : set " << set << " holds "
                << bits.num_shots() << " shots, expected 100" << std::endl;
            return EqualityError;
        }
        const auto counts = bits.get_counts();
        const std::string expected = set == 0 ? "00" : "11";
        if (counts.size() != 1 || counts.find(expected) == counts.end()
            || counts.at(expected) != 100) {
            std::cerr << "  sampler_result_real_payload test : set " << set
                << " counts are wrong" << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_sampler_result_index_out_of_range(void) {
    auto circ = make_circuit();
    auto pub = SamplerPub(circ, std::vector<double>({0.1, 0.2}), 4);
    SamplerPubResult result(pub);

    bool threw = false;
    try {
        result[1].data("meas");
    } catch (const std::out_of_range &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  sampler_result_index_out_of_range test : set 1 of a single-set "
            << "result was accessible" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_sampler_result_unparameterized(void) {
    // A circuit with no parameters still has exactly one entry.
    auto qreg = QuantumRegister(1);
    auto creg = ClassicalRegister(1, std::string("meas"));
    QuantumCircuit circ(qreg, creg);
    circ.h(0);
    circ.measure(qreg, creg);

    auto pub = SamplerPub(circ, 2);
    SamplerPubResult result(pub);
    if (result.size() != 1) {
        std::cerr << "  sampler_result_unparameterized test : " << result.size()
            << " sets, expected 1" << std::endl;
        return EqualityError;
    }

    std::vector<std::string> samples;
    samples.push_back("0x0");
    samples.push_back("0x1");
    auto json = make_sampler_json(samples, 1);
    if (!result.from_json(json)) {
        std::cerr << "  sampler_result_unparameterized test : from_json failed" << std::endl;
        return RuntimeError;
    }
    if (result.data("meas").num_shots() != 2) {
        std::cerr << "  sampler_result_unparameterized test : wrong shot count" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_result_single_set(void) {
    auto circ = QuantumCircuit(1, 0);
    auto theta = Parameter("theta");
    circ.rx(theta, 0);

    auto pub = EstimatorPub(circ, std::string("Z"), 0.01);
    pub.set_parameter_values(std::vector<double>(1, 0.5));
    EstimatorPubResult result(pub);

    nlohmann::ordered_json data;
    data["evs"] = 0.25;
    data["stds"] = 0.01;
    nlohmann::ordered_json input;
    input["data"] = data;

    if (!result.from_json(input)) {
        std::cerr << "  estimator_result_single_set test : from_json failed" << std::endl;
        return RuntimeError;
    }
    if (result.size() != 1 || result.num_observables() != 1) {
        std::cerr << "  estimator_result_single_set test : " << result.size() << " sets, "
            << result.num_observables() << " observables, expected 1 and 1" << std::endl;
        return EqualityError;
    }
    if (result.evs().size() != 1 || result.evs()[0] != 0.25) {
        std::cerr << "  estimator_result_single_set test : the flat evs are wrong"
            << std::endl;
        return EqualityError;
    }
    if (result[0][0] != 0.25 || result.ev(0, 0) != 0.25) {
        std::cerr << "  estimator_result_single_set test : indexed access is wrong"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_result_sweep(void) {
    // Three value sets against one observable return three values, in set
    // order.
    auto circ = QuantumCircuit(1, 0);
    auto theta = Parameter("theta");
    circ.rx(theta, 0);

    std::vector<std::vector<double>> params;
    params.push_back(std::vector<double>(1, 0.0));
    params.push_back(std::vector<double>(1, 0.5));
    params.push_back(std::vector<double>(1, 1.0));

    auto pub = EstimatorPub(circ, std::string("Z"), 0.01);
    pub.set_parameter_values(params);
    EstimatorPubResult result(pub);

    nlohmann::ordered_json data;
    data["evs"] = std::vector<double>({1.0, 0.5, 0.0});
    data["stds"] = std::vector<double>({0.01, 0.02, 0.03});
    nlohmann::ordered_json input;
    input["data"] = data;

    if (!result.from_json(input)) {
        std::cerr << "  estimator_result_sweep test : from_json failed" << std::endl;
        return RuntimeError;
    }
    if (result.size() != 3 || result.num_observables() != 1) {
        std::cerr << "  estimator_result_sweep test : " << result.size() << " sets, "
            << result.num_observables() << " observables, expected 3 and 1" << std::endl;
        return EqualityError;
    }

    const double expected[] = {1.0, 0.5, 0.0};
    const double expected_stds[] = {0.01, 0.02, 0.03};
    for (uint_t set = 0; set < 3; set++) {
        if (result[set].evs().size() != 1 || result[set][0] != expected[set]) {
            std::cerr << "  estimator_result_sweep test : set " << set << " ev is "
                << result[set][0] << ", expected " << expected[set] << std::endl;
            return EqualityError;
        }
        if (result[set].stds().size() != 1 || result[set].stds()[0] != expected_stds[set]) {
            std::cerr << "  estimator_result_sweep test : set " << set
                << " std is wrong" << std::endl;
            return EqualityError;
        }
    }
    // The flat accessor keeps the whole sweep in row-major order.
    if (result.evs().size() != 3) {
        std::cerr << "  estimator_result_sweep test : the flat evs hold "
            << result.evs().size() << " values, expected 3" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_result_nested_evs(void) {
    // Two value sets against two observables arrive nested, one inner array per
    // set, and are flattened in (set, observable) order.
    auto circ = QuantumCircuit(1, 0);
    auto theta = Parameter("theta");
    circ.rx(theta, 0);

    nlohmann::ordered_json observables = nlohmann::ordered_json::array();
    observables.push_back("Z");
    observables.push_back("X");

    std::vector<std::vector<double>> params;
    params.push_back(std::vector<double>(1, 0.0));
    params.push_back(std::vector<double>(1, 1.0));

    auto pub = EstimatorPub(circ, observables, 0.01);
    pub.set_parameter_values(params);
    EstimatorPubResult result(pub);

    nlohmann::ordered_json evs = nlohmann::ordered_json::array();
    evs.push_back(std::vector<double>({1.0, 0.0}));
    evs.push_back(std::vector<double>({0.5, 0.25}));

    nlohmann::ordered_json data;
    data["evs"] = evs;
    nlohmann::ordered_json input;
    input["data"] = data;

    if (!result.from_json(input)) {
        std::cerr << "  estimator_result_nested_evs test : from_json failed" << std::endl;
        return RuntimeError;
    }
    if (result.size() != 2 || result.num_observables() != 2) {
        std::cerr << "  estimator_result_nested_evs test : " << result.size() << " sets, "
            << result.num_observables() << " observables, expected 2 and 2" << std::endl;
        return EqualityError;
    }
    if (result[0][0] != 1.0 || result[0][1] != 0.0
        || result[1][0] != 0.5 || result[1][1] != 0.25) {
        std::cerr << "  estimator_result_nested_evs test : the values were not flattened "
            << "in (set, observable) order" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_result_rejects_nesting_without_sweep(void) {
    // Nesting is only meaningful for a pub that swept. For a single value set
    // it is malformed, and accepting it would mask a result belonging to a
    // different pub.
    auto circ = QuantumCircuit(1, 0);
    auto theta = Parameter("theta");
    circ.rx(theta, 0);

    auto pub = EstimatorPub(circ, std::string("Z"), 0.01);
    pub.set_parameter_values(std::vector<double>(1, 0.5));
    EstimatorPubResult result(pub);

    nlohmann::ordered_json evs = nlohmann::ordered_json::array();
    evs.push_back(std::vector<double>({1.0}));

    nlohmann::ordered_json data;
    data["evs"] = evs;
    nlohmann::ordered_json input;
    input["data"] = data;

    if (result.from_json(input)) {
        std::cerr << "  estimator_result_rejects_nesting_without_sweep test : nested evs "
            << "were accepted for a single value set" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_result_rejects_ragged_nesting(void) {
    // The values form a rectangular array, so inner arrays of differing
    // lengths mean the result is malformed.
    auto circ = QuantumCircuit(1, 0);
    auto theta = Parameter("theta");
    circ.rx(theta, 0);

    nlohmann::ordered_json observables = nlohmann::ordered_json::array();
    observables.push_back("Z");
    observables.push_back("X");

    std::vector<std::vector<double>> params;
    params.push_back(std::vector<double>(1, 0.0));
    params.push_back(std::vector<double>(1, 1.0));

    auto pub = EstimatorPub(circ, observables, 0.01);
    pub.set_parameter_values(params);
    EstimatorPubResult result(pub);

    nlohmann::ordered_json evs = nlohmann::ordered_json::array();
    evs.push_back(std::vector<double>({1.0, 0.0}));
    evs.push_back(std::vector<double>({0.5}));

    nlohmann::ordered_json data;
    data["evs"] = evs;
    nlohmann::ordered_json input;
    input["data"] = data;

    if (result.from_json(input)) {
        std::cerr << "  estimator_result_rejects_ragged_nesting test : inner arrays of "
            << "differing lengths were accepted" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_result_rejects_wrong_length(void) {
    // Two value sets against one observable must return two values; anything
    // else means the result does not match the pub.
    auto circ = QuantumCircuit(1, 0);
    auto theta = Parameter("theta");
    circ.rx(theta, 0);

    std::vector<std::vector<double>> params;
    params.push_back(std::vector<double>(1, 0.0));
    params.push_back(std::vector<double>(1, 1.0));

    auto pub = EstimatorPub(circ, std::string("Z"), 0.01);
    pub.set_parameter_values(params);
    EstimatorPubResult result(pub);

    nlohmann::ordered_json data;
    data["evs"] = std::vector<double>({1.0});
    nlohmann::ordered_json input;
    input["data"] = data;

    if (result.from_json(input)) {
        std::cerr << "  estimator_result_rejects_wrong_length test : 1 value for 2 sets "
            << "was accepted" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_result_index_out_of_range(void) {
    auto circ = QuantumCircuit(1, 0);
    circ.h(0);

    auto pub = EstimatorPub(circ, std::string("Z"), 0.01);
    EstimatorPubResult result(pub);

    nlohmann::ordered_json data;
    data["evs"] = 0.5;
    nlohmann::ordered_json input;
    input["data"] = data;
    if (!result.from_json(input)) {
        std::cerr << "  estimator_result_index_out_of_range test : from_json failed"
            << std::endl;
        return RuntimeError;
    }

    bool threw = false;
    try {
        result[1];
    } catch (const std::out_of_range &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  estimator_result_index_out_of_range test : set 1 of a single-set "
            << "result was accessible" << std::endl;
        return EqualityError;
    }

    threw = false;
    try {
        result[0][1];
    } catch (const std::out_of_range &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  estimator_result_index_out_of_range test : observable 1 of a "
            << "single-observable set was accessible" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_bit_array_shot_range(void) {
    BitArray bits;
    bits.set_bits(2);

    nlohmann::ordered_json input;
    input["samples"] = std::vector<std::string>({"0x0", "0x1", "0x2", "0x3"});
    input["num_bits"] = 2;
    bits.from_json(input);

    auto middle = bits.get_shot_range(1, 2);
    if (middle.num_shots() != 2 || middle.num_bits() != 2) {
        std::cerr << "  bit_array_shot_range test : got " << middle.num_shots()
            << " shots of " << middle.num_bits() << " bits, expected 2 and 2" << std::endl;
        return EqualityError;
    }
    const auto strings = middle.get_bitstrings();
    if (strings[0] != "01" || strings[1] != "10") {
        std::cerr << "  bit_array_shot_range test : got \"" << strings[0] << "\", \""
            << strings[1] << "\", expected \"01\", \"10\"" << std::endl;
        return EqualityError;
    }

    // A range running past the end is truncated rather than reading out of
    // bounds.
    auto tail = bits.get_shot_range(3, 10);
    if (tail.num_shots() != 1) {
        std::cerr << "  bit_array_shot_range test : a truncated range holds "
            << tail.num_shots() << " shots, expected 1" << std::endl;
        return EqualityError;
    }
    auto past = bits.get_shot_range(9, 2);
    if (past.num_shots() != 0 || past.num_bits() != 2) {
        std::cerr << "  bit_array_shot_range test : a range past the end is not empty"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

#if defined(_WIN32)
int test_pub_result(int argc, char** const argv) {
#else
int test_pub_result(int argc, char** argv) {
#endif
    int num_failed = 0;
    num_failed += RUN_TEST(test_bit_array_shot_range);
    num_failed += RUN_TEST(test_sampler_result_single_set);
    num_failed += RUN_TEST(test_sampler_result_splits_sets);
    num_failed += RUN_TEST(test_sampler_result_counts_union);
    num_failed += RUN_TEST(test_sampler_result_rejects_set_mismatch);
    num_failed += RUN_TEST(test_sampler_result_real_payload);
    num_failed += RUN_TEST(test_sampler_result_index_out_of_range);
    num_failed += RUN_TEST(test_sampler_result_unparameterized);
    num_failed += RUN_TEST(test_estimator_result_single_set);
    num_failed += RUN_TEST(test_estimator_result_sweep);
    num_failed += RUN_TEST(test_estimator_result_nested_evs);
    num_failed += RUN_TEST(test_estimator_result_rejects_nesting_without_sweep);
    num_failed += RUN_TEST(test_estimator_result_rejects_ragged_nesting);
    num_failed += RUN_TEST(test_estimator_result_rejects_wrong_length);
    num_failed += RUN_TEST(test_estimator_result_index_out_of_range);

    std::cerr << "=== Number of failed subtests: " << num_failed << std::endl;
    return num_failed;
}
