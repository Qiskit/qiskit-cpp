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

#include <unordered_map>


#ifndef __qiskitcpp_primitives_sampler_pub_result_hpp__
#define __qiskitcpp_primitives_sampler_pub_result_hpp__

#include <nlohmann/json.hpp>

#include "primitives/containers/bit_array.hpp"

namespace Qiskit {
namespace primitives {

/// @class SamplerPubResult
/// @brief Result of Sampler Pub(Primitive Unified Bloc).
class SamplerPubResult {
protected:
    std::unordered_map<std::string, BitArray> data_;     // in pair of creg name and bitstrings
    SamplerPub pub_;    //
public:
    /// @brief Create a new SamplerPubResult
    SamplerPubResult() {}

    /// @brief Create a new SamplerPubResult
    /// @param pub a pub for this result
    SamplerPubResult(SamplerPub& pub)
    {
        pub_ = pub;
        for (auto creg : pub_.circuit().cregs()) {
            data_[creg.name()] = BitArray();
            data_[creg.name()].set_bits(creg.size());
        }
    }

    /// @brief Create a new SamplerPubResult as a copy of src.
    /// @param src copy source.
    SamplerPubResult(const SamplerPubResult& src)
    {
        data_ = src.data_;
        pub_ = src.pub_;
    }

    /// @brief Result data for the pub.
    /// @return the bitarray for the first creg in the pub
    BitArray& data(void)
    {
        return data_[pub_.circuit().cregs()[0].name()];
    }

    /// @brief Result data for the pub.
    /// @param name the name of the creg
    /// @return the bitarray for the creg name in the pub
    BitArray& data(const std::string& name)
    {
        return data_[name];
    }

    /// @brief Result data for the pub.
    /// @param creg the creg to be returned
    /// @return the bitarray for the creg in the pub
    BitArray& data(const circuit::ClassicalRegister& creg)
    {
        return data_[creg.name()];
    }

    /// @brief get pub for this result
    /// @return pub
    const SamplerPub& pub(void) const
    {
        return pub_;
    }

    /// @brief set pub for this result
    /// @param pub to be set
    void set_pub(const SamplerPub& pub)
    {
        pub_ = pub;
        for (auto creg : pub_.circuit().cregs()) {
            data_[creg.name()] = BitArray();
            data_[creg.name()].set_bits(creg.size());
        }
    }

    /// @brief Set pub reuslt from json
    bool from_json(nlohmann::ordered_json& input)
    {
        if (!input.contains("data")) {
            std::cerr << " SamplerPubResult Error : JSON result does not contain data section " << std::endl;
            return false;
        }
        if (!input["data"].contains("c")) {
            std::cerr << " SamplerPubResult Error : JSON result does not contain creg section " << std::endl;
            return false;
        }

        auto data = input["data"]["c"];

        uint_t total_bits = 0;
        for (auto creg : pub_.circuit().cregs()) {
            total_bits += creg.size();
        }
        // read all bits
        BitArray allbits;
        allbits.set_bits(total_bits);
        allbits.from_json(data);

        total_bits = 0;
        for (auto creg : pub_.circuit().cregs()) {
            data_[creg.name()] = allbits.get_subset(total_bits, creg.size());
            total_bits += creg.size();
        }

        return true;
    }

    /// @brief allocate bit array data
    /// @param num_samples number of samples to be allocated
    void allocate(uint_t num_samples)
    {
        for (auto creg : pub_.circuit().cregs()) {
            data_[creg.name()] = BitArray();
            data_[creg.name()].allocate(num_samples, creg.size());
        }
    }

};

} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_sampler_pub_result_hpp__
