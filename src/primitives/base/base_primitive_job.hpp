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

// Base primitive job class

#ifndef __qiskitcpp_primitives_base_primitive_job_def_hpp__
#define __qiskitcpp_primitives_base_primitive_job_def_hpp__

#include "primitives/containers/primitive_result.hpp"


namespace Qiskit {
namespace primitives {

/// @class BasePrimitiveJob
/// @brief Primitive job abstract base class.
class BasePrimitiveJob {
protected:
    std::string job_id_;
public:
    /// @brief Create a new BasePrimitiveJob
    BasePrimitiveJob() {}

    /// @brief Create a new BasePrimitiveJob
    /// @param id a unique id identifying the job.
    BasePrimitiveJob(std::string id) : job_id_(id) {}

    /// @brief Return a unique id identifying the job.
    /// @return a unique id identifying the job.
    std::string& job_id(void)
    {
        return job_id_;
    }

    /// @brief Return the results of the job.
    virtual PrimitiveResult& result(void) = 0;

    /// @brief Return whether the job has successfully run.
    /// @return true if successfully run, otherwise false.
    virtual bool done(void) = 0;

    /// @brief Return whether the job is actively running.
    /// @return true if job is actively running, otherwise false.
    virtual bool running(void) = 0;

    /// @brief Return whether the job has been cancelled.
    /// @return true if job has been cancelled, otherwise false.
    virtual bool cancelled(void) = 0;

    /// @brief Return whether the job is in a final job state such as DONE or ERROR.
    /// @return true if job is in a final job state, otherwise false.
    virtual bool in_final_state(void) = 0;

    /// @brief Attempt to cancel the job.
    virtual void cancel(void) = 0;
};

} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_base_primitive_job_def_hpp__


