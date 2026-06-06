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

// job class for BackendEstimator

#ifndef __qiskitcpp_primitives_backend_estimator_job_hpp__
#define __qiskitcpp_primitives_backend_estimator_job_hpp__

#ifdef _MSC_VER
#define NOMINMAX
#include <windows.h>
#else
#include <chrono>
#include <thread>
#endif

#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "primitives/containers/estimator_result.hpp"
#include "primitives/containers/sampler_pub.hpp"
#include "primitives/detail/estimator_measurement.hpp"
#include "providers/job.hpp"

namespace Qiskit {
namespace primitives {

/// @class BackendEstimatorJob
/// @brief Job class for Backend Estimator primitive.
class BackendEstimatorJob {
    friend class BackendEstimatorV2;
private:
    struct PubState {
        std::vector<double> evs;
        std::vector<double> std_numerators;
        uint_t shots;
        double target_precision;
    };

    struct JobEntry {
        std::shared_ptr<providers::Job> job;
        uint_t result_index;
        estimator_detail::MeasurementTask task;
    };

    std::vector<EstimatorPub> pubs_;
    std::vector<PubState> states_;
    std::vector<JobEntry> entries_;
    bool has_result_ = false;
    bool has_submission_error_ = false;
    std::string submission_error_;
    EstimatorResult cached_result_;

    bool is_final_status(providers::JobStatus status)
    {
        return status == providers::JobStatus::DONE
               || status == providers::JobStatus::CANCELLED
               || status == providers::JobStatus::FAILED
               || status == providers::JobStatus::ERROR;
    }

    std::string status_name(providers::JobStatus status)
    {
        switch (status) {
        case providers::JobStatus::INITIALIZING:
            return "INITIALIZING";
        case providers::JobStatus::QUEUED:
            return "QUEUED";
        case providers::JobStatus::VALIDATING:
            return "VALIDATING";
        case providers::JobStatus::RUNNING:
            return "RUNNING";
        case providers::JobStatus::CANCELLED:
            return "CANCELLED";
        case providers::JobStatus::DONE:
            return "DONE";
        case providers::JobStatus::ERROR:
            return "ERROR";
        case providers::JobStatus::FAILED:
            return "FAILED";
        }
        return "UNKNOWN";
    }

    /// @brief Create a new BackendEstimatorJob.
    BackendEstimatorJob(std::vector<EstimatorPub>& pubs,
                        const std::vector<PubState>& states,
                        const std::vector<JobEntry>& entries,
                        const std::string& submission_error = "")
        : pubs_(pubs),
          states_(states),
          entries_(entries),
          has_submission_error_(!submission_error.empty()),
          submission_error_(submission_error)
    {
    }

public:
    /// @brief Return the aggregate status of the job.
    providers::JobStatus status(void)
    {
        if (has_submission_error_)
            return providers::JobStatus::ERROR;
        if (entries_.empty())
            return providers::JobStatus::DONE;
        bool has_initializing = false;
        bool has_queued = false;
        bool has_validating = false;
        bool has_running = false;
        bool has_cancelled = false;
        bool has_failed = false;
        bool has_error = false;
        for (uint_t i = 0; i < entries_.size(); i++) {
            auto st = entries_[i].job->status();
            if (st == providers::JobStatus::ERROR)
                has_error = true;
            else if (st == providers::JobStatus::FAILED)
                has_failed = true;
            else if (st == providers::JobStatus::CANCELLED)
                has_cancelled = true;
            else if (st == providers::JobStatus::RUNNING)
                has_running = true;
            else if (st == providers::JobStatus::QUEUED)
                has_queued = true;
            else if (st == providers::JobStatus::VALIDATING)
                has_validating = true;
            else if (st != providers::JobStatus::DONE)
                has_initializing = true;
        }
        if (has_error)
            return providers::JobStatus::ERROR;
        if (has_failed)
            return providers::JobStatus::FAILED;
        if (has_cancelled)
            return providers::JobStatus::CANCELLED;
        if (has_running)
            return providers::JobStatus::RUNNING;
        if (has_queued)
            return providers::JobStatus::QUEUED;
        if (has_validating)
            return providers::JobStatus::VALIDATING;
        return has_initializing ? providers::JobStatus::INITIALIZING : providers::JobStatus::DONE;
    }

    /// @brief Return whether any provider job is actively running.
    bool running(void)
    {
        return status() == providers::JobStatus::RUNNING;
    }

