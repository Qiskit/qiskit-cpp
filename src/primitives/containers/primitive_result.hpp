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
using json = nlohmann::json;

#include "primitives/containers/sampler_pub_result.hpp"


namespace Qiskit {
namespace primitives {

/// @class PrimitiveResult
/// @brief A container for multiple pub results and global metadata. Only SamplerPub is supported.
class PrimitiveResult {
protected:
    std::vector<SamplerPubResult> pub_results_;
    std::vector<bool> pub_allocated_;
    json json_;
public:
    /// @brief Create a new PrimitiveResult
    PrimitiveResult() {}

    /// @brief Create a new PrimitiveResult from string
    /// @param str input string
    PrimitiveResult(std::string str)
    {
        from_string(str);
    }

    /// @brief Return the number of PUBs in this result
    /// @return The number of PUBs.
    uint_t size(void)
    {
        return pub_results_.size();
    }

    SamplerPubResult& operator[](uint_t i)
    {
        if (!pub_allocated_[i]) {
            pub_results_[i].from_json(json_["results"][i]);
            pub_allocated_[i] = true;
        }
        return pub_results_[i];
    }

    /// @brief Return json object containing results
    /// @return The reference to json object
    json& json(void)
    {
        return json_;
    }

    /// @brief Create a new PrimitiveResult from string
    /// @param str input string
    void from_string(std::string str)
    {
        json_ = json::parse(str);

        auto num_results = json_["results"].size();
        pub_results_.resize(num_results);
        pub_allocated_.resize(num_results);
        // allocating pub on demand to save memory
        for (int i = 0; i < num_results; i++) {
            pub_allocated_[i] = false;
        }
    }
};

} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_result_hpp__
