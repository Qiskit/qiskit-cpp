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

#include "primitives/containers/sampler_pub_result.hpp"


namespace qiskitcpp {
namespace primitives {

/// @class PrimitiveResult
/// @brief A container for multiple pub results and global metadata. Only SamplerPub is supported.
class PrimitiveResult {
protected:
    std::vector<SamplerPubResult> pub_results_;
public:
    /// @brief Create a new PrimitiveResult
    PrimitiveResult() {}

    /// @brief Create a new PrimitiveResult
    /// @param num_pubs The number of PUBs
    PrimitiveResult(uint_t num_pubs)
    {
        pub_results_.resize(num_pubs);
    }

    /// @brief Create a new PrimitiveResult
    /// @param pub_results Pub results.
    PrimitiveResult(std::vector<SamplerPubResult> pub_results) : pub_results_(pub_results) {}

    /// @brief Return the number of PUBs in this result
    /// @return The number of PUBs.
    uint_t size(void)
    {
        return pub_results_.size();
    }

    SamplerPubResult& operator[](uint_t i)
    {
        return pub_results_[i];
    }

};

} // namespace primitives
} // namespace qiskitcpp


#endif //__qiskitcpp_primitives_result_hpp__
