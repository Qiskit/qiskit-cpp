/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2025.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.
*/

// Job class for SQC

#ifndef __qiskitcpp_providers_SQC_job_hpp__
#define __qiskitcpp_providers_SQC_job_hpp__

#include <nlohmann/json.hpp>

#include "utils/types.hpp"

#include "primitives/containers/sampler_pub_result.hpp"
#include "providers/job.hpp"

namespace Qiskit {
namespace providers {

/// @class SQCJob
/// @brief Job class for SQC
class SQCJob : public Job {
private:
    std::string job_id_;
    nlohmann::ordered_json results_;   // json formatted results (converted output from SQC)
    uint_t num_results_ = 0;

public:
    /// @brief Create a new SQCBackend
    SQCJob()
        : SQCJob(std::string{""})
    {}

    /// @brief Create a new SQCBackend object
    SQCJob(const std::string& job_id)
        : job_id_(job_id),
          num_results_(0)
    {}

    /// @note [TODO] This constructor will be removed after SQC API provides a async job execution
    SQCJob(const nlohmann::ordered_json results)
        : job_id_(""),
          num_results_(results["results"].size()),
          results_(results)
    {}

    /// @brief Create a new SQCJob from other
    SQCJob(const SQCJob& other)
    {
        job_id_ = other.job_id_;
        num_results_ = other.num_results_;
        results_ = other.results_;
    }

    ~SQCJob() {}

    /// @brief Return the status of the job.
    /// @return JobStatus enum.
    providers::JobStatus status(void) override
    {
        /// @todo Wait SQC API for making the status request API public.
        return providers::JobStatus::DONE;
    }


    /// @brief Return the number of results in this job
    /// @return number of results
    uint_t num_results(void) override
    {
        return num_results_;
    }

    /// @brief get sampler pub result
    /// @param index an index of the result
    /// @param result an output sampler pub result
    /// @return true if result is successfully set
    bool result(uint_t index, primitives::SamplerPubResult& result) override
    {
        if (index >= num_results_)
            return false;

        result.from_json(results_["results"][index]);

        return true;
    }
};

} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_providers_SQC_job_hpp__


