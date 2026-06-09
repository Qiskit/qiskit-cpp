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

// job class for BackendEstimator

#ifndef __qiskitcpp_primitives_backend_estimator_job_def_hpp__
#define __qiskitcpp_primitives_backend_estimator_job_def_hpp__

#include <chrono>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

#include "primitives/containers/estimator_result.hpp"
#include "providers/job.hpp"
#include "providers/jobstatus.hpp"

namespace Qiskit {
namespace primitives {

/// @class BackendEstimatorJob
/// @brief Job class for Backend Estimator primitive.
class BackendEstimatorJob {
protected:
    std::shared_ptr<providers::Job> job_ = nullptr;
    std::vector<EstimatorPub> pubs_;

public:
    BackendEstimatorJob(std::shared_ptr<providers::Job> job, std::vector<EstimatorPub>& pubs)
    {
        job_ = job;
        pubs_ = pubs;
    }

    BackendEstimatorJob(const BackendEstimatorJob& other)
    {
        job_ = other.job_;
        pubs_ = other.pubs_;
    }

    ~BackendEstimatorJob()
    {
        if (job_) {
            job_.reset();
        }
    }

    const std::vector<EstimatorPub>& pubs(void) const
    {
        return pubs_;
    }

    providers::JobStatus status(void)
    {
        if (!job_) {
            return providers::JobStatus::ERROR;
        }
        return job_->status();
    }

    bool running(void)
    {
        return status() == providers::JobStatus::RUNNING;
    }

    bool queued(void)
    {
        return status() == providers::JobStatus::QUEUED;
    }

    bool done(void)
    {
        return status() == providers::JobStatus::DONE;
    }

    bool cancelled(void)
    {
        return status() == providers::JobStatus::CANCELLED;
    }

    bool in_final_state(void)
    {
        return is_final_state(status());
    }

    bool cancel(void)
    {
        return false;
    }

    EstimatorPrimitiveResult result(void)
    {
        if (!job_) {
            throw std::runtime_error("BackendEstimatorJob has no provider job.");
        }

        providers::JobStatus st;
        while (true) {
            st = job_->status();
            if (is_final_state(st)) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        if (st != providers::JobStatus::DONE) {
            throw std::runtime_error("BackendEstimatorJob did not finish successfully.");
        }

        uint_t num_results = job_->num_results();
        if (num_results != pubs_.size()) {
            throw std::runtime_error("BackendEstimatorJob result count does not match the submitted pubs.");
        }

        EstimatorPrimitiveResult result;
        result.allocate(num_results);
        for (uint_t i = 0; i < num_results; i++) {
            result[i].set_pub(pubs_[i]);
            if (!job_->result(i, result[i])) {
                throw std::runtime_error("BackendEstimatorJob failed to read an estimator pub result.");
            }
        }
        return result;
    }

protected:
    static bool is_final_state(providers::JobStatus status)
    {
        return status == providers::JobStatus::DONE ||
               status == providers::JobStatus::CANCELLED ||
               status == providers::JobStatus::FAILED ||
               status == providers::JobStatus::ERROR;
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_backend_estimator_job_def_hpp__
