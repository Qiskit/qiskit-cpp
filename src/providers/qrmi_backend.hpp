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
#include "providers/qrmi_common.hpp"
#include "providers/qrmi_estimator_payload.hpp"
#include "providers/qrmi_job.hpp"

#include <nlohmann/json.hpp>

#include "qrmi.h"

#include <cmath>
#include <memory>
#include <stdexcept>
#include <string>

namespace Qiskit {
namespace providers {

/// @class QRMIBackend
/// @brief Backend class using QRMI.
class QRMIBackend : public BackendV2 {
protected:
    std::string primitive_name_ = "sampler";
    std::shared_ptr<QrmiQuantumResource> qrmi_ = nullptr;
    std::shared_ptr<QRMISession> session_ = nullptr;
    transpiler::Target target_;
public:
    /// @brief Create a new QRMIBackend
    QRMIBackend() {}

    /// @brief Create a new QRMIBackend object
    /// @param name a resource name for backend.
    /// @param qrmi a QRMI resource handle for the backend.
    QRMIBackend(const std::string name, const std::shared_ptr<QrmiQuantumResource> qrmi)
    {
        name_ = name;
        qrmi_ = qrmi;
    }

    /// @brief Create a new QRMIBackend from other
    /// @details The session is shared rather than copied, so it is released once
    ///   when the last backend referring to it goes away. ``QiskitRuntimeService``
    ///   returns a backend by value, so copies are expected.
    QRMIBackend(const QRMIBackend& other)
    {
        name_ = other.name_;
        primitive_name_ = other.primitive_name_;
        qrmi_ = other.qrmi_;
        session_ = other.session_;
        target_ = other.target_;
    }

    ~QRMIBackend()
    {
        // Release the session before dropping the resource handle it borrows.
        if (session_) {
            session_.reset();
        }
        if (qrmi_) {
            qrmi_.reset();
        }
    }

    /// @brief Return a target properties for this backend
    /// @return a target class
    const transpiler::Target& target(void) override
    {
        if (target_.is_set()) {
            return target_;
        }
        // A default-constructed backend, or one returned after a failed connect,
        // holds no resource to query.
        if (!qrmi_) {
            std::cerr << "QRMI Error : backend is not connected to a resource." << std::endl;
            return target_;
        }

        ScopedQrmiString target_str;
        QrmiReturnCode rc = qrmi_resource_target(qrmi_.get(), target_str.out());
        if (rc != QRMI_RETURN_CODE_SUCCESS) {
            std::cerr << qrmi_error_message("could not read the target for " + name_, rc) << std::endl;
            return target_;
        }
        nlohmann::ordered_json json_target = nlohmann::ordered_json::parse(target_str.str());
        target_ = transpiler::Target();
        target_.from_json(json_target);
        return target_;
    }

    /// @brief Acquire the quantum resource, opening a session if it supports one.
    /// @return true if a session is held when this returns
    /// @details Resources without a session concept, such as IBM Quantum System
    ///   and IBM Direct Access, return a token without opening anything. For
    ///   Qiskit Runtime Service this creates the session that subsequent jobs run
    ///   inside, honouring the mode set by
    ///   ``QiskitRuntimeService::set_session_mode``. Calling this when a session
    ///   is already held does nothing.
    bool acquire_session(void)
    {
        if (session_) {
            return true;
        }
        if (!qrmi_) {
            std::cerr << "QRMI Error : backend is not connected to a resource." << std::endl;
            return false;
        }

        ScopedQrmiString token;
        QrmiReturnCode rc = qrmi_resource_acquire(qrmi_.get(), token.out());
        if (rc != QRMI_RETURN_CODE_SUCCESS) {
            std::cerr << qrmi_error_message("could not acquire " + name_, rc) << std::endl;
            return false;
        }

        session_ = std::make_shared<QRMISession>(qrmi_, token.str());
        std::cerr << " QRMI session acquired on " << name_ << ", token = " << session_->token() << std::endl;
        return true;
    }

    /// @brief Release the quantum resource, closing the session if one is held.
    /// @details Only takes effect once every backend and job sharing the session
    ///   has released it. Called automatically when the backend is destroyed.
    void release_session(void)
    {
        if (session_) {
            session_.reset();
        }
    }

