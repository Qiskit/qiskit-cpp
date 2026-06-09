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

#include <cmath>
#include <memory>
#include <stdexcept>
#include <vector>

#include "primitives/backend_estimator_job.hpp"
#include "primitives/containers/estimator_pub.hpp"
#include "providers/backend.hpp"

namespace Qiskit {
namespace primitives {

/// @class BackendEstimatorV2
/// @brief Implementation of EstimatorV2 on a backend.
class BackendEstimatorV2 {
protected:
    double default_precision_;
    providers::BackendV2& backend_;

public:
    BackendEstimatorV2(providers::BackendV2& backend, double default_precision = 0.015625)
        : default_precision_(default_precision), backend_(backend)
    {
        if (!valid_precision(default_precision_)) {
            throw std::invalid_argument("BackendEstimatorV2 default precision must be finite and non-negative.");
        }
    }

    const providers::BackendV2& backend(void) const
    {
        return backend_;
    }

    double default_precision(void) const
    {
        return default_precision_;
    }

    std::shared_ptr<BackendEstimatorJob> run(std::vector<EstimatorPub> pubs)
    {
        return run(pubs, default_precision_);
    }

    std::shared_ptr<BackendEstimatorJob> run(std::vector<EstimatorPub> pubs, double precision)
    {
        if (!valid_precision(precision)) {
            throw std::invalid_argument("Estimator run precision must be finite and non-negative.");
        }

        for (uint_t i = 0; i < pubs.size(); i++) {
            std::string message;
            if (!pubs[i].validate(&message)) {
                throw std::invalid_argument(message);
            }
            if (pubs[i].circuit().has_measurements()) {
                throw std::invalid_argument("Estimator circuits must not contain measurement operations.");
            }
            if (!pubs[i].has_precision()) {
                pubs[i].set_precision(precision);
            }
        }

        auto job = backend_.run(pubs, precision);
        if (!job) {
            return nullptr;
        }
        return std::make_shared<BackendEstimatorJob>(job, pubs);
    }

protected:
    static bool valid_precision(double precision)
    {
        return std::isfinite(precision) && precision >= 0.0;
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_backend_estimator_v2_def_hpp__
