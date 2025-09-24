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

// Job class for QRMI

#ifndef __qiskitcpp_providers_QRMI_job_def_hpp__
#define __qiskitcpp_providers_QRMI_job_def_hpp__

#include <nlohmann/json.hpp>

#include "utils/types.hpp"

#include "primitives/containers/sampler_pub_result.hpp"
#include "providers/job.hpp"

#include "qrmi.h"

namespace Qiskit {
namespace providers {

/// @class QkrtJob
/// @brief Job class for Qiskit IBM runtime Rust C-API.
class QRMIJob : public Job {
protected:
    std::string job_id_;
    std::shared_ptr<QrmiQuantumResource> qrmi_ = nullptr;
    nlohmann::ordered_json results_;   // json formatted results (raw output from QRMI)
    uint_t num_results_ = 0;
public:
    /// @brief Create a new QkrtBackend
    QRMIJob()
    {
        num_results_ = 0;
    }

    /// @brief Create a new QkrtBackend object
    /// @param qrmi a pointer to QRMI handle
    /// @param job an id of the job
    QRMIJob(std::shared_ptr<QrmiQuantumResource> qrmi, const std::string& job)
    {
        job_id_ = job;
        qrmi_ = qrmi;
        num_results_ = 0;
    }

    /// @brief Create a new QkrtJob from other
    QRMIJob(const QRMIJob& other)
    {
        job_id_ = other.job_id_;
        qrmi_ = other.qrmi_;
        num_results_ = 0;
    }

    ~QRMIJob()
    {
        if (qrmi_)
            qrmi_.reset();
    }

    /// @brief Return the status of the job.
    /// @return JobStatus enum.
    providers::JobStatus status(void) override
    {
        QrmiTaskStatus st;
        int rc = qrmi_resource_task_status(qrmi_.get(), job_id_.c_str(), &st);
        if (rc != QRMI_RETURN_CODE_SUCCESS)
            return providers::JobStatus::FAILED;
        switch (st) {
            case QRMI_TASK_STATUS_QUEUED:
                return providers::JobStatus::QUEUED;
            case QRMI_TASK_STATUS_RUNNING:
                return providers::JobStatus::RUNNING;
            case QRMI_TASK_STATUS_COMPLETED:
                return providers::JobStatus::DONE;
            case QRMI_TASK_STATUS_FAILED:
                return providers::JobStatus::FAILED;
            case QRMI_TASK_STATUS_CANCELLED:
                return providers::JobStatus::CANCELLED;
        }
        return providers::JobStatus::FAILED;
    }


    /// @brief Return the number of results in this job
    /// @return number of results
    uint_t num_results(void) override
    {
        if (num_results_ == 0)
            read_results();
        return num_results_;
    }

    /// @brief get sampler pub result
    /// @param index an index of the reuslt
    /// @param result an output sampler pub result
    /// @return true if result is successfully set
    bool result(uint_t index, primitives::SamplerPubResult& result) override
    {
        if (num_results_ == 0)
            read_results();

        if (index >= num_results_)
            return false;

        result.from_json(results_["results"][index]);
        return true;
    }

protected:
    void read_results(void)
    {
        char *result = nullptr;
        int rc = qrmi_resource_task_result(qrmi_.get(), job_id_.c_str(), &result);
        if (rc == QRMI_RETURN_CODE_SUCCESS) {
            results_ = nlohmann::ordered_json::parse(result);

            num_results_ = results_["results"].size();
            qrmi_string_free((char *)result);
        }
        qrmi_resource_task_stop(qrmi_.get(), job_id_.c_str());
    }
};

} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_providers_QRMI_job_def_hpp__


