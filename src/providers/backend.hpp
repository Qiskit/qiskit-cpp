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
#include "transpiler/target.hpp"
#include "primitives/containers/sampler_pub.hpp"
#include "providers/job.hpp"

namespace Qiskit {
namespace providers {

/// @class BackendV2
/// @brief BackendV2 base class.
class BackendV2 {
protected:
    std::string name_;
public:
    /// @brief Create a new BackendV2
    BackendV2() {}

    /// @brief Create a new BackendV2
    /// @param name a name for backend.
    BackendV2(std::string name)
    {
        name_ = name;
    }

    /// @brief Create a new BackendV2
    BackendV2(BackendV2& other)
    {
        name_ = other.name_;
    }

    ~BackendV2()
    {
    }

    /// @brief Return a name of backend resource.
    /// @return a name of backend resource.
    std::string& name(void)
    {
        return name_;
    }

    /// @brief Return a target properties for this backend
    /// @return a target class
    virtual std::shared_ptr<transpiler::Target> target(void) = 0;

    /// @brief Run and collect samples from each pub.
    /// @param pubs An iterable of pub-like objects.
    /// @return PrimitiveJob
    virtual std::shared_ptr<providers::Job> run(std::vector<primitives::SamplerPub>& circuits, uint_t shots = 0) = 0;

};

} // namespace providers
} // namespace Qiskit


#endif //__qiskitcpp_providers_backend_def_hpp__


