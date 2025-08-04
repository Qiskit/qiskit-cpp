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
#include "primitives/base/base_sampler.hpp"
#include "primitives/containers/sampler_pub.hpp"

#include "providers/backend.hpp"

namespace Qiskit {
namespace primitives {

/// @class BackendSamplerV2
/// @brief Implementation of SamplerV2 on a backend
template <typename Job>
class BackendSamplerV2 : public BaseSamplerV2<Job> {
protected:
    providers::BackendV2<Job> backend_;
public:
    /// @brief Create a new BackendSamplerV2
    /// @param default_shots The default shots
    BackendSamplerV2(providers::BackendV2<Job>& backend, uint_t shots = 1024) : BaseSamplerV2<Job>(shots)
    {
        backend_ = backend;
    }

    /// @brief Run and collect samples from each pub.
    /// @pubs An iterable of pub-like objects.
    /// @return PrimitiveJob
    std::shared_ptr<Job> run(std::vector<SamplerPub> pubs) override
    {
        this->pubs_ = pubs;

        /*
        if (backend_.job()->connect()) {
            backend_.job()->submit(this->to_json().dump(2));
        }
        */

        return backend_.job();
    }
};

} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_stab_sampler_def_hpp__
