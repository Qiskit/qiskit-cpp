/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2017, 2024.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.
*/

// job class for BackendEstimator

#ifndef __qiskitcpp_primitives_backend_estimator_job_def_hpp__
#define __qiskitcpp_primitives_backend_estimator_job_def_hpp__

#define _USE_MATH_DEFINES
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "compiler/transpiler.hpp"
#include "primitives/backend_sampler_v2.hpp"
#include "primitives/containers/estimator_pub.hpp"
#include "primitives/containers/estimator_result.hpp"
#include "primitives/containers/sampler_pub.hpp"
#include "providers/backend.hpp"
#include "providers/jobstatus.hpp"

namespace Qiskit {
namespace primitives {

namespace {

int bit_value_from_string(const std::string& bitstring, uint_t qubit, uint_t num_qubits)
{
    uint_t pos = num_qubits - 1 - qubit;
    return bitstring[pos] == '1' ? 1 : 0;
}

double pauli_eigenvalue_from_bitstring(const std::string& pauli, const std::string& bitstring)
{
    double value = 1.0;
    uint_t n = pauli.size();

    for (uint_t q = 0; q < n; q++) {
        char p = pauli[n - 1 - q];

        if (p == 'I') {
            continue;
        }

        int bit = bit_value_from_string(bitstring, q, n);
        value *= (bit == 0) ? 1.0 : -1.0;
    }

    return value;
}

void append_basis_rotation(circuit::QuantumCircuit& circ, const std::string& pauli)
{
    uint_t n = pauli.size();

    for (uint_t q = 0; q < n; q++) {
        char p = pauli[n - 1 - q];

        if (p == 'X') {
            circ.h(q);
        } else if (p == 'Y') {
            circ.rz(-M_PI / 2.0, q);
            circ.h(q);
        }
    }
}

void append_measurements(circuit::QuantumCircuit& circ, uint_t num_qubits)
{
    for (uint_t q = 0; q < num_qubits; q++) {
        circ.measure(q, q);
    }
}

EstimatorPubResult estimate_pub(providers::BackendV2& backend,
                              uint_t default_shots,
                              EstimatorPub& pub)
{
    BackendSamplerV2 sampler(backend, default_shots);
    double total_ev = 0.0;
    double variance_accum = 0.0;
    uint_t shots = pub.shots() > 0 ? pub.shots() : default_shots;

    for (const auto& term : pub.observables()) {
        const std::string& pauli = term.first;
        double coeff = term.second;
        uint_t num_qubits = pauli.size();

        auto meas_circ = pub.circuit().copy();
        append_basis_rotation(meas_circ, pauli);

        if (meas_circ.get_measure_map().empty()) {
            append_measurements(meas_circ, num_qubits);
        }

        auto isa_circ = compiler::transpile(meas_circ, backend);
        SamplerPub sampler_pub(isa_circ, shots);

        auto job = sampler.run({sampler_pub});
        if (job == nullptr) {
            continue;
        }

        auto primitive_result = job->result();
        auto pub_result = primitive_result[0];
        auto bits = pub_result.data().get_bitstrings();

        if (bits.empty()) {
            continue;
        }

        double mean = 0.0;
        for (const auto& b : bits) {
            mean += pauli_eigenvalue_from_bitstring(pauli, b);
        }
        mean /= static_cast<double>(bits.size());

        total_ev += coeff * mean;
        variance_accum += coeff * coeff * (1.0 - mean * mean) / static_cast<double>(bits.size());
    }

    EstimatorPubResult result(pub);
    result.set_ev(total_ev);
    result.set_stddev(std::sqrt(variance_accum));
    return result;
}

} // namespace

/// @class BackendEstimatorJob
/// @brief Job class for Backend Estimator primitive.
class BackendEstimatorJob {
protected:
    providers::BackendV2& backend_;
    uint_t shots_;
    std::vector<EstimatorPub> pubs_;
    EstimatorResult result_;
    bool finished_ = false;
public:
    BackendEstimatorJob(providers::BackendV2& backend,
                        uint_t shots,
                        std::vector<EstimatorPub>& pubs)
        : backend_(backend), shots_(shots), pubs_(pubs)
    {
    }

    BackendEstimatorJob(const BackendEstimatorJob& other)
        : backend_(other.backend_),
          shots_(other.shots_),
          pubs_(other.pubs_),
          result_(other.result_),
          finished_(other.finished_)
    {
    }

    const std::vector<EstimatorPub>& pubs(void) const
    {
        return pubs_;
    }

    providers::JobStatus status(void)
    {
        return finished_ ? providers::JobStatus::DONE : providers::JobStatus::RUNNING;
    }

    bool running(void)
    {
        return !finished_;
    }

    bool done(void)
    {
        return finished_;
    }

    bool cancelled(void)
    {
        return false;
    }

    bool in_final_state(void)
    {
        return finished_;
    }

    bool cancel(void)
    {
        return false;
    }

    EstimatorResult result(void)
    {
        if (!finished_) {
            result_.allocate(pubs_.size());
            for (uint_t i = 0; i < pubs_.size(); i++) {
                result_[i] = estimate_pub(backend_, shots_, pubs_[i]);
            }
            finished_ = true;
        }
        return result_;
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_backend_estimator_job_def_hpp__
