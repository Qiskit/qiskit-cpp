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

// job class for BackendSampler

#ifndef __qiskitcpp_primitives_qrmi_qiskit_ibm_runtime_job_def_hpp__
#define __qiskitcpp_primitives_qrmi_qiskit_ibm_runtime_job_def_hpp__

#include <chrono>
#include <thread>


#include "primitives/containers/sampler_pub.hpp"
#include "primitives/base/base_primitive_job.hpp"
#include "primitives/containers/sampler_primitive_result.hpp"
#include "providers/backend.hpp"
#include "providers/job.hpp"

namespace Qiskit {
namespace primitives {

/// @class BackendSamplerJob
/// @brief Job class for Backend Sampler primitive.
class BackendSamplerJob : public BasePrimitiveJob {
protected:
    std::vector<SamplerPub> pubs_;
public:
    /// @brief Create a new BasePrimitiveJob
    /// @param job a job pointer to run pubs
    /// @param pubs a list of pub
    BackendSamplerJob(std::shared_ptr<providers::Job> job, std::vector<SamplerPub>& pubs) : BasePrimitiveJob(job)
    {
        pubs_ = pubs;
    }

    BackendSamplerJob(std::shared_ptr<providers::Job> job, const nlohmann::ordered_json& pubs) : BasePrimitiveJob(job)
    {
        for (size_t i = 0; i < pubs.size(); i++) {
            pubs_.push_back(SamplerPub(pubs[i]));
        }
    }

    BackendSamplerJob(const BackendSamplerJob& other) : BasePrimitiveJob(other)
    {
        pubs_ = other.pubs_;
    }

    ~BackendSamplerJob()
    {
    }

    /// @brief get pubs for this job
    /// @return a list of pub
    const std::vector<SamplerPub>& pubs(void) const
    {
        return pubs_;
    }

    /// @brief Return the status of the job.
    /// @return JobStatus enum.
    providers::JobStatus status(void) override
    {
        return job_->status();
    }

    /// @brief Return whether the job is actively running.
    /// @return true if job is actively running, otherwise false.
    bool running(void) override
    {
        return job_->status() == providers::JobStatus::RUNNING;
    }

    /// @brief Return whether the job is queued.
    /// @return true if job is queued, otherwise false.
    bool queued(void)
    {
        return job_->status() == providers::JobStatus::QUEUED;
    }

    /// @brief Return whether the job has successfully run.
    /// @return true if successfully run, otherwise false.
    bool done(void) override
    {
        return job_->status() == providers::JobStatus::DONE;
    }

    /// @brief Return whether the job has been cancelled.
    /// @return true if job has been cancelled, otherwise false.
    bool cancelled(void) override
    {
        return job_->status() == providers::JobStatus::CANCELLED;
    }

    /// @brief Return whether the job is in a final job state such as DONE or ERROR.
    /// @return true if job is in a final job state, otherwise false.
    bool in_final_state(void) override
    {
        auto status = job_->status();
        if (status == providers::JobStatus::DONE || status == providers::JobStatus::CANCELLED || status == providers::JobStatus::FAILED)
            return true;
        return false;
    }

    /// @brief Attempt to cancel the job.
    bool cancel(void) override
    {
        //return job_.stop_job();
        return true;
    }

    SamplerPrimitiveResult result(void)
    {
        providers::JobStatus st;
        while (true) {
            st = job_->status();
            if (st == providers::JobStatus::DONE || st == providers::JobStatus::CANCELLED || st == providers::JobStatus::FAILED)
                break;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        SamplerPrimitiveResult result;
        if (st == providers::JobStatus::DONE) {
            uint_t num_results = job_->num_results();
            result.allocate(num_results);

            for (uint_t i = 0; i< num_results; i++) {
                result[i].set_pub(pubs_[i]);
                job_->result(i, result[i]);
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

};

} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_primitives_qrmi_qiskit_ibm_runtime_job_def_hpp__
