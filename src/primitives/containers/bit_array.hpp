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

// bit array to store sampling results

#ifndef __qiskitcpp_primitives_bit_array_hpp__
#define __qiskitcpp_primitives_bit_array_hpp__

#include <nlohmann/json.hpp>

#include <unordered_map>
#include "utils/bitvector.hpp"


namespace Qiskit {
namespace primitives {

/// @class BitArray
/// @brief Stores an array of bit values.
class BitArray {
protected:
    std::vector<BitVector> array_;
    uint_t num_bits_;
public:
    /// @brief Create a new BitArray
    BitArray()
    {
        num_bits_ = 0;
    }

    /// @brief Create a BitArray from other
    /// @param src BitArrya to be copied
    BitArray(const BitArray& src)
    {
        array_ = src.array_;
        num_bits_ = src.num_bits_;
    }

    /// @brief Resize this BitArray with the specified num_samples and num_bits
    /// @param num_samples number of samples (shots) saved in this array
    /// @param num_bits number of bits for each bitstring
    void allocate(uint_t num_samples, uint_t num_bits)
    {
        array_.resize(num_samples, BitVector(num_bits));
        num_bits_ = num_bits;
    }

    /// @brief Return the number of bits
    /// @return the number of bits
    uint_t num_bits(void)
    {
        return num_bits_;
    }

    /// @brief set number of bits
    /// @param bits
    void set_bits(uint_t bits)
    {
        num_bits_ = bits;
    }

    /// @brief Return the number of shots sampled from the register in each configuration.
    /// @return The number of shots sampled from the register in each configuration.
    uint_t num_shots(void)
    {
        return array_.size();
    }

    // from simulator samples (< 64 qubits)
    void from_samples(const reg_t& samples, uint_t num_bits);
    void from_samples(const uint_t* samples, uint_t num_samples, uint_t num_bits);
    // from bitstring
    void from_bitstring(const std::vector<std::string>& samples);

    /// @brief Return a list of bitstrings.
    /// @return A list of bitstrings.
    std::vector<std::string> get_bitstring(void);

    /// @brief Return a list of hex string
    /// @return A list of hex string.
    std::vector<std::string> get_hexstring(void);

    /// @brief Return a counts dictionary with bitstring keys.
    /// @return A counts dictionary with bitstring keys.
    std::unordered_map<std::string, uint_t> get_counts(void);

    /// @brief Set pub samples from json
    /// @param input JSON input
    void from_json(nlohmann::ordered_json& input);
};

void BitArray::from_samples(const reg_t& samples, uint_t num_bits)
{
    num_bits_ = num_bits;
    array_.resize(samples.size());
    for (uint_t i = 0; i < samples.size(); i++) {
        array_[i].from_uint(samples[i], num_bits);
    }
}

void BitArray::from_samples(const uint_t* samples, uint_t num_samples, uint_t num_bits)
{
    num_bits_ = num_bits;
    array_.resize(num_samples);
    for (uint_t i = 0; i < num_samples; i++) {
        array_[i].from_uint(samples[i], num_bits);
    }
}

void BitArray::from_bitstring(const std::vector<std::string>& samples)
{
    array_.resize(samples.size());
    for (uint_t i = 0; i < samples.size(); i++) {
        array_[i].from_string(samples[i]);
    }
    num_bits_ = array_[0].size();
}

std::vector<std::string> BitArray::get_bitstring(void)
{
    std::vector<std::string> ret(array_.size());
    for (uint_t i = 0; i < array_.size(); i++) {
        ret[i] = array_[i].to_string();
    }
    return ret;
}

std::vector<std::string> BitArray::get_hexstring(void)
{
    std::vector<std::string> ret(array_.size());
    for (uint_t i = 0; i < array_.size(); i++) {
        ret[i] = array_[i].to_hex_string();
    }
    return ret;
}

std::unordered_map<std::string, uint_t> BitArray::get_counts(void)
{
    std::unordered_map<std::string, uint_t> ret;
    for (uint_t i = 0; i < array_.size(); i++) {
        ret[array_[i].to_string()]++;
    }
    return ret;
}

void BitArray::from_json(nlohmann::ordered_json& input)
{
    auto samples = input["data"]["c"]["samples"];
    auto num_bits = input["data"]["c"]["num_bits"];
    auto num_shots = samples.size();
    if (num_bits_ == 0)
        num_bits_ = num_bits;
    allocate(num_shots, num_bits_);
    for (uint_t i = 0; i < num_shots; i++) {
        array_[i].from_hex_string(samples[i]);
    }
}


} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_bit_array_hpp__