    /// @brief Return the acquisition token of the session currently held.
    /// @return the token, or an empty string if no session is held
    std::string session_token(void) const
    {
        return session_ ? session_->token() : std::string();
    }

    /// @brief Run and collect samples from each pub.
    /// @param input_pubs An iterable of pub-like objects.
    /// @param shots The number of shots to run each pub for.
    /// @return PrimitiveJob
    std::shared_ptr<providers::Job> run(std::vector<primitives::SamplerPub>& input_pubs, uint_t shots) override
    {
        // JSON string to be submitted
        nlohmann::ordered_json sampler;
        nlohmann::ordered_json sampler_pubs = nlohmann::ordered_json::array();
        for (int i = 0; i < input_pubs.size(); i++) {
            // A parameterized circuit is bound on the server from the values in
            // the pub, so a count mismatch has to be caught before submitting.
            std::string message;
            if (!input_pubs[i].validate(&message)) {
                std::cerr << "QRMI Error : sampler pub " << i << " is not valid : "
                    << message << std::endl;
                return nullptr;
            }
            try {
                sampler_pubs += input_pubs[i].to_json();
            } catch (const std::exception& exc) {
                std::cerr << "QRMI Error : could not serialize sampler pub " << i
                    << " : " << exc.what() << std::endl;
                return nullptr;
            }
        }
        sampler["pubs"] = sampler_pubs;

        sampler["version"] = 2;
        sampler["support_qiskit"] = false;
        sampler["shots"] = shots;
        sampler["options"] = json::object();

        return submit_task(sampler.dump(2), primitive_name_, "a sampler task");
    }

    /// @brief Run and estimate expectation values from each pub.
    /// @param input_pubs An iterable of estimator pub-like objects.
    /// @param precision The default target precision for pubs without an explicit precision.
    /// @return PrimitiveJob
    std::shared_ptr<providers::Job> run(std::vector<primitives::EstimatorPub>& input_pubs, double precision) override
    {
        if (!std::isfinite(precision) || precision < 0.0) {
            std::cerr << "QRMI Error : estimator precision must be finite and non-negative." << std::endl;
            return nullptr;
        }

        nlohmann::ordered_json estimator;
        try {
            estimator = QRMIEstimatorPayload::build_estimator_input(input_pubs, precision);
        } catch (const std::invalid_argument& exc) {
            std::cerr << "QRMI Error : " << exc.what() << std::endl;
            return nullptr;
        }

        return submit_task(estimator.dump(2),
                           QRMIEstimatorPayload::estimator_program_id(),
                           "an estimator task");
    }

protected:
    /// @brief Submit a Qiskit primitive payload as a QRMI task.
    /// @param input the primitive input, as a JSON string
    /// @param program_id the primitive to run, "sampler" or "estimator"
    /// @param description what is being submitted, for diagnostics
    /// @return the job, or nullptr if the task could not be started
    std::shared_ptr<providers::Job> submit_task(const std::string& input,
                                                const std::string& program_id,
                                                const std::string& description)
    {
        // Open the session before submitting so the job runs inside it. Done here
        // rather than at construction so that a backend used only for transpiling
        // does not hold a session open.
        if (!acquire_session()) {
            return nullptr;
        }

        QrmiPayload payload;
        payload.tag = QRMI_PAYLOAD_QISKIT_PRIMITIVE;
        payload.QISKIT_PRIMITIVE.input = (char*)input.c_str();
        payload.QISKIT_PRIMITIVE.program_id = (char*)program_id.c_str();

        ScopedQrmiString id;
        QrmiReturnCode rc = qrmi_resource_task_start(qrmi_.get(), &payload, id.out());
        if (rc != QRMI_RETURN_CODE_SUCCESS) {
            std::cerr << qrmi_error_message("could not start " + description + " on " + name_, rc) << std::endl;
            return nullptr;
        }

        const std::string job_id = id.str();
        std::cerr << " QRMI Job submitted to " << name_ << ", JOB ID = " << job_id << std::endl;

        return std::make_shared<providers::QRMIJob>(qrmi_, job_id, session_);
    }
};

} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_providers_backend_def_hpp__
