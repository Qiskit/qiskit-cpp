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

#ifdef _MSC_VER
#include <windows.h>
#else
#include <thread>
#include <chrono>
#endif


#include "primitives/base/base_primitive_job.hpp"
#include "providers/backend.hpp"

#include "qrmi.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace Qiskit {
namespace primitives {

/// @class BackendSamplerJob
/// @brief Job class for Backend Sampler primitive.
class BackendSamplerJob : public BasePrimitiveJob {
protected:
    providers::BackendV2& backend_;
public:
    /// @brief Create a new BasePrimitiveJob
    /// @param backend a backend to be used in this job
    /// @param id a unique id identifying the job.
    BackendSamplerJob(providers::BackendV2& backend, std::string id, std::vector<SamplerPub>& pubs) : BasePrimitiveJob(id, pubs), backend_(backend) {}


    /// @brief Return the status of the job.
    /// @return JobStatus enum.
    providers::JobStatus status(void) override
    {
        return backend_.status(job_id_);
    }

    /// @brief Return whether the job is actively running.
    /// @return true if job is actively running, otherwise false.
    bool running(void) override
    {
        return backend_.status(job_id_) == providers::JobStatus::RUNNING;
    }

    /// @brief Return whether the job is queued.
    /// @return true if job is queued, otherwise false.
    bool queued(void)
    {
        return backend_.status(job_id_) == providers::JobStatus::QUEUED;
    }

    /// @brief Return whether the job has successfully run.
    /// @return true if successfully run, otherwise false.
    bool done(void) override
    {
        return backend_.status(job_id_) == providers::JobStatus::DONE;
    }

    /// @brief Return whether the job has been cancelled.
    /// @return true if job has been cancelled, otherwise false.
    bool cancelled(void) override
    {
        return backend_.status(job_id_) == providers::JobStatus::CANCELLED;
    }

    /// @brief Return whether the job is in a final job state such as DONE or ERROR.
    /// @return true if job is in a final job state, otherwise false.
    bool in_final_state(void) override
    {
        auto status = backend_.status(job_id_);
        if (status == providers::JobStatus::DONE || status == providers::JobStatus::CANCELLED || status == providers::JobStatus::FAILED)
            return true;
        return false;
    }


    /// @brief Attempt to cancel the job.
    bool cancel(void) override
    {
        return backend_.stop_job(job_id_);
    }

    PrimitiveResult result(void) override
    {
        auto result = backend_.result(job_id_);
        result.set_pubs(pubs_);
        return result;
    }
};

} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_primitives_qrmi_qiskit_ibm_runtime_job_def_hpp__


