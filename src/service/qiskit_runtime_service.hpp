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

// Qiskit Runtime Service

#ifdef QRMI_ROOT
#include "service/qiskit_runtime_service qrmi.hpp"
#elif defined(SQC_ROOT)
#include "service/qiskit_runtime_service_sqc.hpp"
#else   // otherwise use Qiskit IBM runtime C-API

#ifndef __qiskitcpp_providers_qiskit_runtime_service_def_hpp__
#define __qiskitcpp_providers_qiskit_runtime_service_def_hpp__

#include <unordered_map>

#include "utils/types.hpp"
#include "providers/qkrt_backend.hpp"

#ifdef _MSC_VER
#include <windows.h>
#else
#include <thread>
#include <chrono>
#endif

extern "C" {
#include "qiskit_ibm_runtime/qiskit_ibm_runtime.h"
}

using qkrt_Backend = Backend;

namespace Qiskit {
namespace service {

/// @class QiskitRuntimeService
/// @brief runtime service using Qiskit IBM runtime Rust C-API.
class QiskitRuntimeService {
protected:
    std::shared_ptr<Service> service_ = nullptr;
    BackendSearchResults* backend_results_ = nullptr;
    std::unordered_map<std::string, qkrt_Backend*> backends_;
public:
    /// @brief Create a new runtime service class
    /// This constructor initializes runtime service from saved account configuration
    /// in ~/.qiskit/qiskit-ibm.json
    QiskitRuntimeService()
    {
        Service* service;
        int ret = qkrt_service_new(&service);
        if (ret != 0) {
            std::cerr << "ERROR: Qiskit IBM runtime serivce initialization failed with code " << ret << std::endl;
        } else {
            service_ = std::shared_ptr<Service>(service, qkrt_service_free);
        }
    }

    ~QiskitRuntimeService()
    {
        if (service_) {
            service_.reset();
        }
        if (backend_results_) {
            qkrt_backend_search_results_free(backend_results_);
        }
    }

    /// @brief get list of backends
    /// @return a list of backends provided in this service
    std::vector<std::string> backends(void)
    {
        get_backends();

        std::vector<std::string> keys;
        for (auto b : backends_) {
            keys.push_back(b.first);
        }
        return keys;
    }

    /// @brief create a new backend object
    /// @param name the name of the backend resource
    /// @return a new QkrtBackend class
    Qiskit::providers::QkrtBackend backend(std::string name)
    {
        get_backends();

        qkrt_Backend* backend = backends_[name];
        if (backend == nullptr) {
            std::cerr << "ERROR: backend " << name << " is not provided in this service" << std::endl;
            return Qiskit::providers::QkrtBackend();
        }

        return Qiskit::providers::QkrtBackend(service_, backend);
    }

    /// @brief find the least busy backend
    /// @return a new QkrtBackend class
    Qiskit::providers::QkrtBackend least_busy(void)
    {
        get_backends();
        qkrt_Backend *least_busy = qkrt_backend_search_results_least_busy(backend_results_);

        return Qiskit::providers::QkrtBackend(service_, least_busy);
    }

protected:
    bool get_backends(void)
    {
        if (backend_results_ != nullptr)
            return true;

        int ret = qkrt_backend_search(&backend_results_, service_.get());
        if (ret != 0) {
            std::cerr << "ERROR : qkrt_backend_search fails with code " << ret << std::endl;
            return false;
        }
        uint_t count = qkrt_backend_search_results_length(backend_results_);

        // make map of backend name and pointer
        qkrt_Backend **backends = qkrt_backend_search_results_data(backend_results_);
        for (uint_t i = 0; i < count; i++) {
            std::string name = qkrt_backend_name(backends[i]);
            backends_[name] = backends[i];
        }
        return true;
    }
};

} // namespace service
} // namespace Qiskit

#endif //__qiskitcpp_providers_qiskit_runtime_service_def_hpp__

#endif

