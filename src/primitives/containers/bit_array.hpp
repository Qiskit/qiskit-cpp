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

    /// @brief Return subsets of the BitArray
    /// @param start_bit start bit index of subset
    /// @param num_bits number of bits in a subset
    /// @return A new BitArray
    BitArray get_subset(const uint_t start_bit, const uint_t num_bits);

    /// @brief Return a list of bitstrings.
    /// @return A list of bitstrings.
    std::vector<std::string> get_bitstrings(void);

    /// @brief Return a list of bitstrings.
    /// @param index a list of index to be stored in the output list
    /// @return A list of bitstrings.
    std::vector<std::string> get_bitstrings(reg_t& index);

    /// @brief Return a list of hex string
    /// @return A list of hex string.
    std::vector<std::string> get_hexstrings(void);

    /// @brief Return a list of hex string.
    /// @param index a list of index to be stored in the output list
    /// @return A list of hex string.
    std::vector<std::string> get_hexstrings(reg_t& index);

    /// @brief Return a counts dictionary with bitstring keys.
    /// @return A counts dictionary with bitstring keys.
    std::unordered_map<std::string, uint_t> get_counts(void);

    /// @brief Return a counts dictionary with bitstring keys.
    /// @param index a list of index to be stored in the output map
    /// @return A counts dictionary with bitstring keys.
    std::unordered_map<std::string, uint_t> get_counts(reg_t& index);

    /// @brief Set pub samples from json
    /// @param input JSON input
    void from_json(nlohmann::ordered_json& input);

    /// @brief Set pub sample from hexstring
    /// @param index an index to be set
    /// @param input a sample in a hex string format
    void set_hexstring(uint_t index, std::string& input);

    /// @brief Return a list of bit counts
    /// @return A list of interger counts of bits appears in each shot
    reg_t bitcount(void);

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

std::vector<std::string> BitArray::get_bitstrings(void)
{
    std::vector<std::string> ret(array_.size());
    for (uint_t i = 0; i < array_.size(); i++) {
        ret[i] = array_[i].to_string();
    }
    return ret;
}

std::vector<std::string> BitArray::get_bitstrings(reg_t& index)
{
    uint_t size = std::min(array_.size(), index.size());
    std::vector<std::string> ret(size);

    for (uint_t i = 0; i < size; i++) {
        uint_t pos = index[i];
        if (pos < array_.size())
            ret[i] = array_[pos].to_string();
    }
    return ret;
}

std::vector<std::string> BitArray::get_hexstrings(void)
{
    std::vector<std::string> ret(array_.size());
    for (uint_t i = 0; i < array_.size(); i++) {
        ret[i] = array_[i].to_hex_string();
    }
    return ret;
}

std::vector<std::string> BitArray::get_hexstrings(reg_t& index)
{
    uint_t size = std::min(array_.size(), index.size());
    std::vector<std::string> ret(size);

    for (uint_t i = 0; i < size; i++) {
        uint_t pos = index[i];
        if (pos < array_.size())
            ret[i] = array_[pos].to_hex_string();
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

std::unordered_map<std::string, uint_t> BitArray::get_counts(reg_t& index)
{
    uint_t size = std::min(array_.size(), index.size());
    std::unordered_map<std::string, uint_t> ret;

    for (uint_t i = 0; i < size; i++) {
        uint_t pos = index[i];
        if (pos < array_.size())
            ret[array_[pos].to_string()]++;
    }
    return ret;
}

void BitArray::from_json(nlohmann::ordered_json& input)
{
    auto samples = input["samples"];
    auto num_bits = input["num_bits"];
    auto num_shots = samples.size();
    if (num_bits_ == 0)
        num_bits_ = num_bits;
    allocate(num_shots, num_bits_);
    for (uint_t i = 0; i < num_shots; i++) {
        array_[i].from_hex_string(samples[i]);
    }
}

void BitArray::set_hexstring(uint_t index, std::string& input)
{
    if (index < array_.size())
        array_[index].from_hex_string(input);
}

BitArray BitArray::get_subset(const uint_t start_bit, const uint_t num_bits)
{
    BitArray ret;
    ret.allocate(array_.size(), num_bits);

    for (uint_t i = 0; i < array_.size(); i++) {
        ret.array_[i] = array_[i].get_subset(start_bit, num_bits);
    }

    return ret;
}

reg_t BitArray::bitcount(void)
{
    reg_t count(array_.size());
    for (uint_t i = 0; i < array_.size(); i++) {
        count[i] = array_[i].popcount();
    }
    return count;
}



} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_bit_array_hpp__
