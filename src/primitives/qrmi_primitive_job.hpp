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

// QRMI IBM runtime job class

#ifndef __qiskitcpp_primitives_qrmi_qiskit_ibm_runtime_job_def_hpp__
#define __qiskitcpp_primitives_qrmi_qiskit_ibm_runtime_job_def_hpp__

#ifdef _MSC_VER
#include <windows.h>
#else
#include <thread>
#include <chrono>
#endif


#include "primitives/base/base_primitive_job.hpp"
#include "qrmi.h"


namespace Qiskit {
namespace primitives {

/// @class QRMIJob
/// @brief Job class for QRMI C-API.
class QRMIPrimitiveJob : public BasePrimitiveJob {
protected:
    std::string backend_name_;
    std::string primitive_name_ = "sampler";
    std::string acc_token_;
    QrmiQuantumResource* qrmi_ = nullptr;
    QrmiResourceType type_ = QRMI_RESOURCE_TYPE_QISKIT_RUNTIME_SERVICE;
public:
    QRMIPrimitiveJob() {}

    /// @brief Create a new QRMIPrimitiveJob object
    /// @param backend_name a resource name for backend.
    /// @param type a type of QRMI resource.
    QRMIPrimitiveJob(std::string backend_name, QrmiResourceType type = QRMI_RESOURCE_TYPE_QISKIT_RUNTIME_SERVICE)
    {
        backend_name_ = backend_name;
        type_ = type;
    }

    ~QRMIPrimitiveJob()
    {
        if (qrmi_) {
            qrmi_resource_free(qrmi_);
        }
    }

    /// @brief Set type of QRMI resource.
    void set_type(QrmiResourceType type)
    {
        type_ = type;
        if (!qrmi_) {
            connect();
        }
    }

