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
#include "primitives/base/base_primitive_job.hpp"
#include "primitives/backend_sampler_job.hpp"
#include "primitives/backend_estimator_job.hpp"

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
    QrmiResourceType type_ = QRMI_RESOURCE_TYPE_IBM_QUANTUM_COMPUTE_SERVICE;
    QrmiConfigMap config_;
    QrmiResourceProvider* provider_ = nullptr;
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
        config_.length = 0;
    }

    /// @brief Create a new QRMIBackend
    QiskitRuntimeService(std::string token, std::string instance)
    {
        token_ = token;
        instance_ = instance;
        config_.length = 0;
    }

    ~QiskitRuntimeService()
    {
        if (provider_)
            qrmi_provider_free(provider_);
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
        if (!make_config())
            return Qiskit::providers::QRMIBackend();

        std::shared_ptr<QrmiQuantumResource> qrmi = new_qrmi_resource(name);
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

    /// @brief get a list of resources in the provider
    /// @param filter filter strings to select the returning resource
    /// @return a list of name of resource
    std::vector<std::string> backends(const std::string filter = std::string())
    {
        const char* filter_ptr = filter.length() > 0 ? filter.c_str() : nullptr;

        std::vector<std::string> list;
        get_provider();
        if (provider_) {
            QrmiQuantumResources resources;
            auto rc = qrmi_provider_resources(provider_, filter_ptr, &resources);
            if (rc == QRMI_RETURN_CODE_SUCCESS) {
                for (size_t i = 0; i < resources.length; i++) {
                    char* res;
                    qrmi_resource_id(resources.resources[i], &res);
                    list.push_back(std::string(res));
                    qrmi_string_free(res);
                }
                qrmi_provider_resources_free(&resources);
            }
        }
        return list;
    }

    /// @brief get a least busy available backend
    /// @param filter filter strings to select the returning resource
    /// @return a new QRMIBackend classes
    Qiskit::providers::QRMIBackend least_busy(const std::string filter = std::string())
    {
        const char* filter_ptr = filter.length() > 0 ? filter.c_str() : nullptr;

        get_provider();
        if (provider_ == nullptr) {
            return Qiskit::providers::QRMIBackend();
        }

        QrmiQuantumResource* resource;
        QrmiReturnCode rc = qrmi_provider_least_busy(provider_, filter_ptr, &resource);
        if (rc != QRMI_RETURN_CODE_SUCCESS) {
            std::cerr << Qiskit::providers::qrmi_error_message("could not find leasy busy backend") << std::endl;
            return Qiskit::providers::QRMIBackend();
        }
        char* id;
        qrmi_resource_id(resource, &id);
        std::string name = id;
        qrmi_string_free(id);

        std::shared_ptr<QrmiQuantumResource> qrmi(resource, qrmi_resource_free);

        bool is_accessible = false;
        rc = qrmi_resource_is_accessible(qrmi.get(), &is_accessible);
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

    /// @brief restart a job from checkpoint file
    /// @param name filename for the checkpoint
    /// @return a new primitive job
    std::shared_ptr<Qiskit::primitives::BasePrimitiveJob> restart(const std::string& name)
    {
        nlohmann::json checkpoint;
        std::ifstream file(name);
        if (!file.is_open()) {
            std::cerr << "QRMI Qiskit Runtime ERROR: could not open checkpoint file " << name << std::endl;
            return nullptr;
        }
        file >> checkpoint;
        file.close();

        std::string backend = checkpoint["job"]["backend"];
        auto qrmi = new_qrmi_resource(backend);

        if (!qrmi) {
            std::cerr << "QRMI Qiskit Runtime ERROR: could not create a resource for " << backend << std::endl;
            return nullptr;
        }

        std::string token = checkpoint["job"]["token"];
        auto session = std::make_shared<Qiskit::providers::QRMISession>(qrmi, token.c_str(), backend);

        std::string program_id = checkpoint["job"]["program_id"];
        auto job = std::make_shared<Qiskit::providers::QRMIJob>(qrmi, checkpoint["job"]["job_id"], program_id, session);

        if (program_id == std::string("sampler")) {
            return std::make_shared<Qiskit::primitives::BackendSamplerJob>(job, checkpoint["pubs"]);
        } else {
            return std::make_shared<Qiskit::primitives::BackendEstimatorJob>(job, checkpoint["pubs"]);
        }
    }

protected:
    /// @brief Return the endpoint URL to configure for the selected resource type.
    /// @return the URL QRMI should use
    std::string endpoint_url(void) const
    {
        // Qiskit Runtime Service is reached through the versioned API path; the
        // other resource types take the endpoint as given.
        if (type_ == QRMI_RESOURCE_TYPE_QISKIT_RUNTIME_SERVICE || type_ == QRMI_RESOURCE_TYPE_IBM_QUANTUM_COMPUTE_SERVICE) {
            return url_ + "/api/v1";
        }
        return url_;
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

    /// @brief make a new QRMI resource from config
    /// @return a new QRMI resource shared pointer
    std::shared_ptr<QrmiQuantumResource> new_qrmi_resource(std::string name)
    {
        if (!make_config())
            return nullptr;

        return std::shared_ptr<QrmiQuantumResource>(qrmi_resource_new_from_config(name.c_str(), type_, &config_), qrmi_resource_free);
    }

    /// @brief make config map
    bool make_config(void)
    {
        // get API key and CRN from env and set env for QRMI C-API
        if (token_.length() == 0) {
            std::cerr << " ERROR: please set your API key to \"QISKIT_IBM_TOKEN\" environment variable" << std::endl;
            return false;
        }
        if (instance_.length() == 0) {
            std::cerr << " ERROR: please set your CRN to \"QISKIT_IBM_INSTANCE\" environment variable" << std::endl;
            return false;
        }

        // QRMI reads its configuration from environment variables named
        // "<resource>_QRMI_<prefix>_<setting>", where the prefix identifies the
        // resource type.
        std::string header;
        bool has_session = false;
        switch (type_) {
        case QRMI_RESOURCE_TYPE_IBM_QUANTUM_COMPUTE_SERVICE:
            header = "QRMI_IBM_QCS_";
            break;
        case QRMI_RESOURCE_TYPE_IBM_QUANTUM_SYSTEM:
            header = "QRMI_IBM_QS_";
            break;
        case QRMI_RESOURCE_TYPE_QISKIT_RUNTIME_SERVICE:
            // deplicated
            header = "QRMI_IBM_QRS_";
            has_session = true;
            break;
        case QRMI_RESOURCE_TYPE_PASQAL_CLOUD:
        case QRMI_RESOURCE_TYPE_PASQAL_LOCAL:
            std::cerr << " ERROR: Pasqal resource types are not supported by QiskitRuntimeService" << std::endl;
            return false;
        default:
            std::cerr << " ERROR: unknown QRMI resource type " << type_ << std::endl;
            return false;
        }
        static std::string endpoint_key = header + "ENDPOINT";
        static std::string iam_key = header + "IAM_ENDPOINT";
        static std::string apikey_key = header + "IAM_APIKEY";
        static std::string crn_key = header + "SERVICE_CRN";
        static std::string smode_key = header + "SESSION_MODE";
        static std::string ep_url = endpoint_url();

        if (has_session) {
            QrmiKeyValue variables[] = {
                {(char *)endpoint_key.c_str(), (char *)ep_url.c_str()},
                {(char *)iam_key.c_str(), (char *)iam_url_.c_str()},
                {(char *)apikey_key.c_str(), (char *)token_.c_str()},
                {(char *)crn_key.c_str(), (char *)instance_.c_str()},
                {(char *)smode_key.c_str(), (char *)session_mode_.c_str()},
            };
            config_.variables = variables;
            config_.length = 5;
        } else {
            QrmiKeyValue variables[] = {
                {(char *)endpoint_key.c_str(), (char *)ep_url.c_str()},
                {(char *)iam_key.c_str(), (char *)iam_url_.c_str()},
                {(char *)apikey_key.c_str(), (char *)token_.c_str()},
                {(char *)crn_key.c_str(), (char *)instance_.c_str()},
            };
            config_.variables = variables;
            config_.length = 4;
        }
        return true;
    }
    /// @brief get provider
    void get_provider(void)
    {
        if (provider_)
            return;
        make_config();
        provider_ = qrmi_provider_new(type_, &config_);
    }
};

} // namespace service
} // namespace Qiskit


#endif //__qiskitcpp_providers_qiskit_runtime_service_QRMI_def_hpp__
