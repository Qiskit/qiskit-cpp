/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2017, 2026.
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

#include <chrono>
#include <thread>

#include <memory>
#include <stdexcept>
#include <vector>

#include "primitives/base/base_primitive_job.hpp"
#include "primitives/containers/estimator_primitive_result.hpp"
#include "providers/job.hpp"

// Windows headers can define ERROR after jobstatus.hpp has already been parsed.
#ifdef ERROR
#undef ERROR
#endif

namespace Qiskit {
namespace primitives {

/// @class BackendEstimatorJob
/// @brief Job class for Backend Estimator primitive.
class BackendEstimatorJob : public BasePrimitiveJob {
protected:
    std::vector<EstimatorPub> pubs_;
public:
    /// @brief Create a new BackendEstimatorJob.
    /// @param job a job pointer to run pubs.
    /// @param pubs a list of pubs.
    BackendEstimatorJob(std::shared_ptr<providers::Job> job, std::vector<EstimatorPub>& pubs) : BasePrimitiveJob(job)
    {
        pubs_ = pubs;
    }

    BackendEstimatorJob(std::shared_ptr<providers::Job> job, const nlohmann::ordered_json& pubs) : BasePrimitiveJob(job)
    {
        for (size_t i = 0; i < pubs.size(); i++) {
            pubs_.push_back(EstimatorPub(pubs[i]));
        }
    }

    BackendEstimatorJob(const BackendEstimatorJob& other) : BasePrimitiveJob(other)
    {
        pubs_ = other.pubs_;
    }

    ~BackendEstimatorJob()
    {
    }

    /// @brief get pubs for this job.
    /// @return a list of pub.
    const std::vector<EstimatorPub>& pubs(void) const
    {
        return pubs_;
    }

    /// @brief Return the status of the job.
    /// @return JobStatus enum.
    providers::JobStatus status(void) override
    {
        if (!job_) {
            return providers::JobStatus::ERROR;
        }
        return job_->status();
    }

    /// @brief Return whether the job is actively running.
    /// @return true if job is actively running, otherwise false.
    bool running(void)  override
    {
        return status() == providers::JobStatus::RUNNING;
    }

    /// @brief Return whether the job is queued.
    /// @return true if job is queued, otherwise false.
    bool queued(void)
    {
        return status() == providers::JobStatus::QUEUED;
    }

    /// @brief Return whether the job has successfully run.
    /// @return true if successfully run, otherwise false.
    bool done(void) override
    {
        return status() == providers::JobStatus::DONE;
    }

    /// @brief Return whether the job has been cancelled.
    /// @return true if job has been cancelled, otherwise false.
    bool cancelled(void) override
    {
        return status() == providers::JobStatus::CANCELLED;
    }

    /// @brief Return whether the job is in a final job state such as DONE or ERROR.
    /// @return true if job is in a final job state, otherwise false.
    bool in_final_state(void) override
    {
        return is_final_state(status());
    }

    /// @brief Attempt to cancel the job.
    /// @return false because backend cancellation is not implemented.
    bool cancel(void) override
    {
        return false;
    }

    /// @brief Return the results of the job.
    /// @return estimator primitive result.
    EstimatorPrimitiveResult result(void)
    {
        if (!job_) {
            throw std::runtime_error("BackendEstimatorJob has no provider job.");
        }

        providers::JobStatus st;
        while (true) {
            st = job_->status();
            if (is_final_state(st)) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if (st != providers::JobStatus::DONE) {
            throw std::runtime_error("BackendEstimatorJob did not finish successfully.");
        }

        uint_t num_results = job_->num_results();
        if (num_results != pubs_.size()) {
            throw std::runtime_error("BackendEstimatorJob result count does not match the submitted pubs.");
        }

        EstimatorPrimitiveResult result;
        result.allocate(num_results);
        for (uint_t i = 0; i < num_results; i++) {
            result[i].set_pub(pubs_[i]);
            if (!job_->result(i, result[i])) {
                throw std::runtime_error("BackendEstimatorJob failed to read an estimator pub result.");
            }
        }
        return result;
    }

    /// @brief take a checkpoint for this job
    /// @param name filename to store the checkpoint
    /// @return true if successed
    bool checkpoint(const std::string& name) override
    {
        nlohmann::json checkpoint;
        checkpoint["job"] = job_->get_info();
        nlohmann::ordered_json pubs = nlohmann::ordered_json::array();

        for (auto p : pubs_) {
            pubs.push_back(p.to_json());
        }
        checkpoint["pubs"] = pubs;

        std::ofstream file(name);
        file << checkpoint;
        file.close();

        // do not release the session to keep alive on QC
        job_->keep_alive(true);
        return false;
    }

protected:
    static bool is_final_state(providers::JobStatus status)
    {
        return status == providers::JobStatus::DONE ||
               status == providers::JobStatus::CANCELLED ||
               status == providers::JobStatus::FAILED ||
               status == providers::JobStatus::ERROR;
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_backend_estimator_job_def_hpp__
