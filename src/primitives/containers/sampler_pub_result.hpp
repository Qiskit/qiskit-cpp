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

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "primitives/containers/bit_array.hpp"

namespace Qiskit {
namespace primitives {

/// @class SamplerPubResult
/// @brief Result of Sampler Pub(Primitive Unified Bloc).
class SamplerPubResult {
protected:
    BitArray data_;
    SamplerPub pub_;
public:
    /// @brief Create a new SamplerPubResult
    SamplerPubResult() {}

    /// @brief Create a new SamplerPubResult
    /// @param pub a pub for this result
    SamplerPubResult(SamplerPub& pub)
    {
        pub_ = pub;
    }

    /// @brief Create a new SamplerPubResult as a copy of src.
    /// @param src copy source.
    SamplerPubResult(const SamplerPubResult& src)
    {
        data_ = src.data_;
        pub_ = src.pub_;
    }

    /// @brief Result data for the pub.
    BitArray& data(void)
    {
        return data_;
    }

    /// @brief get pub for this result
    /// @return pub
    const SamplerPub& pub(void) const
    {
        return pub_;
    }

    /// @brief set pub for this result
    /// @param pub to be set
    void set_pub(SamplerPub& pub)
    {
        pub_ = pub;
    }

    /// @brief Set pub reuslt from json
    void from_json(json& input) {
        data_.set_bits(pub_.circuit().get_measure_map().size());
        data_.from_json(input);
    }


};

} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_sampler_pub_result_hpp__