    /// @brief Return whether any provider job is queued or validating.
    bool queued(void)
    {
        auto st = status();
        return st == providers::JobStatus::QUEUED || st == providers::JobStatus::VALIDATING;
    }

    /// @brief Return whether all provider jobs are done.
    bool done(void)
    {
        return status() == providers::JobStatus::DONE;
    }

    /// @brief Return whether any provider job has been cancelled.
    bool cancelled(void)
    {
        return status() == providers::JobStatus::CANCELLED;
    }

    /// @brief Return whether every provider job is in a final state.
    bool in_final_state(void)
    {
        return is_final_status(status());
    }

    /// @brief Attempt to cancel the job.
    bool cancel(void)
    {
        return false;
    }

    /// @brief Wait for provider jobs and return estimator results.
    EstimatorResult result(void)
    {
        if (has_result_)
            return cached_result_;
        if (has_submission_error_) {
            throw std::runtime_error(submission_error_);
        }

        std::vector<PubState> states = states_;
        for (uint_t i = 0; i < entries_.size(); i++) {
            providers::JobStatus st;
            while (true) {
                st = entries_[i].job->status();
                if (is_final_status(st))
                    break;
#ifdef _MSC_VER
                Sleep(1);
#else
                std::this_thread::sleep_for(std::chrono::seconds(1));
#endif
            }
            if (st != providers::JobStatus::DONE) {
                throw std::runtime_error("BackendEstimatorJob: provider job ended with status " + status_name(st));
            }
            if (entries_[i].job->num_results() <= entries_[i].result_index) {
                throw std::runtime_error("BackendEstimatorJob: provider result count mismatch");
            }

            circuit::QuantumCircuit result_circuit = entries_[i].task.measured_circuit.copy();
            SamplerPub sampler_pub(result_circuit);
            SamplerPubResult sampler_result(sampler_pub);
            if (!entries_[i].job->result(entries_[i].result_index, sampler_result)) {
                throw std::runtime_error("BackendEstimatorJob: provider result returned false");
            }

            BitArray& bits = sampler_result.data();
            if (bits.num_shots() == 0) {
                throw std::runtime_error("BackendEstimatorJob: empty samples for measurement task");
            }
            if (bits.num_shots() != entries_[i].task.shots) {
                throw std::runtime_error("BackendEstimatorJob: sample count mismatch");
            }
            if (bits.num_bits() != entries_[i].task.measured_circuit.num_clbits()) {
                throw std::runtime_error("BackendEstimatorJob: sample bit width mismatch");
            }
            for (uint_t shot = 0; shot < bits.num_shots(); shot++) {
                if (bits[shot].size() != bits.num_bits()) {
                    throw std::runtime_error("BackendEstimatorJob: sample bit width mismatch");
                }
                for (uint_t bit = 0; bit < bits.num_bits(); bit++) {
                    if (bits[shot][bit] > 1) {
                        throw std::runtime_error("BackendEstimatorJob: non-binary sample value");
                    }
                }
            }

            uint_t pub_index = entries_[i].task.pub_index;
            for (uint_t term_index = 0; term_index < entries_[i].task.group.terms.size(); term_index++) {
                const estimator_detail::PauliTerm& term = entries_[i].task.group.terms[term_index];
                auto stats = estimator_detail::pauli_statistics(bits, term);
                double variance = stats.variance < 0.0 ? 0.0 : stats.variance;
                states[pub_index].evs[term.observable_index] += term.coeff.real() * stats.mean;
                states[pub_index].std_numerators[term.observable_index] += std::fabs(term.coeff.real()) * std::sqrt(variance);
            }
        }

        EstimatorResult result;
        for (uint_t pub_index = 0; pub_index < pubs_.size(); pub_index++) {
            std::vector<double> stds(states[pub_index].std_numerators.size(), 0.0);
            for (uint_t obs_index = 0; obs_index < stds.size(); obs_index++) {
                stds[obs_index] = estimator_detail::estimator_std_from_numerator(
                    states[pub_index].std_numerators[obs_index],
                    states[pub_index].shots);
            }
            EstimatorPubResult pub_result(pubs_[pub_index],
                                          states[pub_index].evs,
                                          stds,
                                          states[pub_index].shots,
                                          states[pub_index].target_precision);
            result.push_back(pub_result);
        }
        states_ = states;
        cached_result_ = result;
        has_result_ = true;
        return result;
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_backend_estimator_job_hpp__
