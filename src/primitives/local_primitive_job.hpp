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

// primitive job class for local calculation

#ifndef __qiskitcpp_primitives_local_primitive_job_def_hpp__
#define __qiskitcpp_primitives_local_primitive_job_def_hpp__

#include "primitives/base/base_primitive_job.hpp"


namespace Qiskit {
namespace primitives {

/// @class LocalPrimitiveJob
/// @brief Qiskit representation of a primitive job.
class LocalPrimitiveJob : public BasePrimitiveJob {
protected:
    PrimitiveResult result_;
public:
    /// @brief Create a new LocalPrimitiveJob
    LocalPrimitiveJob() {}

    /// @brief Create a new LocalPrimitiveJob
    /// @param id a unique id identifying the job.
    LocalPrimitiveJob(std::string id) : BasePrimitiveJob(id) {}

    /// @brief Return the results of the job.
    /// @return a PrimitiveResult
    PrimitiveResult& result(void) override
    {
        return result_;
    }

    /// @brief Return whether the job has successfully run.
    /// @return true if successfully run, otherwise false.
    bool done(void) override
    {
        return true;
    }

    /// @brief Return whether the job is actively running.
    /// @return true if job is actively running, otherwise false.
    bool running(void) override
    {
        return false;
    }

    /// @brief Return whether the job has been cancelled.
    /// @return true if job has been cancelled, otherwise false.
    bool cancelled(void) override
    {
        return false;
    }

    /// @brief Return whether the job is in a final job state such as DONE or ERROR.
    /// @return true if job is in a final job state, otherwise false.
    bool in_final_state(void) override
    {
        return true;
    }

    /// @brief Attempt to cancel the job.
    void cancel(void) override {}
};

} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_base_primitive_job_def_hpp__


