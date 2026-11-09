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
#include <complex>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

#include <nlohmann/json.hpp>

#include "common.hpp"

#include "circuit/quantumcircuit.hpp"
#include "primitives/backend_estimator_v2.hpp"
#include "primitives/containers/estimator_pub.hpp"
#include "providers/backend.hpp"
#include "providers/job.hpp"
#include "providers/qrmi_estimator_payload.hpp"
#include "quantum_info/sparse_observable.hpp"

// Windows headers can define ERROR after jobstatus.hpp has already been parsed.
#ifdef ERROR
#undef ERROR
#endif

using namespace Qiskit;
using namespace Qiskit::circuit;
using namespace Qiskit::primitives;
using namespace Qiskit::providers;
using namespace Qiskit::quantum_info;

class MockEstimatorJob : public Job {
protected:
    nlohmann::ordered_json results_;
    JobStatus status_;
    bool fail_result_ = false;
    bool override_num_results_ = false;
    uint_t reported_num_results_ = 0;

public:
    MockEstimatorJob(const nlohmann::ordered_json& results, JobStatus status = JobStatus::DONE, bool fail_result = false, bool override_num_results = false, uint_t reported_num_results = 0)
    {
        results_ = results;
        status_ = status;
        fail_result_ = fail_result;
        override_num_results_ = override_num_results;
        reported_num_results_ = reported_num_results;
    }

    JobStatus status(void) override
    {
        return status_;
    }

    uint_t num_results(void) override
    {
        if (override_num_results_) {
            return reported_num_results_;
        }
        return results_.size();
    }

    bool result(uint_t index, SamplerPubResult& result) override
    {
        return false;
    }

    bool result(uint_t index, EstimatorPubResult& result) override
    {
        if (fail_result_) {
            return false;
        }
        if (index >= results_.size()) {
            return false;
        }
        auto item = results_[index];
        return result.from_json(item);
    }
};

class MockEstimatorBackend : public BackendV2 {
protected:
    transpiler::Target target_;
    nlohmann::ordered_json results_;
    JobStatus status_ = JobStatus::DONE;
    bool fail_result_ = false;
    bool override_num_results_ = false;
    uint_t reported_num_results_ = 0;

public:
    double received_precision_ = -1.0;
    std::vector<EstimatorPub> received_pubs_;

    MockEstimatorBackend(const nlohmann::ordered_json& results)
    {
        results_ = results;
    }

    MockEstimatorBackend(const nlohmann::ordered_json& results, JobStatus status)
    {
        results_ = results;
        status_ = status;
    }

    const transpiler::Target& target(void) override
    {
        return target_;
    }

    std::shared_ptr<Job> run(std::vector<SamplerPub>& circuits, uint_t shots = 0) override
    {
        return nullptr;
    }

    std::shared_ptr<Job> run(std::vector<EstimatorPub>& pubs, double precision) override
    {
        received_precision_ = precision;
        received_pubs_ = pubs;
        return std::make_shared<MockEstimatorJob>(results_, status_, fail_result_, override_num_results_, reported_num_results_);
    }

    void set_fail_result(bool fail_result)
    {
        fail_result_ = fail_result;
    }

    void set_reported_num_results(uint_t reported_num_results)
    {
        override_num_results_ = true;
        reported_num_results_ = reported_num_results;
    }
};

static nlohmann::ordered_json estimator_result(double ev, double stddev, int shots, double target_precision)
{
    nlohmann::ordered_json result;
    result["data"]["evs"] = ev;
    result["data"]["stds"] = stddev;
    result["metadata"]["shots"] = shots;
    result["metadata"]["target_precision"] = target_precision;
    return result;
}

