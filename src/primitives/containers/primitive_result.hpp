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

// primitive result class

#ifndef __qiskitcpp_primitives_result_hpp__
#define __qiskitcpp_primitives_result_hpp__

#include <nlohmann/json.hpp>

#include "primitives/containers/sampler_pub_result.hpp"


namespace Qiskit {
namespace primitives {

/// @class PrimitiveResult
/// @brief A container for multiple pub results and global metadata. Only SamplerPub is supported.
class PrimitiveResult {
protected:
    std::vector<SamplerPubResult> pub_results_;     // a list of pub results
public:
    /// @brief Create a new PrimitiveResult
    PrimitiveResult() {}


    /// @brief allocate pub results
    /// @param num_results number of pub results to be allocated
    void allocate(uint_t num_results)
    {
        pub_results_.resize(num_results);
    }

    /// @brief Return the number of PUBs in this result
    /// @return The number of PUBs.
    uint_t size(void)
    {
        return pub_results_.size();
    }

    /// @brief Return the pub result
    /// @param i index of pub
    /// @return The pub result
    SamplerPubResult& operator[](uint_t i)
    {
        return pub_results_[i];
    }

    /// @brief set pubs in the results
    /// @param pubs a list of pubs to be set
    void set_pubs(const std::vector<SamplerPub>& pubs)
    {
        if (pubs.size() == pub_results_.size()) {
            for (int i = 0; i < pub_results_.size(); i++) {
                pub_results_[i].set_pub(pubs[i]);
            }
        }
    }
};

} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_result_hpp__
