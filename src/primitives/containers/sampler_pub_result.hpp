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

// sampler pub result class

#ifndef __qiskitcpp_primitives_sampler_pub_result_hpp__
#define __qiskitcpp_primitives_sampler_pub_result_hpp__

#include "primitives/containers/bit_array.hpp"

namespace Qiskit {
namespace primitives {

/// @class SamplerPubResult
/// @brief Result of Sampler Pub(Primitive Unified Bloc).
class SamplerPubResult {
protected:
    BitArray data_;
public:
    /// @brief Create a new SamplerPubResult
    SamplerPubResult() {}

    /// @brief Create a new SamplerPubResult as a copy of src.
    /// @param src copy source.
    SamplerPubResult(const SamplerPubResult& src) : data_(src.data_) {}

    /// @brief Result data for the pub.
    BitArray& data(void)
    {
        return data_;
    }

};

} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_sampler_pub_result_hpp__
