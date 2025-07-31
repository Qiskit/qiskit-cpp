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

// Sampler V2 interface definition

#ifndef __qiskitcpp_primitives_base_sampler_def_hpp__
#define __qiskitcpp_primitives_base_sampler_def_hpp__
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "circuit/quantumcircuit.hpp"
#include "primitives/containers/sampler_pub.hpp"
#include "primitives/base/base_primitive_job.hpp"

namespace Qiskit {
namespace primitives {

/// @class BaseSamplerV2
/// @brief Implementation of SamplerV2 interface.
template <typename Job>
class BaseSamplerV2 {
protected:
    uint_t shots_;
    std::vector<SamplerPub> pubs_;
public:
    /// @brief Create a new StatevectorSampler
    /// @param default_shots The default shots
    BaseSamplerV2(uint_t shots = 1024)
    {
        shots_ = shots;
    }

    /// @brief Run and collect samples from each pub.
    /// @pubs An iterable of pub-like objects.
    /// @return PrimitiveJob
    virtual std::shared_ptr<Job> run(std::vector<SamplerPub> pubs) = 0;

    /// @brief Return a JSON format sampler job data for backend
    /// @return a JSON format sampler pubs
    nlohmann::ordered_json to_json(void)
    {
        nlohmann::ordered_json sampler;
        json pubs = json::array();
        for (int i = 0; i < pubs_.size(); i++) {
            pubs += pubs_[i].to_json();
        }
        sampler["pubs"] = pubs;

        sampler["version"] = 2;
        sampler["support_qiskit"] = false;
        sampler["shots"] = shots_;
        sampler["options"] = json::object();

        return sampler;
    }
};


} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_base_sampler_def_hpp__
