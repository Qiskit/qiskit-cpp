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

// estimator pub result class

#ifndef __qiskitcpp_primitives_estimator_pub_result_hpp__
#define __qiskitcpp_primitives_estimator_pub_result_hpp__

#include "primitives/containers/estimator_pub.hpp"

namespace Qiskit {
namespace primitives {

/// @class EstimatorPubResult
/// @brief Result of Estimator Pub (Primitive Unified Bloc).
class EstimatorPubResult {
protected:
    EstimatorPub pub_;
    double ev_ = 0.0;
    double stddev_ = 0.0;
public:
    EstimatorPubResult() {}

    EstimatorPubResult(const EstimatorPub& pub) : pub_(pub) {}

    EstimatorPubResult(const EstimatorPubResult& src)
    {
        pub_ = src.pub_;
        ev_ = src.ev_;
        stddev_ = src.stddev_;
    }

    const EstimatorPub& pub(void) const
    {
        return pub_;
    }

    void set_pub(const EstimatorPub& pub)
    {
        pub_ = pub;
    }

    double ev(void) const
    {
        return ev_;
    }

    double stddev(void) const
    {
        return stddev_;
    }

    void set_ev(double ev)
    {
        ev_ = ev;
    }

    void set_stddev(double stddev)
    {
        stddev_ = stddev;
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_estimator_pub_result_hpp__
