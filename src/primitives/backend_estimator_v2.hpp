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

// estimator implementation for a backend

#ifndef __qiskitcpp_primitives_backend_estimator_v2_def_hpp__
#define __qiskitcpp_primitives_backend_estimator_v2_def_hpp__

#include <memory>
#include <vector>

#include "primitives/backend_estimator_job.hpp"
#include "primitives/containers/estimator_pub.hpp"
#include "providers/backend.hpp"

namespace Qiskit {
namespace primitives {

/// @class BackendEstimatorV2
/// @brief Implementation of EstimatorV2 on a backend
class BackendEstimatorV2 {
protected:
    uint_t shots_;
    providers::BackendV2& backend_;
public:
    /// @brief Create a new BackendEstimatorV2
    /// @param backend The backend to run circuits on
    /// @param default_shots The default shots
    BackendEstimatorV2(providers::BackendV2& backend, uint_t shots = 1024)
        : shots_(shots), backend_(backend)
    {
    }

    /// @brief Return reference to backend object
    const providers::BackendV2& backend(void) const
    {
        return backend_;
    }

    /// @brief Run and collect expectation values from each pub.
    /// @param pubs An iterable of estimator pub-like objects.
    /// @return BackendEstimatorJob
    std::shared_ptr<BackendEstimatorJob> run(std::vector<EstimatorPub> pubs)
    {
        return std::make_shared<BackendEstimatorJob>(backend_, shots_, pubs);
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_backend_estimator_v2_def_hpp__
