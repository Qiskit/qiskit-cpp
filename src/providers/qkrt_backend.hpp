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

// BackendV2 implemented on Qiskit IBM Runtime Rust C-API

#ifndef __qiskitcpp_providers_Qiskit_Runtime_backend_def_hpp__
#define __qiskitcpp_providers_Qiskit_Runtime_backend_def_hpp__

#include "utils/types.hpp"
#include "providers/backend.hpp"
#include "providers/qkrt_job.hpp"

#include "qiskit.h"

extern "C" {
    #include "qiskit_ibm_runtime/qiskit_ibm_runtime.h"
}

// rename Job object to differ from Qiskit::providers::Job same for Backend and Service
using qkrt_Job = Job;
using qkrt_Backend = Backend;
using qkrt_Service = Service;



namespace Qiskit {
namespace providers {

/// @class QkrtBackend
/// @brief Backend class for Qiskit IBM runtime Rust C-API.
class QkrtBackend : public BackendV2 {
protected:
    std::string primitive_name_ = "sampler";
    qkrt_Backend* backend_ = nullptr;
    std::shared_ptr<qkrt_Service> service_;
    std::shared_ptr<transpiler::Target> target_ = nullptr;
public:
    /// @brief Create a new QkrtBackend
    QkrtBackend() {}

    /// @brief Create a new QkrtBackend object
    /// @param backend a backenbd object of Qiskit IBM runtime C-API.
    QkrtBackend(std::shared_ptr<qkrt_Service> service, qkrt_Backend* backend)
    {
        service_ = service;
        backend_ = backend;
        name_ = qkrt_backend_name(backend);
    }

    /// @brief Create a new QkrtBackend from other
    QkrtBackend(const QkrtBackend& other)
    {
        service_ = other.service_;
        backend_ = other.backend_;
        name_ = other.name_;
        primitive_name_ = other.primitive_name_;
        target_ = other.target_;
    }

    ~QkrtBackend()
    {
        if (service_)
            service_.reset();
        if (target_)
            target_.reset();
    }

    /// @brief Return a target properties for this backend
    /// @return a target class
    std::shared_ptr<transpiler::Target> target(void) override
    {
        if (target_) {
            return target_;
        }

        QkTarget *target = qkrt_get_backend_target(service_.get(), backend_);
        if (target == nullptr) {
            std::cerr << "ERROR: failed to get target for the backend " << name_ << std::endl;
            return nullptr;
        }
        target_ = std::make_shared<transpiler::Target>(target);
        return target_;
    }

    /// @brief Run and collect samples from each pub.
    /// @param pubs An iterable of pub-like objects.
    /// @return PrimitiveJob
    std::shared_ptr<providers::Job>  run(std::vector<primitives::SamplerPub>& input_pubs, uint_t in_shots) override
    {
        qkrt_Job* job;
        uint_t shots = in_shots;
        if (shots == 0)
            shots = input_pubs[0].shots();

        auto circuit = input_pubs[0].circuit();
        int ret = qkrt_sampler_job_run(&job, service_.get(), backend_, circuit.get_rust_circuit().get(), shots, NULL);

        return std::make_shared<providers::QkrtJob>(service_, std::shared_ptr<qkrt_Job>(job, qkrt_job_free));
    }
};

} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_providers_Qiskit_Runtime_backend_def_hpp__


