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

// Backend class

#ifndef __qiskitcpp_providers_backend_def_hpp__
#define __qiskitcpp_providers_backend_def_hpp__

#include "utils/types.hpp"

namespace Qiskit {
namespace providers {

/// @class BackendV2
/// @brief BackendV2 base class.
template <typename Job>
class BackendV2 {
protected:
    std::string name_;
    std::shared_ptr<Job> job_ = nullptr;
public:
    /// @brief Create a new BackendV2
    BackendV2() {}

    /// @brief Create a new BackendV2
    /// @param name a name for backend.
    BackendV2(std::string name)
    {
        name_ = name;
        job_ = std::make_shared<Job>(name);
    }

    /// @brief Create a new BackendV2
    BackendV2(BackendV2& other)
    {
        name_ = other.name_;
        job_ = other.job_;
    }

    ~BackendV2()
    {
        if (job_) {
            job_.reset();
        }
    }

    /// @brief Return a name of backend resource.
    /// @return a name of backend resource.
    std::string& name(void)
    {
        return name_;
    }

    /// @brief Return a job resource interface.
    /// @return a smart pointer to job interface.
    std::shared_ptr<Job> job(void)
    {
        return job_;
    }
};

} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_providers_backend_def_hpp__