static int test_estimator_pub_to_json(void)
{
    auto circ = QuantumCircuit(2, 0);
    std::vector<std::pair<std::string, double>> terms;
    terms.push_back(std::make_pair(std::string("ZI"), 1.0));
    terms.push_back(std::make_pair(std::string("IZ"), -0.5));

    auto unresolved_pub = EstimatorPub(circ, terms);
    auto unresolved_payload = unresolved_pub.to_json();
    if (unresolved_payload.size() != 3) {
        std::cerr << "  estimator pub json test : unexpected unresolved PUB shape." << std::endl;
        return EqualityError;
    }

    auto pub = EstimatorPub(circ, terms, 0.02);
    auto payload = pub.to_json();

    if (payload.size() != 4 || !payload[1].is_object() || !payload[2].is_null()) {
        std::cerr << "  estimator pub json test : unexpected PUB shape." << std::endl;
        return EqualityError;
    }
    if (std::abs(payload[1]["ZI"].get<double>() - 1.0) > 1e-12 ||
        std::abs(payload[1]["IZ"].get<double>() + 0.5) > 1e-12 ||
        std::abs(payload[3].get<double>() - 0.02) > 1e-12) {
        std::cerr << "  estimator pub json test : unexpected observable or precision values." << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_qrmi_payload_contract(void)
{
    auto circ = QuantumCircuit(1, 0);
    auto default_precision_pub = EstimatorPub(circ, std::string("Z"));
    auto explicit_precision_pub = EstimatorPub(circ, std::string("X"), 0.03125);
    std::vector<EstimatorPub> pubs;
    pubs.push_back(default_precision_pub);
    pubs.push_back(explicit_precision_pub);

    auto payload = QRMIEstimatorPayload::build_estimator_input(pubs, 0.02);

    if (QRMIEstimatorPayload::estimator_program_id() != "estimator") {
        std::cerr << "  estimator QRMI payload test : wrong program id." << std::endl;
        return EqualityError;
    }

    if (!payload.is_object() ||
        !payload.contains("version") ||
        !payload["version"].is_number_integer() ||
        payload["version"].get<int>() != 2 ||
        !payload.contains("support_qiskit") ||
        !payload["support_qiskit"].is_boolean() ||
        payload["support_qiskit"].get<bool>() ||
        !payload.contains("options") ||
        !payload["options"].is_object() ||
        !payload.contains("pubs") ||
        !payload["pubs"].is_array() ||
        payload["pubs"].size() != 2) {
        std::cerr << "  estimator QRMI payload test : unexpected top-level shape." << std::endl;
        return EqualityError;
    }

    const auto& first_pub = payload["pubs"][0];
    const auto& second_pub = payload["pubs"][1];
    // The circuit is a QPY payload object by default, or an OpenQASM 3 string
    // when QPY is unavailable or has been selected against. Qiskit Runtime
    // accepts either, so both are valid here.
    const bool first_circuit_ok =
        first_pub[0].is_string() ||
        (first_pub[0].is_object() && first_pub[0].contains("__type__") &&
         first_pub[0]["__type__"] == "QuantumCircuit" &&
         first_pub[0].contains("__value__") && first_pub[0]["__value__"].is_string());
    if (!first_pub.is_array() || first_pub.size() != 4 ||
        !second_pub.is_array() || second_pub.size() != 4 ||
        !first_circuit_ok ||
        !first_pub[1].is_string() ||
        first_pub[1].get<std::string>() != "Z" ||
        !first_pub[2].is_null() ||
        !first_pub[3].is_number() ||
        std::abs(first_pub[3].get<double>() - 0.02) > 1e-12 ||
        !second_pub[1].is_string() ||
        second_pub[1].get<std::string>() != "X" ||
        !second_pub[3].is_number() ||
        std::abs(second_pub[3].get<double>() - 0.03125) > 1e-12) {
        std::cerr << "  estimator QRMI payload test : unexpected PUB shape." << std::endl;
        return EqualityError;
    }

    if (!pubs[0].has_precision() ||
        std::abs(pubs[0].precision() - 0.02) > 1e-12 ||
        !pubs[1].has_precision() ||
        std::abs(pubs[1].precision() - 0.03125) > 1e-12) {
        std::cerr << "  estimator QRMI payload test : precision resolution failed." << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_estimator_qrmi_payload_rejects_measured_circuit(void)
{
    auto circ = QuantumCircuit(1, 1);
    circ.measure(0, 0);
    auto pub = EstimatorPub(circ, std::string("Z"));
    std::vector<EstimatorPub> pubs;
    pubs.push_back(pub);

    try {
        auto payload = QRMIEstimatorPayload::build_estimator_input(pubs, 0.02);
        (void)payload;
    } catch (const std::invalid_argument&) {
        return Ok;
    }

    std::cerr << "  estimator QRMI payload measured circuit test : measurement was not rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_pub_sums_duplicate_labels(void)
{
    // Issue #163: a label repeated in the term list is another term of the same
    // sum, so its coefficients add. Before the fix the last one won and this
    // observable was 0.25 * Z rather than 0.75 * Z.
    auto circ = QuantumCircuit(1, 0);
    std::vector<std::pair<std::string, double>> terms;
    terms.push_back(std::make_pair(std::string("Z"), 0.5));
    terms.push_back(std::make_pair(std::string("Z"), 0.25));

    auto payload = EstimatorPub(circ, terms).to_json();
    if (payload[1].size() != 1) {
        std::cerr << "  estimator pub duplicate labels test : " << payload[1].size()
            << " observable terms, expected 1." << std::endl;
        return EqualityError;
    }
    if (std::abs(payload[1]["Z"].get<double>() - 0.75) > 1e-12) {
        std::cerr << "  estimator pub duplicate labels test : coefficient is "
            << payload[1]["Z"].get<double>() << ", expected 0.75." << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_pub_duplicate_labels_match_observable(void)
{
    // The two input paths must agree: a repeated label through the term list
    // must give what the same sum gives through SparseObservable, which already
    // summed the duplicates.
    auto circ = QuantumCircuit(1, 0);
    std::vector<std::pair<std::string, double>> terms;
    terms.push_back(std::make_pair(std::string("Z"), 0.5));
    terms.push_back(std::make_pair(std::string("Z"), 0.25));

    std::vector<std::pair<std::string, std::complex<double>>> list;
    list.push_back(std::make_pair(std::string("Z"), std::complex<double>(0.5, 0.0)));
    list.push_back(std::make_pair(std::string("Z"), std::complex<double>(0.25, 0.0)));

    auto from_terms = EstimatorPub(circ, terms).to_json();
    auto from_observable = EstimatorPub(circ, SparseObservable::from_list(list)).to_json();

    if (from_terms[1] != from_observable[1]) {
        std::cerr << "  estimator pub duplicate labels match test : term list gave "
            << from_terms[1].dump() << " but SparseObservable gave "
            << from_observable[1].dump() << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_pub_sums_duplicate_complex_labels(void)
{
    // The complex overload forwards to the real one, so it must sum as well.
    auto circ = QuantumCircuit(1, 0);
    std::vector<std::pair<std::string, std::complex<double>>> terms;
    terms.push_back(std::make_pair(std::string("X"), std::complex<double>(1.5, 0.0)));
    terms.push_back(std::make_pair(std::string("Z"), std::complex<double>(1.0, 0.0)));
    terms.push_back(std::make_pair(std::string("X"), std::complex<double>(-0.5, 0.0)));

    auto payload = EstimatorPub(circ, terms).to_json();
    if (payload[1].size() != 2) {
        std::cerr << "  estimator pub duplicate complex labels test : " << payload[1].size()
            << " observable terms, expected 2." << std::endl;
        return EqualityError;
    }
    if (std::abs(payload[1]["X"].get<double>() - 1.0) > 1e-12 ||
        std::abs(payload[1]["Z"].get<double>() - 1.0) > 1e-12) {
        std::cerr << "  estimator pub duplicate complex labels test : unexpected coefficients "
            << payload[1].dump() << std::endl;
        return EqualityError;
    }
    // The terms keep the order their labels first appeared in.
    if (payload[1].begin().key() != "X") {
        std::cerr << "  estimator pub duplicate complex labels test : the first term is \""
            << payload[1].begin().key() << "\", expected \"X\"." << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_pub_duplicate_labels_cancel(void)
{
    // Summing to zero leaves the term in place with a zero coefficient, as
    // Qiskit does: dropping it is simplify's job, not the pub's.
    auto circ = QuantumCircuit(1, 0);
    std::vector<std::pair<std::string, double>> terms;
    terms.push_back(std::make_pair(std::string("Z"), 1.0));
    terms.push_back(std::make_pair(std::string("Z"), -1.0));

    auto payload = EstimatorPub(circ, terms).to_json();
    if (payload[1].size() != 1 || std::abs(payload[1]["Z"].get<double>()) > 1e-12) {
        std::cerr << "  estimator pub duplicate labels cancel test : expected a single zero "
            << "term, got " << payload[1].dump() << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_pub_sparse_observable(void)
{
    auto circ = QuantumCircuit(2, 0);
    std::string label = "ZI";
    auto observable = SparseObservable::from_label(label);
    auto pub = EstimatorPub(circ, observable);
    auto payload = pub.to_json();

    if (!payload[1].is_object() || !payload[1].contains("ZI")) {
        std::cerr << "  estimator pub sparse observable test : expected ZI observable." << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_estimator_rejects_empty_json_observable_map(void)
{
    auto circ = QuantumCircuit(1, 0);
    nlohmann::ordered_json observables = nlohmann::ordered_json::object();

    try {
        auto pub = EstimatorPub(circ, observables);
        (void)pub;
    } catch (const std::invalid_argument&) {
        return Ok;
    }

    std::cerr << "  estimator empty JSON observable test : empty map was not rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_rejects_nested_observable_array(void)
{
    auto circ = QuantumCircuit(1, 0);
    nlohmann::ordered_json nested = nlohmann::ordered_json::array();
    nested.push_back("Z");
    nested.push_back("X");
    nlohmann::ordered_json observables = nlohmann::ordered_json::array();
    observables.push_back(nested);

    try {
        auto pub = EstimatorPub(circ, observables);
        (void)pub;
    } catch (const std::invalid_argument&) {
        return Ok;
    }

    std::cerr << "  estimator nested observable array test : nested array was not rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_rejects_empty_pauli_terms(void)
{
    auto circ = QuantumCircuit(1, 0);

    try {
        std::vector<std::pair<std::string, double>> real_terms;
        auto pub = EstimatorPub(circ, real_terms);
        (void)pub;
    } catch (const std::invalid_argument&) {
        try {
            std::vector<std::pair<std::string, std::complex<double>>> complex_terms;
            auto pub = EstimatorPub(circ, complex_terms);
            (void)pub;
        } catch (const std::invalid_argument&) {
            return Ok;
        }
    }

    std::cerr << "  estimator empty Pauli terms test : empty terms were not rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_rejects_zero_sparse_observable(void)
{
    auto circ = QuantumCircuit(1, 0);

    try {
        auto observable = SparseObservable::zero(1);
        auto pub = EstimatorPub(circ, observable);
        (void)pub;
    } catch (const std::invalid_argument&) {
        return Ok;
    }

    std::cerr << "  estimator zero SparseObservable test : zero observable was not rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_rejects_bad_precision(void)
{
    auto circ = QuantumCircuit(1, 0);
    try {
        auto pub = EstimatorPub(circ, std::string("Z"), std::nan(""));
        (void)pub;
    } catch (const std::invalid_argument&) {
        return Ok;
    }

    std::cerr << "  estimator precision test : NaN precision was not rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_rejects_width_mismatch(void)
{
    auto circ = QuantumCircuit(2, 0);
    try {
        auto pub = EstimatorPub(circ, std::string("ZZZ"));
        (void)pub;
    } catch (const std::invalid_argument&) {
        return Ok;
    }

    std::cerr << "  estimator width test : width mismatch was not rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_rejects_measured_circuit(void)
{
    auto circ = QuantumCircuit(1, 1);
    circ.measure(0, 0);
    auto pub = EstimatorPub(circ, std::string("Z"));
    nlohmann::ordered_json results = nlohmann::ordered_json::array();
    results.push_back(estimator_result(1.0, 0.0, 100, 0.02));
    MockEstimatorBackend backend(results);
    auto estimator = BackendEstimatorV2(backend);

    try {
        estimator.run({pub});
    } catch (const std::invalid_argument&) {
        return Ok;
    }

    std::cerr << "  estimator measured circuit test : measurement was not rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_job_result(void)
{
    auto circ = QuantumCircuit(1, 0);
    auto pub = EstimatorPub(circ, std::string("Z"));
    nlohmann::ordered_json results = nlohmann::ordered_json::array();
    results.push_back(estimator_result(0.75, 0.125, 4096, 0.02));
    MockEstimatorBackend backend(results);
    auto estimator = BackendEstimatorV2(backend);

    auto job = estimator.run({pub}, 0.02);
    if (!job) {
        std::cerr << "  estimator job result test : estimator returned nullptr job." << std::endl;
        return NullptrError;
    }

    auto primitive_result = job->result();
    if (primitive_result.size() != 1 || primitive_result[0].evs().size() != 1 || primitive_result[0].stds().size() != 1) {
        std::cerr << "  estimator job result test : unexpected result shape." << std::endl;
        return EqualityError;
    }
    if (std::abs(primitive_result[0].evs()[0] - 0.75) > 1e-12 ||
        std::abs(primitive_result[0].stds()[0] - 0.125) > 1e-12 ||
        backend.received_pubs_.size() != 1 ||
        !backend.received_pubs_[0].has_precision() ||
        std::abs(backend.received_pubs_[0].precision() - 0.02) > 1e-12) {
        std::cerr << "  estimator job result test : unexpected values." << std::endl;
        return EqualityError;
    }

    return Ok;
}

static int test_estimator_result_accepts_one_observable_scalar(void)
{
    auto circ = QuantumCircuit(1, 0);
    auto pub = EstimatorPub(circ, std::string("Z"));
    auto result = EstimatorPubResult(pub);

    nlohmann::ordered_json input;
    input["data"]["evs"] = 0.5;
    input["data"]["stds"] = 0.125;

    if (result.from_json(input) && result.evs().size() == 1 && result.stds().size() == 1) {
        return Ok;
    }

    std::cerr << "  estimator scalar result test : one-observable scalar result was rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_result_accepts_one_observable_array(void)
{
    auto circ = QuantumCircuit(1, 0);
    auto pub = EstimatorPub(circ, std::string("Z"));
    auto result = EstimatorPubResult(pub);

    nlohmann::ordered_json input;
    input["data"]["evs"] = nlohmann::ordered_json::array({0.5});
    input["data"]["stds"] = nlohmann::ordered_json::array({0.125});

    if (result.from_json(input) && result.evs().size() == 1 && result.stds().size() == 1) {
        return Ok;
    }

    std::cerr << "  estimator one-observable array result test : result was rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_result_accepts_multi_observable_array(void)
{
    auto circ = QuantumCircuit(1, 0);
    nlohmann::ordered_json observables = nlohmann::ordered_json::array();
    observables.push_back("Z");
    observables.push_back("X");
    auto pub = EstimatorPub(circ, observables);
    auto result = EstimatorPubResult(pub);

    nlohmann::ordered_json input;
    input["data"]["evs"] = nlohmann::ordered_json::array({0.5, -0.25});
    input["data"]["stds"] = nlohmann::ordered_json::array({0.125, 0.0625});

    if (result.from_json(input) && result.evs().size() == 2 && result.stds().size() == 2) {
        return Ok;
    }

    std::cerr << "  estimator multi-observable result test : valid result was rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_result_rejects_too_few_evs(void)
{
    auto circ = QuantumCircuit(1, 0);
    nlohmann::ordered_json observables = nlohmann::ordered_json::array();
    observables.push_back("Z");
    observables.push_back("X");
    auto pub = EstimatorPub(circ, observables);
    auto result = EstimatorPubResult(pub);

    nlohmann::ordered_json input;
    input["data"]["evs"] = nlohmann::ordered_json::array({0.5});

    if (!result.from_json(input)) {
        return Ok;
    }

    std::cerr << "  estimator too-few evs test : short evs array was not rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_result_rejects_too_many_evs(void)
{
    auto circ = QuantumCircuit(1, 0);
    auto pub = EstimatorPub(circ, std::string("Z"));
    auto result = EstimatorPubResult(pub);

    nlohmann::ordered_json input;
    input["data"]["evs"] = nlohmann::ordered_json::array({0.5, -0.25});

    if (!result.from_json(input)) {
        return Ok;
    }

    std::cerr << "  estimator too-many evs test : long evs array was not rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_result_rejects_stds_mismatch(void)
{
    auto circ = QuantumCircuit(1, 0);
    nlohmann::ordered_json observables = nlohmann::ordered_json::array();
    observables.push_back("Z");
    observables.push_back("X");
    auto pub = EstimatorPub(circ, observables);
    auto result = EstimatorPubResult(pub);

    nlohmann::ordered_json input;
    input["data"]["evs"] = nlohmann::ordered_json::array({0.5, -0.25});
    input["data"]["stds"] = nlohmann::ordered_json::array({0.125});

    if (!result.from_json(input)) {
        return Ok;
    }

    std::cerr << "  estimator stds mismatch test : mismatched stds were not rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_result_rejects_nested_evs(void)
{
    auto circ = QuantumCircuit(1, 0);
    auto pub = EstimatorPub(circ, std::string("Z"));
    auto result = EstimatorPubResult(pub);

    nlohmann::ordered_json input;
    input["data"]["evs"] = nlohmann::ordered_json::array();
    input["data"]["evs"].push_back(nlohmann::ordered_json::array({1.0}));

    if (!result.from_json(input)) {
        return Ok;
    }

    std::cerr << "  estimator nested evs test : nested evs were not rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_result_rejects_bad_metadata(void)
{
    auto circ = QuantumCircuit(1, 0);
    auto pub = EstimatorPub(circ, std::string("Z"));
    auto result = EstimatorPubResult(pub);

    nlohmann::ordered_json input;
    input["data"]["evs"] = 1.0;
    input["metadata"]["shots"] = -1;

    if (!result.from_json(input)) {
        return Ok;
    }

    std::cerr << "  estimator metadata test : negative shots were not rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_result_rejects_bad_target_precision(void)
{
    auto circ = QuantumCircuit(1, 0);
    auto pub = EstimatorPub(circ, std::string("Z"));
    auto result = EstimatorPubResult(pub);

    nlohmann::ordered_json input;
    input["data"]["evs"] = 1.0;
    input["metadata"]["target_precision"] = -0.1;

    if (!result.from_json(input)) {
        return Ok;
    }

    std::cerr << "  estimator metadata test : negative target_precision was not rejected." << std::endl;
    return EqualityError;
}

static bool estimator_job_result_throws_for_status(JobStatus status)
{
    auto circ = QuantumCircuit(1, 0);
    auto pub = EstimatorPub(circ, std::string("Z"));
    nlohmann::ordered_json results = nlohmann::ordered_json::array();
    results.push_back(estimator_result(1.0, 0.0, 100, 0.02));
    MockEstimatorBackend backend(results, status);
    auto estimator = BackendEstimatorV2(backend);
    auto job = estimator.run({pub}, 0.02);

    try {
        auto result = job->result();
        (void)result;
    } catch (const std::runtime_error&) {
        return true;
    }

    return false;
}

static int test_estimator_job_rejects_error_state(void)
{
    if (estimator_job_result_throws_for_status(JobStatus::ERROR)) {
        return Ok;
    }

    std::cerr << "  estimator error state test : ERROR state produced a result." << std::endl;
    return EqualityError;
}

static int test_estimator_job_rejects_failed_state(void)
{
    if (estimator_job_result_throws_for_status(JobStatus::FAILED)) {
        return Ok;
    }

    std::cerr << "  estimator failed state test : FAILED state produced a result." << std::endl;
    return EqualityError;
}

static int test_estimator_job_rejects_cancelled_state(void)
{
    if (estimator_job_result_throws_for_status(JobStatus::CANCELLED)) {
        return Ok;
    }

    std::cerr << "  estimator cancelled state test : CANCELLED state produced a result." << std::endl;
    return EqualityError;
}

static int test_estimator_job_rejects_provider_read_failure(void)
{
    auto circ = QuantumCircuit(1, 0);
    auto pub = EstimatorPub(circ, std::string("Z"));
    nlohmann::ordered_json results = nlohmann::ordered_json::array();
    results.push_back(estimator_result(1.0, 0.0, 100, 0.02));
    MockEstimatorBackend backend(results);
    backend.set_fail_result(true);
    auto estimator = BackendEstimatorV2(backend);
    auto job = estimator.run({pub}, 0.02);

    try {
        auto result = job->result();
        (void)result;
    } catch (const std::runtime_error&) {
        return Ok;
    }

    std::cerr << "  estimator provider read failure test : failed read produced a result." << std::endl;
    return EqualityError;
}

static int test_estimator_job_rejects_result_count_mismatch(void)
{
    auto circ = QuantumCircuit(1, 0);
    auto pub = EstimatorPub(circ, std::string("Z"));
    nlohmann::ordered_json results = nlohmann::ordered_json::array();
    results.push_back(estimator_result(1.0, 0.0, 100, 0.02));
    MockEstimatorBackend backend(results);
    backend.set_reported_num_results(2);
    auto estimator = BackendEstimatorV2(backend);
    auto job = estimator.run({pub}, 0.02);

    try {
        auto result = job->result();
        (void)result;
    } catch (const std::runtime_error&) {
        return Ok;
    }

    std::cerr << "  estimator result count mismatch test : mismatched count produced a result." << std::endl;
    return EqualityError;
}

#if defined(_WIN32)
int test_estimator(int argc, char** const argv)
#else
int test_estimator(int argc, char** argv)
#endif
{
    int num_failed = 0;
    num_failed += RUN_TEST(test_estimator_pub_to_json);
    num_failed += RUN_TEST(test_estimator_qrmi_payload_contract);
    num_failed += RUN_TEST(test_estimator_qrmi_payload_rejects_measured_circuit);
    num_failed += RUN_TEST(test_estimator_pub_sums_duplicate_labels);
    num_failed += RUN_TEST(test_estimator_pub_duplicate_labels_match_observable);
    num_failed += RUN_TEST(test_estimator_pub_sums_duplicate_complex_labels);
    num_failed += RUN_TEST(test_estimator_pub_duplicate_labels_cancel);
    num_failed += RUN_TEST(test_estimator_pub_sparse_observable);
    num_failed += RUN_TEST(test_estimator_rejects_empty_json_observable_map);
    num_failed += RUN_TEST(test_estimator_rejects_nested_observable_array);
    num_failed += RUN_TEST(test_estimator_rejects_empty_pauli_terms);
    num_failed += RUN_TEST(test_estimator_rejects_zero_sparse_observable);
    num_failed += RUN_TEST(test_estimator_rejects_bad_precision);
    num_failed += RUN_TEST(test_estimator_rejects_width_mismatch);
    num_failed += RUN_TEST(test_estimator_rejects_measured_circuit);
    num_failed += RUN_TEST(test_estimator_job_result);
    num_failed += RUN_TEST(test_estimator_result_accepts_one_observable_scalar);
    num_failed += RUN_TEST(test_estimator_result_accepts_one_observable_array);
    num_failed += RUN_TEST(test_estimator_result_accepts_multi_observable_array);
    num_failed += RUN_TEST(test_estimator_result_rejects_too_few_evs);
    num_failed += RUN_TEST(test_estimator_result_rejects_too_many_evs);
    num_failed += RUN_TEST(test_estimator_result_rejects_stds_mismatch);
    num_failed += RUN_TEST(test_estimator_result_rejects_nested_evs);
    num_failed += RUN_TEST(test_estimator_result_rejects_bad_metadata);
    num_failed += RUN_TEST(test_estimator_result_rejects_bad_target_precision);
    num_failed += RUN_TEST(test_estimator_job_rejects_error_state);
    num_failed += RUN_TEST(test_estimator_job_rejects_failed_state);
    num_failed += RUN_TEST(test_estimator_job_rejects_cancelled_state);
    num_failed += RUN_TEST(test_estimator_job_rejects_provider_read_failure);
    num_failed += RUN_TEST(test_estimator_job_rejects_result_count_mismatch);

    std::cerr << "=== Number of failed subtests: " << num_failed << std::endl;

    return num_failed;
}
