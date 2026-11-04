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

#ifndef __qiskitcpp_providers_qiskit_runtime_service_QRMI_def_hpp__
#define __qiskitcpp_providers_qiskit_runtime_service_QRMI_def_hpp__

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <nlohmann/json.hpp>

#include "utils/types.hpp"
#include "providers/qrmi_backend.hpp"
#include "providers/qrmi_common.hpp"

#ifdef _MSC_VER
#define NOMINMAX
#include <windows.h>
#ifdef ERROR
#undef ERROR
#endif
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
        // check $HOME/.qiskit/qiskit-ibm.json first to get account info
#ifdef _MSC_VER
        char* home = nullptr;
        size_t len;
        _dupenv_s(&home, &len, "HOME");
#else
        char* home = getenv("HOME");
#endif
        if (home) {
            std::string conf(home);
            conf += "/.qiskit/qiskit-ibm.json";
            std::ifstream file(conf);
            if (file) {
                std::string str;
                std::string conf_str;
                while (std::getline(file, str)) {
                    conf_str += str;
                }
                nlohmann::json conf_json = nlohmann::json::parse(conf_str);

                if (conf_json.contains("default-ibm-quantum-platform")) {
                    auto iqp_set = conf_json["default-ibm-quantum-platform"];
                    if (iqp_set.contains("token") && iqp_set.at("token").is_string())
                        token_ = iqp_set["token"];
                    if (iqp_set.contains("instance") && iqp_set.at("instance").is_string())
                        instance_ = iqp_set["instance"];
                }
            }
        }

        // then get account info from environment variables
        if (token_.size() == 0) {
#ifdef _MSC_VER
            char* token = nullptr;
            _dupenv_s(&token, &len, "QISKIT_IBM_TOKEN");
#else
            char* token = getenv("QISKIT_IBM_TOKEN");
#endif
            if (token != nullptr) {
                token_ = token;
            }
        }
        if (instance_.size() == 0) {
#ifdef _MSC_VER
            char* crn = nullptr;
            _dupenv_s(&crn, &len, "QISKIT_IBM_INSTANCE");
#else
            char* crn = getenv("QISKIT_IBM_INSTANCE");
#endif
            if (crn != nullptr) {
                instance_ = crn;
            }
        }
    }

    /// @brief Create a new QRMIBackend
    QiskitRuntimeService(std::string token, std::string instance)
    {
        token_ = token;
        instance_ = instance;
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
    /// @note Only applies to QRMI_RESOURCE_TYPE_QISKIT_RUNTIME_SERVICE; the other
    ///   resource types have no session concept.
    void set_session_mode(std::string mode)
    {
        session_mode_ = mode;
    }

    /// @brief set the QRMI resource type to connect through
    /// @param type one of the QRMI resource types. Supported here are
    ///   QRMI_RESOURCE_TYPE_QISKIT_RUNTIME_SERVICE (the default),
    ///   QRMI_RESOURCE_TYPE_IBM_QUANTUM_SYSTEM and
    ///   QRMI_RESOURCE_TYPE_IBM_DIRECT_ACCESS (deprecated by QRMI).
    void set_resource_type(QrmiResourceType type)
    {
        type_ = type;
    }

    /// @brief set the service endpoint URL
    /// @param url the endpoint URL
    void set_url(std::string url)
    {
        url_ = url;
    }

    /// @brief set the IBM Cloud IAM endpoint URL
    /// @param url the IAM endpoint URL
    void set_iam_url(std::string url)
    {
        iam_url_ = url;
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

        // QRMI reads its configuration from environment variables named
        // "<resource>_QRMI_<prefix>_<setting>", where the prefix identifies the
        // resource type.
        std::string header;
        bool has_session = false;
        switch (type_) {
        case QRMI_RESOURCE_TYPE_IBM_QUANTUM_SYSTEM:
            header = "_QRMI_IBM_QS_";
            break;
        case QRMI_RESOURCE_TYPE_QISKIT_RUNTIME_SERVICE:
            header = "_QRMI_IBM_QRS_";
            has_session = true;
            break;
        case QRMI_RESOURCE_TYPE_PASQAL_CLOUD:
        case QRMI_RESOURCE_TYPE_PASQAL_LOCAL:
            std::cerr << " ERROR: Pasqal resource types are not supported by QiskitRuntimeService" << std::endl;
            return Qiskit::providers::QRMIBackend();
        default:
            std::cerr << " ERROR: unknown QRMI resource type " << type_ << std::endl;
            return Qiskit::providers::QRMIBackend();
        }

        set_resource_env(name, header, "ENDPOINT", endpoint_url());
        set_resource_env(name, header, "IAM_ENDPOINT", iam_url_);
        set_resource_env(name, header, "IAM_APIKEY", token_);
        set_resource_env(name, header, "SERVICE_CRN", instance_);
        if (has_session) {
            set_resource_env(name, header, "SESSION_MODE", session_mode_);
        }
        // The AWS/S3 settings that IBM Quantum System uses to deliver results are
        // optional in QRMI and have no counterpart here, so they are left to the
        // environment.

        std::shared_ptr<QrmiQuantumResource> qrmi(qrmi_resource_new(name.c_str(), type_), qrmi_resource_free);
        if (!qrmi) {
            std::cerr << Qiskit::providers::qrmi_error_message("could not create a resource for " + name) << std::endl;
            return Qiskit::providers::QRMIBackend();
        }

        bool is_accessible = false;
        QrmiReturnCode rc = qrmi_resource_is_accessible(qrmi.get(), &is_accessible);
        if (rc != QRMI_RETURN_CODE_SUCCESS) {
            std::cerr << Qiskit::providers::qrmi_error_message("could not check whether " + name + " is accessible", rc) << std::endl;
            return Qiskit::providers::QRMIBackend();
        }
        if (is_accessible == false) {
            std::cerr << "QRMI Error : " << name << " cannot be accessed." << std::endl;
            return Qiskit::providers::QRMIBackend();
        }

        std::cerr << "QRMI connecting : " << name << report_resource(qrmi.get()) << std::endl;

        Qiskit::providers::QRMIBackend backend(name, qrmi);
        return backend;
    }

protected:
    /// @brief Return the endpoint URL to configure for the selected resource type.
    /// @return the URL QRMI should use
    std::string endpoint_url(void) const
    {
        // Qiskit Runtime Service is reached through the versioned API path; the
        // other resource types take the endpoint as given.
        if (type_ == QRMI_RESOURCE_TYPE_QISKIT_RUNTIME_SERVICE) {
            return url_ + "/api/v1";
        }
        return url_;
    }

    /// @brief Set one QRMI configuration environment variable.
    /// @param name the resource name the setting applies to
    /// @param header the resource type prefix, e.g. "_QRMI_IBM_QRS_"
    /// @param suffix the setting name, e.g. "IAM_APIKEY"
    /// @param value the value to set
    static void set_resource_env(const std::string& name, const std::string& header,
                                 const std::string& suffix, const std::string& value)
    {
        const std::string key = name + header + suffix;
#ifdef _MSC_VER
        _putenv_s(key.c_str(), value.c_str());
#else
        setenv(key.c_str(), value.c_str(), 1);
#endif
    }

    /// @brief Describe the resource QRMI connected to, for logging.
    /// @param qrmi the resource handle
    /// @return a description such as " (id=ibm_torino, type=qiskit-runtime-service)",
    ///   or an empty string if QRMI could not report it
    static std::string report_resource(QrmiQuantumResource* qrmi)
    {
        Qiskit::providers::ScopedQrmiString id;
        if (qrmi_resource_id(qrmi, id.out()) != QRMI_RETURN_CODE_SUCCESS) {
            return std::string();
        }

        std::string ret = " (id=" + id.str();
        QrmiResourceType type;
        if (qrmi_resource_type(qrmi, &type) == QRMI_RETURN_CODE_SUCCESS) {
            Qiskit::providers::ScopedQrmiString type_str(
                (char *)qrmi_config_resource_type_to_str(type));
            if (type_str.valid()) {
                ret += ", type=" + type_str.str();
            }
        }
        ret += ")";
        return ret;
    }

};

} // namespace service
} // namespace Qiskit


#endif //__qiskitcpp_providers_qiskit_runtime_service_QRMI_def_hpp__