    /// @brief Return the status of the job.
    /// @return JobStatus enum.
    providers::JobStatus status(void) override
    {
        QrmiTaskStatus st;
        int rc = qrmi_resource_task_status(qrmi_, job_id_.c_str(), &st);
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

    /// @brief connect to backend.
    /// @return false if connection failed.
    bool connect(void)
    {
        if (qrmi_) {
            qrmi_resource_free(qrmi_);
        }

        // get API key and CRN from env and set env for QRMI C-API
        char* api_key = getenv("QISKIT_IBM_TOKEN");
        if (api_key == nullptr) {
            std::cerr << " ERROR: please set your API key to \"QISKIT_IBM_TOKEN\" environment variable" << std::endl;
            return false;
        }
        char* crn = getenv("QISKIT_IBM_INSTANCE");
        if (crn == nullptr) {
            std::cerr << " ERROR: please set your CRN to \"QISKIT_IBM_INSTANCE\" environment variable" << std::endl;
            return false;
        }
        std::string header;
        if (type_ == QRMI_RESOURCE_TYPE_IBM_DIRECT_ACCESS) {
            header = "_QRMI_IBM_DS_";
        } else if (type_ == QRMI_RESOURCE_TYPE_PASQAL_CLOUD) {
            std::cerr << " ERROR: resource type QRMI_RESOURCE_TYPE_PASQAL_CLOUD is not supported yet" << std::endl;
            return false;
        } else {
            header = "_QRMI_IBM_QRS_";
        }

        std::string envEndPoint = backend_name_ + header + "ENDPOINT=https://quantum.cloud.ibm.com/api/v1";
        std::string envIAMEndPoint = backend_name_ + header + "IAM_ENDPOINT=https://iam.cloud.ibm.com";
        std::string envAPIKey = backend_name_ + header + "IAM_APIKEY=" + api_key;
        std::string envCRN = backend_name_ + header + "SERVICE_CRN=" + crn;
        std::string envSession = backend_name_ + header + "SESSION_MODE=batch";
#ifdef _MSC_VER
        _putenv(envEndPoint.c_str());
        _putenv(envIAMEndPoint.c_str());
        _putenv(envAPIKey.c_str());
        _putenv(envCRN.c_str());
        _putenv(envSession.c_str());
#else
        putenv((char*)envEndPoint.c_str());
        putenv((char*)envIAMEndPoint.c_str());
        putenv((char*)envAPIKey.c_str());
        putenv((char*)envCRN.c_str());
        putenv((char*)envSession.c_str());
#endif

        qrmi_ = qrmi_resource_new(backend_name_.c_str(), QRMI_RESOURCE_TYPE_QISKIT_RUNTIME_SERVICE);

        bool is_accessible = false;
        QrmiReturnCode rc = qrmi_resource_is_accessible(qrmi_, &is_accessible);
        if (rc == QRMI_RETURN_CODE_SUCCESS) {
            if (is_accessible == false) {
                std::cerr << "QRMI Error : " << backend_name_ << " cannot be accessed." << std::endl;
                return false;
            }
        } else {
            std::cerr << "qrmi_ibmqrs_is_accessible() failed." << std::endl;
            return false;
        }
        std::cerr << "QRMI connecting : " << backend_name_ << std::endl;

//        const char *acquisition_token = qrmi_ibmqrs_acquire(qrmi_);
//        acc_token_ = acquisition_token;
//        std::cerr << "QRMI acquire : " << acc_token_ << std::endl;

//        qrmi_free_string((char *)acquisition_token);
        return true;
    }

    /// @brief submit a job
    /// @param json a job data in JSON format.
    /// @return false if connection failed.
    bool submit(std::string input)
    {
        QrmiPayload payload;
        payload.tag = QRMI_PAYLOAD_QISKIT_PRIMITIVE;
        payload.QISKIT_PRIMITIVE.input = (char*)input.c_str();
        payload.QISKIT_PRIMITIVE.program_id = (char*)primitive_name_.c_str();

        char *job_id = NULL;
        QrmiReturnCode rc = qrmi_resource_task_start(qrmi_, &payload, &job_id);
        if (rc != QRMI_RETURN_CODE_SUCCESS) {
            std::cerr << "QRMI Error : failed to start a task." << std::endl;
            return false;
        }
        job_id_ = job_id;
        qrmi_string_free((char *)job_id);
        std::cerr << " QRMI Job submitted to " << backend_name_ << ", JOB ID = " << job_id_ << std::endl;
        return true;
    }

    /// @brief Return target configs.
    /// @return a JSON format target configs.
    nlohmann::json target(void)
    {
        if (!qrmi_) {
            if (!connect()){
                return nlohmann::json();
            }
        }
        char *target = NULL;
        QrmiReturnCode rc = qrmi_resource_target(qrmi_, &target);
        if (rc == QRMI_RETURN_CODE_SUCCESS) {
            nlohmann::json ret = nlohmann::json::parse(target);
            qrmi_string_free((char *)target);
            return ret;
        }
        return nlohmann::json();
    }

    /// @brief Return whether the job is actively running.
    /// @return true if job is actively running, otherwise false.
    bool running(void) override
    {
        QrmiTaskStatus status;
        QrmiReturnCode rc = qrmi_resource_task_status(qrmi_, job_id_.c_str(), &status);
        if (rc != QRMI_RETURN_CODE_SUCCESS || status != QRMI_TASK_STATUS_RUNNING)
            return false;
        return true;
    }

    /// @brief Return whether the job is queued.
    /// @return true if job is queued, otherwise false.
    bool queued(void)
    {
        QrmiTaskStatus status;
        QrmiReturnCode rc = qrmi_resource_task_status(qrmi_, job_id_.c_str(), &status);
        if (rc != QRMI_RETURN_CODE_SUCCESS || status != QRMI_TASK_STATUS_QUEUED)
            return false;
        return true;
    }

    /// @brief Return whether the job has successfully run.
    /// @return true if successfully run, otherwise false.
    bool done(void) override
    {
        QrmiTaskStatus status;
        QrmiReturnCode rc = qrmi_resource_task_status(qrmi_, job_id_.c_str(), &status);
        if (rc != QRMI_RETURN_CODE_SUCCESS || status != QRMI_TASK_STATUS_COMPLETED)
            return false;
        return true;
    }

    /// @brief Return whether the job has been cancelled.
    /// @return true if job has been cancelled, otherwise false.
    bool cancelled(void) override
    {
        QrmiTaskStatus status;
        QrmiReturnCode rc = qrmi_resource_task_status(qrmi_, job_id_.c_str(), &status);
        if (rc != QRMI_RETURN_CODE_SUCCESS || status != QRMI_TASK_STATUS_CANCELLED)
            return false;
        return true;
    }

    /// @brief Return whether the job is in a final job state such as DONE or ERROR.
    /// @return true if job is in a final job state, otherwise false.
    bool in_final_state(void) override
    {
        QrmiTaskStatus status;
        QrmiReturnCode rc = qrmi_resource_task_status(qrmi_, job_id_.c_str(), &status);
        if (rc != QRMI_RETURN_CODE_SUCCESS)
            return false;
        if (status == QRMI_TASK_STATUS_COMPLETED || status == QRMI_TASK_STATUS_FAILED)
            return true;
        return false;
    }


    /// @brief Attempt to cancel the job.
    bool cancel(void) override
    {
        QrmiReturnCode rc = qrmi_resource_task_stop(qrmi_, job_id_.c_str());
        if (rc != QRMI_RETURN_CODE_SUCCESS)
            return false;
        return true;
    }

    std::shared_ptr<PrimitiveResult> result(void)
    {
        QrmiTaskStatus status;
        QrmiReturnCode rc;
        while (true) {
            rc = qrmi_resource_task_status(qrmi_, job_id_.c_str(), &status);
            if (rc != QRMI_RETURN_CODE_SUCCESS)
                return nullptr;
            if (status == QRMI_TASK_STATUS_COMPLETED || status == QRMI_TASK_STATUS_FAILED)
                break;
#ifdef _MSC_VER
            Sleep(1);
#else
            std::this_thread::sleep_for(std::chrono::seconds(1));
#endif
        }
        std::shared_ptr<PrimitiveResult> ret = nullptr;
        if (status == QRMI_TASK_STATUS_COMPLETED) {
            ret = std::make_shared<PrimitiveResult>();
            std::shared_ptr<PrimitiveResult> ret = std::make_shared<PrimitiveResult>();
            char *result = nullptr;
            rc = qrmi_resource_task_result(qrmi_, job_id_.c_str(), &result);
            if (rc == QRMI_RETURN_CODE_SUCCESS) {
                std::cerr << result << std::endl;
                ret->from_string(std::string(result));
                qrmi_string_free((char *)result);
            }
        }
        qrmi_resource_task_stop(qrmi_, job_id_.c_str());

//        rc = qrmi_ibmqrs_release(qrmi_, acc_token_.c_str());

        qrmi_resource_free(qrmi_);
        qrmi_ = nullptr;

        return ret;
    }

};

} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_primitives_qrmi_qiskit_ibm_runtime_job_def_hpp__


