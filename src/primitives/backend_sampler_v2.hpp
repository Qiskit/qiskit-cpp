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

// sampler implementation for a backend

#ifndef __qiskitcpp_primitives_stab_sampler_def_hpp__
#define __qiskitcpp_primitives_stab_sampler_def_hpp__

#include "circuit/quantumcircuit.hpp"
#include "primitives/containers/sampler_pub.hpp"
#include "providers/backend.hpp"

#include "primitives/backend_sampler_job.hpp"

namespace Qiskit {
namespace primitives {

/// @class BackendSamplerV2
/// @brief Implementation of SamplerV2 on a backend
class BackendSamplerV2 {
protected:
    uint_t shots_;
    providers::BackendV2& backend_;
public:
    /// @brief Create a new BackendSamplerV2
    /// @param default_shots The default shots
    BackendSamplerV2(providers::BackendV2& backend, uint_t shots = 1024) : shots_(shots), backend_(backend)
    {

    }

    /// @brief return reference to backend object
    /// @return backendV2 object
    const providers::BackendV2& backend(void) const
    {
        return backend_;
    }

    /// @brief Run and collect samples from each pub.
    /// @pubs An iterable of pub-like objects.
    /// @return PrimitiveJob
    std::shared_ptr<BasePrimitiveJob> run(std::vector<SamplerPub> pubs)
    {
        return backend_.run(pubs, shots_);
    }

};

} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_stab_sampler_def_hpp__
