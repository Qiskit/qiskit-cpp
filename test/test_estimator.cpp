// This code is part of Qiskit.
//
// (C) Copyright IBM 2024.
//
// This code is licensed under the Apache License, Version 2.0. You may
// obtain a copy of this license in the LICENSE.txt file in the root directory
// of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
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

    auto pub = EstimatorPub(circ, terms, 0.02);
    auto payload = pub.to_json();

    if (payload.size() != 4 || !payload[1].is_object() || !payload[2].is_null()) {
        std::cerr << " estimator pub json test : unexpected PUB shape." << std::endl;
        return EqualityError;
    }
    if (std::abs(payload[1]["ZI"].get<double>() - 1.0) > 1e-12 ||
        std::abs(payload[1]["IZ"].get<double>() + 0.5) > 1e-12 ||
        std::abs(payload[3].get<double>() - 0.02) > 1e-12) {
        std::cerr << " estimator pub json test : unexpected observable or precision values." << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_qrmi_payload_contract(void)
{
    auto circ = QuantumCircuit(1, 0);
    std::vector<EstimatorPub> pubs;
    pubs.push_back(EstimatorPub(circ, std::string("Z")));
    pubs.push_back(EstimatorPub(circ, std::string("X"), 0.03125));

    auto payload = QRMIEstimatorPayload::build_estimator_input(pubs, 0.02);

    if (QRMIEstimatorPayload::estimator_program_id() != "estimator") {
        std::cerr << " estimator QRMI payload test : wrong program id." << std::endl;
        return EqualityError;
    }
    if (!payload.is_object() ||
        payload["version"].get<int>() != 2 ||
        payload["support_qiskit"].get<bool>() ||
        !payload["pubs"].is_array() ||
        payload["pubs"].size() != 2) {
        std::cerr << " estimator QRMI payload test : unexpected top-level shape." << std::endl;
        return EqualityError;
    }
    if (!pubs[0].has_precision() || std::abs(pubs[0].precision() - 0.02) > 1e-12) {
        std::cerr << " estimator QRMI payload test : precision resolution failed." << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_estimator_qrmi_payload_rejects_measured_circuit(void)
{
    auto circ = QuantumCircuit(1, 1);
    circ.measure(0, 0);
    std::vector<EstimatorPub> pubs;
    pubs.push_back(EstimatorPub(circ, std::string("Z")));

    try {
        QRMIEstimatorPayload::build_estimator_input(pubs, 0.02);
    } catch (const std::invalid_argument&) {
        return Ok;
    }

    std::cerr << " estimator QRMI payload measured circuit test : measurement was not rejected." << std::endl;
    return EqualityError;
}

static int test_estimator_pub_sparse_observable(void)
{
    auto circ = QuantumCircuit(2, 0);
    std::string label = "ZI";
    auto observable = SparseObservable::from_label(label);
    auto pub = EstimatorPub(circ, observable);
    auto payload = pub.to_json();

    if (!payload[1].is_object() || !payload[1].contains("ZI")) {
        std::cerr << " estimator pub sparse observable test : expected ZI observable." << std::endl;
        return EqualityError;
    }

    std::vector<std::pair<std::string, std::complex<double>>> terms;
    terms.push_back(std::make_pair(std::string("ZZ"), std::complex<double>(1.0)));
    terms.push_back(std::make_pair(std::string("XX"), std::complex<double>(0.5)));
    auto multi_term = SparseObservable::from_list(terms, 2);
    auto multi_pub = EstimatorPub(circ, multi_term);
    auto multi_payload = multi_pub.to_json();

    if (!multi_payload[1].is_object() ||
        !multi_payload[1].contains("ZZ") ||
        !multi_payload[1].contains("XX") ||
        std::abs(multi_payload[1]["ZZ"].get<double>() - 1.0) > 1e-12 ||
        std::abs(multi_payload[1]["XX"].get<double>() - 0.5) > 1e-12) {
        std::cerr << " estimator pub sparse observable test : expected ZZ and XX observables." << std::endl;
        return EqualityError;
    }
    return Ok;
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

    std::cerr << " estimator measured circuit test : measurement was not rejected." << std::endl;
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
        std::cerr << " estimator job result test : estimator returned nullptr job." << std::endl;
        return NullptrError;
    }

    auto primitive_result = job->result();
    if (primitive_result.size() != 1 || primitive_result[0].evs().size() != 1 || primitive_result[0].stds().size() != 1) {
        std::cerr << " estimator job result test : unexpected result shape." << std::endl;
        return EqualityError;
    }
    if (std::abs(primitive_result[0].evs()[0] - 0.75) > 1e-12 ||
        std::abs(primitive_result[0].stds()[0] - 0.125) > 1e-12) {
        std::cerr << " estimator job result test : unexpected values." << std::endl;
        return EqualityError;
    }
    return Ok;
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

    std::cerr << " estimator too-many evs test : long evs array was not rejected." << std::endl;
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

static int test_estimator_job_rejects_failed_state(void)
{
    if (estimator_job_result_throws_for_status(JobStatus::FAILED)) {
        return Ok;
    }
    std::cerr << " estimator failed state test : FAILED state produced a result." << std::endl;
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
    num_failed += RUN_TEST(test_estimator_pub_sparse_observable);
    num_failed += RUN_TEST(test_estimator_rejects_measured_circuit);
    num_failed += RUN_TEST(test_estimator_job_result);
    num_failed += RUN_TEST(test_estimator_result_rejects_too_many_evs);
    num_failed += RUN_TEST(test_estimator_job_rejects_failed_state);
    return num_failed;
}
