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

// estimator pub result class

#ifndef __qiskitcpp_primitives_estimator_pub_result_hpp__
#define __qiskitcpp_primitives_estimator_pub_result_hpp__

#include <vector>

#include "primitives/containers/estimator_pub.hpp"

namespace Qiskit {
namespace primitives {

/// @class EstimatorPubResult
/// @brief Result of Estimator Pub(Primitive Unified Bloc).
class EstimatorPubResult {
protected:
    EstimatorPub pub_;
    std::vector<double> evs_;
    std::vector<double> stds_;
    uint_t shots_ = 0;
    double target_precision_ = 0.0;

public:
    /// @brief Create a new EstimatorPubResult.
    EstimatorPubResult() {}

    /// @brief Create a new EstimatorPubResult.
    EstimatorPubResult(const EstimatorPub& pub,
                       const std::vector<double>& evs,
                       const std::vector<double>& stds,
                       uint_t shots,
                       double target_precision)
        : pub_(pub), evs_(evs), stds_(stds), shots_(shots),
          target_precision_(target_precision)
    {
    }

    /// @brief Return the pub for this result.
    const EstimatorPub& pub(void) const
    {
        return pub_;
    }

    /// @brief Return expectation values.
    const std::vector<double>& evs(void) const
    {
        return evs_;
    }

    /// @brief Return standard errors.
    const std::vector<double>& stds(void) const
    {
        return stds_;
    }

    /// @brief Return the number of shots used for this pub.
    uint_t shots(void) const
    {
        return shots_;
    }

    /// @brief Return resolved target precision for this pub.
    double target_precision(void) const
    {
        return target_precision_;
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_estimator_pub_result_hpp__
