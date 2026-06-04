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

// estimator primitive result class

#ifndef __qiskitcpp_primitives_estimator_result_hpp__
#define __qiskitcpp_primitives_estimator_result_hpp__

#include "primitives/containers/estimator_pub.hpp"
#include "primitives/containers/estimator_pub_result.hpp"

namespace Qiskit {
namespace primitives {

/// @class EstimatorResult
/// @brief A container for multiple estimator pub results.
class EstimatorResult {
protected:
    std::vector<EstimatorPubResult> pub_results_;
public:
    EstimatorResult() {}

    void allocate(uint_t num_results)
    {
        pub_results_.resize(num_results);
    }

    uint_t size(void) const
    {
        return pub_results_.size();
    }

    EstimatorPubResult& operator[](uint_t i)
    {
        return pub_results_[i];
    }

    const EstimatorPubResult& operator[](uint_t i) const
    {
        return pub_results_[i];
    }

    void set_pubs(const std::vector<EstimatorPub>& pubs)
    {
        if (pubs.size() == pub_results_.size()) {
            for (uint_t i = 0; i < pub_results_.size(); i++) {
                pub_results_[i].set_pub(pubs[i]);
            }
        }
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_estimator_result_hpp__
