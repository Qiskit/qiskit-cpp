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

// Backend access through QRMI

#ifndef __qiskitcpp_providers_QRMI_backend_def_hpp__
#define __qiskitcpp_providers_QRMI_backend_def_hpp__

#include "utils/types.hpp"
#include "providers/backend.hpp"
#include "primitives/backend_sampler_job.hpp"

#include <nlohmann/json.hpp>

#ifdef _MSC_VER
#include <windows.h>
#else
#include <thread>
#include <chrono>
#endif

#include "qrmi.h"


namespace Qiskit {
namespace providers {

/// @class QRMIBackend
/// @brief Backend class using QRMI.
class QRMIBackend : public BackendV2 {
protected:
    std::string primitive_name_ = "sampler";
    std::string acc_token_;
    std::shared_ptr<QrmiQuantumResource> qrmi_ = nullptr;
    std::shared_ptr<transpiler::Target> target_ = nullptr;
public:
    /// @brief Create a new QRMIBackend
    QRMIBackend() {}

    /// @brief Create a new QRMIBackend object
    /// @param backend_name a resource name for backend.
    /// @param type a type of QRMI resource.
    QRMIBackend(const std::string name, const std::shared_ptr<QrmiQuantumResource> qrmi)
    {
        name_ = name;
        qrmi_ = qrmi;
    }

    ~QRMIBackend()
    {
        if (qrmi_) {
            qrmi_.reset();
        }
    }

    /// @brief Return a target properties for this backend
    /// @return a target class
    std::shared_ptr<transpiler::Target> target(void) override
    {
        if (target_) {
            return target_;
        }
        transpiler::Target target;

        char *target_str = NULL;
        QrmiReturnCode rc = qrmi_resource_target(qrmi_.get(), &target_str);
        if (rc != QRMI_RETURN_CODE_SUCCESS) {
            return nullptr;
        }
        nlohmann::ordered_json json_target = nlohmann::ordered_json::parse(target_str);
        qrmi_string_free((char *)target_str);
        target_ = std::make_shared<transpiler::Target>();
        target_->from_json(json_target);
        return target_;
    }

    /// @brief Run and collect samples from each pub.
    /// @param pubs An iterable of pub-like objects.
    /// @return PrimitiveJob
    std::shared_ptr<primitives::BasePrimitiveJob> run(std::vector<primitives::SamplerPub>& input_pubs, uint_t shots) override
    {
        // JSON string to be submitted
        nlohmann::ordered_json sampler;
        nlohmann::ordered_json sampler_pubs = nlohmann::ordered_json::array();
        for (int i = 0; i < input_pubs.size(); i++) {
            sampler_pubs += input_pubs[i].to_json();
        }
        sampler["pubs"] = sampler_pubs;

        sampler["version"] = 2;
        sampler["support_qiskit"] = false;
        sampler["shots"] = shots;
        sampler["options"] = json::object();

        std::string sampler_input = sampler.dump(2);

        QrmiPayload payload;
        payload.tag = QRMI_PAYLOAD_QISKIT_PRIMITIVE;
        payload.QISKIT_PRIMITIVE.input = (char*)sampler_input.c_str();
        payload.QISKIT_PRIMITIVE.program_id = (char*)primitive_name_.c_str();

        char *id = NULL;
        QrmiReturnCode rc = qrmi_resource_task_start(qrmi_.get(), &payload, &id);
        if (rc != QRMI_RETURN_CODE_SUCCESS) {
            std::cerr << "QRMI Error : failed to start a task." << std::endl;
            return nullptr;
        }
        std::string job_id = id;
        qrmi_string_free((char *)id);
        std::cerr << " QRMI Job submitted to " << name_ << ", JOB ID = " << job_id << std::endl;

        return std::make_shared<primitives::BackendSamplerJob>(*this, job_id, input_pubs);
    }


    /// @brief Return the status of the job.
    /// @param job_id job id to get status
    /// @return JobStatus enum.
    providers::JobStatus status(std::string& job_id) override
    {
        QrmiTaskStatus st;
        int rc = qrmi_resource_task_status(qrmi_.get(), job_id.c_str(), &st);
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

    /// @brief Attempt to cancel the job.
    /// @param job_id job id to be cancelled
    /// @return true if the job is cancelled
    bool stop_job(std::string& job_id) override
    {
        QrmiReturnCode rc = qrmi_resource_task_stop(qrmi_.get(), job_id.c_str());
        if (rc != QRMI_RETURN_CODE_SUCCESS)
            return false;
        return true;
    }

    /// @brief Attempt to cancel the job.
    /// @param job_id job id to be cancelled
    /// @return true if the job is cancelled
    primitives::PrimitiveResult result(std::string& job_id) override
    {
        primitives::PrimitiveResult ret;
        QrmiTaskStatus status;
        QrmiReturnCode rc;
        while (true) {
            rc = qrmi_resource_task_status(qrmi_.get(), job_id.c_str(), &status);
            if (rc != QRMI_RETURN_CODE_SUCCESS)
                return ret;
            if (status == QRMI_TASK_STATUS_COMPLETED || status == QRMI_TASK_STATUS_FAILED || status == QRMI_TASK_STATUS_CANCELLED)
                break;
#ifdef _MSC_VER
            Sleep(1);
#else
            std::this_thread::sleep_for(std::chrono::seconds(1));
#endif
        }
        if (status == QRMI_TASK_STATUS_COMPLETED) {
            char *result = nullptr;
            rc = qrmi_resource_task_result(qrmi_.get(), job_id.c_str(), &result);
            if (rc == QRMI_RETURN_CODE_SUCCESS) {
                std::cerr << result << std::endl;
                ret.from_string(std::string(result));
                qrmi_string_free((char *)result);
            }
        }
        qrmi_resource_task_stop(qrmi_.get(), job_id.c_str());
        return ret;
    }

};

} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_providers_backend_def_hpp__


