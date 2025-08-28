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

// Qiskit Runtime Service using QRMI

#ifndef __qiskitcpp_providers_qiskit_runtime_service_def_hpp__
#define __qiskitcpp_providers_qiskit_runtime_service_def_hpp__

#include "utils/types.hpp"
#include "providers/qrmi_backend.hpp"

#ifdef _MSC_VER
#include <windows.h>
#else
#include <thread>
#include <chrono>
#endif

#include "qrmi.h"


namespace Qiskit {
namespace service {

/// @class QiskitRuntimeService
/// @brief runtime service using QRMI.
class QiskitRuntimeService {
protected:
    std::string channel_ = "ibm_quantum_platform";
    std::string url_ = "https://quantum.cloud.ibm.com";
    std::string iam_url_ = "https://iam.cloud.ibm.com";
    std::string token_;     //  API Key
    std::string instance_;  // CRN
    std::string session_mode_ = "batch";
    QrmiResourceType type_ = QRMI_RESOURCE_TYPE_QISKIT_RUNTIME_SERVICE;
public:
    /// @brief Create a new QRMIBackend
    QiskitRuntimeService()
    {
        char* token = getenv("QISKIT_IBM_TOKEN");
        if (token != nullptr) {
            token_ = token;
        }
        char* crn = getenv("QISKIT_IBM_INSTANCE");
        if (crn != nullptr) {
            instance_ = crn;
        }
    }

    ~QiskitRuntimeService()
    {
    }

    /// @brief set API key
    /// @param token API token key to access IQP
    void set_token(std::string token)
    {
        token_ = token;
    }

    /// @brief set session mode
    /// @param mode "batch" or "dedicated"
    void set_session_mode(std::string mode)
    {
        session_mode_ = mode;
    }

    /// @brief create a new backend object
    /// @param name the name of the backend resource
    /// @return a new QRMIBackend classes
    Qiskit::providers::QRMIBackend backend(std::string name, std::string instance = std::string())
    {
        if (instance.length() > 0) {
            instance_ = instance;
        }

        // get API key and CRN from env and set env for QRMI C-API
        if (token_.length() == 0) {
            std::cerr << " ERROR: please set your API key to \"QISKIT_IBM_TOKEN\" environment variable" << std::endl;
            return Qiskit::providers::QRMIBackend();
        }
        if (instance_.length() == 0) {
            std::cerr << " ERROR: please set your CRN to \"QISKIT_IBM_INSTANCE\" environment variable" << std::endl;
            return Qiskit::providers::QRMIBackend();
        }

        std::string header;
        if (type_ == QRMI_RESOURCE_TYPE_IBM_DIRECT_ACCESS) {
            header = "_QRMI_IBM_DS_";
        } else if (type_ == QRMI_RESOURCE_TYPE_PASQAL_CLOUD) {
            std::cerr << " ERROR: resource type QRMI_RESOURCE_TYPE_PASQAL_CLOUD is not supported yet" << std::endl;
            return Qiskit::providers::QRMIBackend();
        } else {
            header = "_QRMI_IBM_QRS_";
        }

        std::string envEndPoint = name + header + "ENDPOINT=" + url_ + "/api/v1";
        std::string envIAMEndPoint = name + header + "IAM_ENDPOINT=" + iam_url_;
        std::string envAPIKey = name + header + "IAM_APIKEY=" + token_;
        std::string envCRN = name + header + "SERVICE_CRN=" + instance_;
        std::string envSession = name + header + "SESSION_MODE=" + session_mode_;
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
        std::shared_ptr<QrmiQuantumResource> qrmi(qrmi_resource_new(name.c_str(), type_), qrmi_resource_free);
        bool is_accessible = false;
        QrmiReturnCode rc = qrmi_resource_is_accessible(qrmi.get(), &is_accessible);
        if (rc == QRMI_RETURN_CODE_SUCCESS) {
            if (is_accessible == false) {
                std::cerr << "QRMI Error : " << name << " cannot be accessed." << std::endl;
                return Qiskit::providers::QRMIBackend();
            }
        } else {
            std::cerr << "qrmi_ibmqrs_is_accessible() failed." << std::endl;
            return Qiskit::providers::QRMIBackend();
        }
        std::cerr << "QRMI connecting : " << name << std::endl;

        Qiskit::providers::QRMIBackend backend(name, qrmi);
        return backend;
    }

};

} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_providers_backend_def_hpp__


