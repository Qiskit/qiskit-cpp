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

// estimator implementation for a backend

#ifndef __qiskitcpp_primitives_backend_estimator_v2_hpp__
#define __qiskitcpp_primitives_backend_estimator_v2_hpp__

#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "primitives/backend_estimator_job.hpp"
#include "primitives/containers/estimator_pub.hpp"
#include "primitives/containers/sampler_pub.hpp"
#include "primitives/detail/estimator_measurement.hpp"
#include "providers/backend.hpp"

namespace Qiskit {
namespace primitives {

/// @class BackendEstimatorV2
/// @brief Implementation of EstimatorV2 on a sampler-backed backend.
class BackendEstimatorV2 {
protected:
    providers::BackendV2& backend_;
    double default_precision_;
    bool abelian_grouping_;

    std::shared_ptr<BackendEstimatorJob> submit_entries_by_shots(
        std::vector<EstimatorPub>& pubs,
        const std::vector<BackendEstimatorJob::PubState>& states,
        std::vector<BackendEstimatorJob::JobEntry>& entries)
    {
        std::vector<bool> submitted(entries.size(), false);
        std::vector<BackendEstimatorJob::JobEntry> submitted_entries;

        for (uint_t entry_index = 0; entry_index < entries.size(); entry_index++) {
            if (submitted[entry_index])
                continue;

            uint_t shots = entries[entry_index].task.shots;
            std::vector<SamplerPub> sampler_pubs;
            std::vector<uint_t> bucket_entries;

            for (uint_t candidate = entry_index; candidate < entries.size(); candidate++) {
                if (!submitted[candidate] && entries[candidate].task.shots == shots) {
                    SamplerPub sampler_pub(entries[candidate].task.measured_circuit);
                    sampler_pubs.push_back(sampler_pub);
                    bucket_entries.push_back(candidate);
                    submitted[candidate] = true;
                }
            }

            auto provider_job = backend_.run(sampler_pubs, shots);
            if (!provider_job) {
                return std::shared_ptr<BackendEstimatorJob>(
                    new BackendEstimatorJob(pubs, states, submitted_entries, "BackendEstimatorV2: backend returned null job"));
            }

            for (uint_t result_index = 0; result_index < bucket_entries.size(); result_index++) {
                entries[bucket_entries[result_index]].job = provider_job;
                entries[bucket_entries[result_index]].result_index = result_index;
                submitted_entries.push_back(entries[bucket_entries[result_index]]);
            }
        }

        return std::shared_ptr<BackendEstimatorJob>(new BackendEstimatorJob(pubs, states, entries));
    }

public:
    /// @brief Create a new BackendEstimatorV2.
    /// @param backend backend to run measurement sampler jobs.
    /// @param default_precision default target precision.
    /// @param abelian_grouping whether to group qubit-wise compatible Pauli terms.
    BackendEstimatorV2(providers::BackendV2& backend,
                       double default_precision = 0.015625,
                       bool abelian_grouping = true)
        : backend_(backend),
          default_precision_(default_precision),
          abelian_grouping_(abelian_grouping)
    {
        if (!std::isfinite(default_precision_) || default_precision_ <= 0.0) {
            throw std::invalid_argument("BackendEstimatorV2: precision must be greater than 0");
        }
    }

    /// @brief return reference to backend object.
    const providers::BackendV2& backend(void) const
    {
        return backend_;
    }

    /// @brief Run estimator pubs.
    /// @details Input circuits with existing measurements are reported as unsupported.
    /// @param pubs An iterable of estimator pub-like objects.
    /// @param precision call-level precision. Zero means unset.
    /// @return BackendEstimatorJob
    /// @throws std::invalid_argument if an input circuit already contains measurements.
    std::shared_ptr<BackendEstimatorJob> run(std::vector<EstimatorPub> pubs,
                                             double precision = 0.0)
    {
        if (!std::isfinite(precision) || precision < 0.0) {
            throw std::invalid_argument("BackendEstimatorV2: precision must be non-negative");
        }

        std::vector<BackendEstimatorJob::PubState> states(pubs.size());
        std::vector<BackendEstimatorJob::JobEntry> entries;

        for (uint_t pub_index = 0; pub_index < pubs.size(); pub_index++) {
            if (!pubs[pub_index].circuit().get_measure_map().empty()) {
                throw std::invalid_argument("Estimator measurement: input circuit already contains measurements");
            }
            double target_precision = estimator_detail::resolve_precision(
                pubs[pub_index].precision(), precision, default_precision_);
            uint_t shots = estimator_detail::shots_from_precision(target_precision);
            uint_t num_observables = pubs[pub_index].observables().size();
            states[pub_index].evs.resize(num_observables, 0.0);
            states[pub_index].std_numerators.resize(num_observables, 0.0);
            states[pub_index].shots = shots;
            states[pub_index].target_precision = target_precision;

            std::vector<estimator_detail::PauliTerm> terms;
            for (uint_t obs_index = 0; obs_index < num_observables; obs_index++) {
                if (pubs[pub_index].observables()[obs_index].num_qubits()
                    != pubs[pub_index].circuit().num_qubits()) {
                    throw std::invalid_argument("BackendEstimatorV2: observable/circuit width mismatch");
                }
                auto extracted = estimator_detail::extract_pauli_terms(
                    pubs[pub_index].observables()[obs_index], obs_index);
                for (uint_t term_index = 0; term_index < extracted.size(); term_index++) {
                    if (extracted[term_index].factors.empty()) {
                        states[pub_index].evs[obs_index] += extracted[term_index].coeff.real();
                    } else {
                        terms.push_back(extracted[term_index]);
                    }
                }
            }

            auto groups = estimator_detail::group_pauli_terms(
                terms, pubs[pub_index].circuit().num_qubits(), abelian_grouping_);
            for (uint_t group_index = 0; group_index < groups.size(); group_index++) {
                BackendEstimatorJob::JobEntry entry;
                entry.result_index = 0;
                entry.task.pub_index = pub_index;
                entry.task.group_index = group_index;
                entry.task.shots = shots;
                entry.task.group = groups[group_index];
                entry.task.measured_circuit = estimator_detail::make_measurement_circuit(
                    pubs[pub_index].circuit(), groups[group_index].basis);

                entries.push_back(entry);
            }
        }

        return submit_entries_by_shots(pubs, states, entries);
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_backend_estimator_v2_hpp__
