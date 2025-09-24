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

// Job class for Qiskit IBM Runtime Rust C-API

#ifndef __qiskitcpp_providers_Qiskit_Runtime_job_def_hpp__
#define __qiskitcpp_providers_Qiskit_Runtime_job_def_hpp__

#include "utils/types.hpp"
#include "primitives/containers/sampler_pub_result.hpp"
#include "providers/job.hpp"

extern "C" {
    #include "qiskit_ibm_runtime/qiskit_ibm_runtime.h"
}

// rename Job object to differ from Qiskit::providers::Job same for Backend and Service
using qkrt_Job = Job;
using qkrt_Backend = Backend;
using qkrt_Service = Service;

namespace Qiskit {
namespace providers {

/// @class QkrtJob
/// @brief Job class for Qiskit IBM runtime Rust C-API.
class QkrtJob : public providers::Job {
protected:
    std::shared_ptr<qkrt_Job> job_ = nullptr;
    std::shared_ptr<qkrt_Service> service_ = nullptr;
public:
    /// @brief Create a new QkrtBackend
    QkrtJob() {}

    /// @brief Create a new QkrtBackend object
    /// @param service a service object of Qiskit IBM runtime C-API.
    /// @param job a job object of Qiskit IBM runtime C-API.
    QkrtJob(std::shared_ptr<qkrt_Service> service, std::shared_ptr<qkrt_Job> job)
    {
        job_ = job;
        service_ = service;
    }

    /// @brief Create a new QkrtJob from other
    QkrtJob(const QkrtJob& other)
    {
        job_ = other.job_;
        service_ = other.service_;
    }

    ~QkrtJob()
    {
        if (job_)
            job_.reset();
        if (service_)
            service_.reset();
    }

    /// @brief Return the status of the job.
    /// @return JobStatus enum.
    providers::JobStatus status(void) override
    {
        uint32_t st;
        int ret = qkrt_job_status(&st, service_.get(), job_.get());
        if (ret != 0)
            return providers::JobStatus::FAILED;
        switch (st) {
            case 0:
                return providers::JobStatus::QUEUED;
            case 1:
                return providers::JobStatus::RUNNING;
            case 2:
                return providers::JobStatus::DONE;
            case 3:
            case 4:
                return providers::JobStatus::CANCELLED;
            case 5:
                return providers::JobStatus::FAILED;
        }
        return providers::JobStatus::FAILED;
    }

    /// @brief Return the number of results in this job
    /// @return number of results
    uint_t num_results(void) override
    {
        // we can run a single circuit currently
        return 1;
    }

    /// @brief get sampler pub result
    /// @param index an index of the reuslt
    /// @param result an output sampler pub result
    /// @return true if result is successfully set
    bool result(uint_t index, primitives::SamplerPubResult& result) override
    {
        Samples *samples;
        int ret = qkrt_job_results(&samples, service_.get(), job_.get());
        if (ret != 0) {
            std::cerr << "ERROR: qkrt_job_results fails with code " << ret << std::endl;
            return false;
        }
        size_t num_samples = qkrt_samples_num_samples(samples);
        result.allocate(num_samples);
        for (size_t i = 0; i< num_samples; i++) {
            char* sample = qkrt_samples_get_sample(samples, i);
            std::string ss(sample);
            result.data().set_hexstring(i, ss);
            qkrt_str_free(sample);
        }
        qkrt_samples_free(samples);

        return true;
    }

};

} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_providers_Qiskit_Runtime_backend_def_hpp__


