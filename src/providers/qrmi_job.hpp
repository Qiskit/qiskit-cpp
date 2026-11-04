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

#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

#include "utils/types.hpp"

#include "primitives/containers/estimator_pub_result.hpp"
#include "primitives/containers/sampler_pub.hpp"
#include "primitives/containers/sampler_pub_result.hpp"
#include "providers/job.hpp"
#include "providers/qrmi_common.hpp"

#include "qrmi.h"

namespace Qiskit {
namespace providers {

/// @class QRMISession
/// @brief An acquisition of a QRMI quantum resource.
/// @details Acquired by ``QRMIBackend::acquire_session`` and released when the
///   last backend or job holding it is destroyed. The resource handle is kept
///   alive here so that the release cannot outlive the resource it refers to.
///   For Qiskit Runtime Service the acquisition is the session that jobs run
///   inside; for resources with no session concept QRMI returns a token without
///   opening anything, so holding one is harmless.
class QRMISession {
protected:
    std::shared_ptr<QrmiQuantumResource> qrmi_;
    std::string token_;
public:
    /// @brief Take ownership of an acquisition.
    /// @param qrmi the resource that was acquired
    /// @param token the acquisition token QRMI returned
    QRMISession(std::shared_ptr<QrmiQuantumResource> qrmi, const std::string& token)
        : qrmi_(qrmi), token_(token)
    {
    }

    // An acquisition is released exactly once, so it must not be copied.
    QRMISession(const QRMISession&) = delete;
    QRMISession& operator=(const QRMISession&) = delete;

    ~QRMISession()
    {
        if (!qrmi_ || token_.empty()) {
            return;
        }
        QrmiReturnCode rc = qrmi_resource_release(qrmi_.get(), token_.c_str());
        if (rc != QRMI_RETURN_CODE_SUCCESS) {
            // Reported rather than thrown: a destructor must not throw, and a
            // session left open expires on its own after its max_ttl.
            std::cerr << qrmi_error_message("could not release the session", rc) << std::endl;
        }
    }

    /// @brief Return the acquisition token.
    /// @return the token QRMI returned when the resource was acquired
    const std::string& token(void) const { return token_; }
};

/// @class QRMIJob
/// @brief Job class for QRMI.
class QRMIJob : public Job {
protected:
    std::string job_id_;
    std::shared_ptr<QrmiQuantumResource> qrmi_ = nullptr;
    std::shared_ptr<QRMISession> session_ = nullptr;
    nlohmann::ordered_json results_;   // json formatted results (raw output from QRMI)
    uint_t num_results_ = 0;
    bool results_read_ = false;
public:
    /// @brief Create a new QRMIJob
    QRMIJob()
    {
        num_results_ = 0;
    }

    /// @brief Create a new QRMIJob object
    /// @param qrmi a pointer to QRMI handle
    /// @param job an id of the job
    /// @param session the session the job was submitted inside, if any. Held so
    ///   that it stays open for as long as the results may be read.
    QRMIJob(std::shared_ptr<QrmiQuantumResource> qrmi, const std::string& job,
            std::shared_ptr<QRMISession> session = nullptr)
    {
        job_id_ = job;
        qrmi_ = qrmi;
        session_ = session;
        num_results_ = 0;
    }

    /// @brief Create a new QRMIJob from other
    QRMIJob(const QRMIJob& other)
    {
        job_id_ = other.job_id_;
        qrmi_ = other.qrmi_;
        session_ = other.session_;
        results_ = other.results_;
        num_results_ = other.num_results_;
        results_read_ = other.results_read_;
    }

    ~QRMIJob()
    {
        // Drop the session before the resource handle it borrows.
        if (session_)
            session_.reset();
        if (qrmi_)
            qrmi_.reset();
    }

    /// @brief Return the id of this job.
    /// @return the task id QRMI assigned.
    const std::string& job_id(void) const { return job_id_; }

    /// @brief Return the status of the job.
    /// @return JobStatus enum.
    providers::JobStatus status(void) override
    {
        QrmiTaskStatus st;
        QrmiReturnCode rc = qrmi_resource_task_status(qrmi_.get(), job_id_.c_str(), &st);
        if (rc != QRMI_RETURN_CODE_SUCCESS) {
            std::cerr << qrmi_error_message("could not read the status of job " + job_id_, rc) << std::endl;
            return providers::JobStatus::FAILED;
        }
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
        if (!results_read_ && !read_results()) {
            throw std::runtime_error("QRMIJob failed to read task results.");
        }
        return num_results_;
    }

    /// @brief get sampler pub result
    /// @param index an index of the reuslt
    /// @param result an output sampler pub result
    /// @return true if result is successfully set
    bool result(uint_t index, primitives::SamplerPubResult& result) override
    {
        if (!results_read_ && !read_results())
            return false;

        if (index >= num_results_)
            return false;

        return result.from_json(results_["results"][index]);
    }

    /// @brief get estimator pub result
    /// @param index an index of the result
    /// @param result an output estimator pub result
    /// @return true if result is successfully set
    bool result(uint_t index, primitives::EstimatorPubResult& result) override
    {
        if (!results_read_ && !read_results())
            return false;

        if (index >= num_results_)
            return false;

        return result.from_json(results_["results"][index]);
    }

    /// @brief Return the execution logs of this job.
    /// @return the logs, or an empty string if they could not be read
    std::string logs(void)
    {
        ScopedQrmiString logs;
        QrmiReturnCode rc = qrmi_resource_task_logs(qrmi_.get(), job_id_.c_str(), logs.out());
        if (rc != QRMI_RETURN_CODE_SUCCESS) {
            std::cerr << qrmi_error_message("could not read the logs of job " + job_id_, rc) << std::endl;
            return std::string();
        }
        return logs.str();
    }

    /// @brief Attempt to cancel this job.
    /// @return true if the request was accepted
    /// @details QRMI cancels the task only if it is still queued or running; a
    ///   task that already finished is left alone.
    bool cancel(void)
    {
        return stop_task();
    }

protected:
    bool read_results(void)
    {
        if (results_read_) {
            return true;
        }

        ScopedQrmiString result;
        QrmiReturnCode rc = qrmi_resource_task_result(qrmi_.get(), job_id_.c_str(), result.out());
        if (rc != QRMI_RETURN_CODE_SUCCESS) {
            std::cerr << qrmi_error_message("could not read the result of job " + job_id_, rc) << std::endl;
            return false;
        }

        try {
            results_ = nlohmann::ordered_json::parse(result.str());
        } catch (const std::exception& e) {
            // The task state is unknown at this point, so stop it rather than
            // leaving it to run against the account.
            stop_task();
            std::cerr << "QRMI Error : failed to parse the result of job " << job_id_
                << ": " << e.what() << std::endl;
            return false;
        }
        // A completed task needs no stop: qrmi_resource_task_stop only cancels a
        // task that is still queued or running.

        if (!results_.is_object() || !results_.contains("results") || !results_["results"].is_array()) {
            std::cerr << "QRMI Error : the result of job " << job_id_
                << " does not contain a results array." << std::endl;
            return false;
        }

        num_results_ = results_["results"].size();
        results_read_ = true;
        return true;
    }

    bool stop_task(void)
    {
        QrmiReturnCode rc = qrmi_resource_task_stop(qrmi_.get(), job_id_.c_str());
        if (rc != QRMI_RETURN_CODE_SUCCESS) {
            std::cerr << qrmi_error_message("could not stop job " + job_id_, rc) << std::endl;
            return false;
        }
        return true;
    }
};

} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_providers_QRMI_job_def_hpp__
