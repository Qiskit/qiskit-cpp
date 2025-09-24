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

// Base Job class to run single pub

#ifndef __qiskitcpp_providers_job_def_hpp__
#define __qiskitcpp_providers_job_def_hpp__

#include "utils/types.hpp"
#include "providers/jobstatus.hpp"
#include "primitives/containers/sampler_pub_result.hpp"

namespace Qiskit {
namespace providers {

/// @class Job
/// @brief Base Job class to manage single circuit run
class Job {
protected:
public:
    /// @brief Create a new Job
    Job() {}

    ~Job() {}

    /// @brief Return the status of the job.
    /// @return JobStatus enum.
    virtual providers::JobStatus status(void) = 0;

    /// @brief Return the number of results in this job
    /// @return number of results
    virtual uint_t num_results(void) = 0;

    /// @brief get sampler pub result
    /// @param index an index of the reuslt
    /// @param result an output sampler pub result
    /// @return true if result is successfully set
    virtual bool result(uint_t index, primitives::SamplerPubResult& result) = 0;
};

} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_providers_job_def_hpp__


