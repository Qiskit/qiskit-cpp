/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2026.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.
*/

// estimator result class

#ifndef __qiskitcpp_primitives_estimator_result_hpp__
#define __qiskitcpp_primitives_estimator_result_hpp__

#include <vector>

#include "primitives/containers/estimator_pub_result.hpp"

namespace Qiskit {
namespace primitives {

/// @class EstimatorResult
/// @brief A container for multiple estimator pub results.
class EstimatorResult {
protected:
    std::vector<EstimatorPubResult> pub_results_;

public:
    /// @brief Create a new EstimatorResult.
    EstimatorResult() {}

    /// @brief Add an estimator pub result.
    void push_back(const EstimatorPubResult& result)
    {
        pub_results_.push_back(result);
    }

    /// @brief Return the number of PUBs in this result.
    uint_t size(void) const
    {
        return pub_results_.size();
    }

    /// @brief Return the pub result.
    EstimatorPubResult& operator[](uint_t i)
    {
        return pub_results_[i];
    }

    /// @brief Return the pub result.
    const EstimatorPubResult& operator[](uint_t i) const
    {
        return pub_results_[i];
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_estimator_result_hpp__
