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

#include <algorithm>
#include <unordered_map>
#include <vector>
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

    /// @brief accessing raw bit array
    BitVector& operator[](const uint_t i)
    {
        return array_[i];
    }

    // from simulator samples (< 64 qubits)
    void from_samples(const reg_t& samples, uint_t num_bits)
    {
        num_bits_ = num_bits;
        array_.resize(samples.size());
        for (uint_t i = 0; i < samples.size(); i++) {
            array_[i].from_uint(samples[i], num_bits);
        }
    }

    void from_samples(const uint_t* samples, uint_t num_samples, uint_t num_bits)
    {
        num_bits_ = num_bits;
        array_.resize(num_samples);
        for (uint_t i = 0; i < num_samples; i++) {
            array_[i].from_uint(samples[i], num_bits);
        }
    }

    // from bitstring
    void from_bitstring(const std::vector<std::string>& samples)
    {
        array_.resize(samples.size());
        for (uint_t i = 0; i < samples.size(); i++) {
            array_[i].from_string(samples[i]);
        }
        num_bits_ = array_[0].size();
    }

    /// @brief Return subsets of the BitArray
    /// @param start_bit start bit index of subset
    /// @param num_bits number of bits in a subset
    /// @return A new BitArray
    BitArray get_subset(const uint_t start_bit, const uint_t num_bits)
    {
        BitArray ret;
        ret.allocate(array_.size(), num_bits);

        for (uint_t i = 0; i < array_.size(); i++) {
            ret.array_[i] = array_[i].get_subset(start_bit, num_bits);
        }

        return ret;
    }

    /// @brief Return a contiguous range of shots as a new BitArray
    /// @param start_shot index of the first shot to take
    /// @param num_shots number of shots to take
    /// @return A new BitArray holding those shots
    /// @details Used to split the shots of a pub submitted with several
    ///   parameter value sets into one BitArray per set. A range that runs past
    ///   the end is truncated.
    BitArray get_shot_range(const uint_t start_shot, const uint_t num_shots)
    {
        BitArray ret;
        ret.num_bits_ = num_bits_;

        if (start_shot >= array_.size()) {
            return ret;
        }
        const uint_t available = std::min(num_shots, (uint_t)array_.size() - start_shot);
        ret.array_.assign(array_.begin() + start_shot,
                          array_.begin() + start_shot + available);
        return ret;
    }

    /// @brief Return a list of bitstrings.
    /// @return A list of bitstrings.
    std::vector<std::string> get_bitstrings(void)
    {
        std::vector<std::string> ret(array_.size());
        for (uint_t i = 0; i < array_.size(); i++) {
            ret[i] = array_[i].to_string();
        }
        return ret;
    }

    /// @brief Return a list of bitstrings.
    /// @param index a list of index to be stored in the output list
    /// @return A list of bitstrings.
    std::vector<std::string> get_bitstrings(reg_t& index)
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

    /// @brief Return a list of hex string
    /// @return A list of hex string.
    std::vector<std::string> get_hexstrings(void)
    {
        std::vector<std::string> ret(array_.size());
        for (uint_t i = 0; i < array_.size(); i++) {
            ret[i] = array_[i].to_hex_string();
        }
        return ret;
    }

    /// @brief Return a list of hex string.
    /// @param index a list of index to be stored in the output list
    /// @return A list of hex string.
    std::vector<std::string> get_hexstrings(reg_t& index)
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

    /// @brief Return a counts dictionary with bitstring keys.
    /// @return A counts dictionary with bitstring keys.
    std::unordered_map<std::string, uint_t> get_counts(void)
    {
        std::unordered_map<std::string, uint_t> ret;
        for (uint_t i = 0; i < array_.size(); i++) {
            ret[array_[i].to_string()]++;
        }
        return ret;
    }

    /// @brief Return a counts dictionary with bitstring keys.
    /// @param index a list of index to be stored in the output map
    /// @return A counts dictionary with bitstring keys.
    std::unordered_map<std::string, uint_t> get_counts(reg_t& index)
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

    /// @brief Return the number of parameter value sets a result holds
    /// @param input JSON input for one classical register
    /// @return the number of sets
    /// @details A pub submitted with a parameter sweep returns ``samples`` as
    ///   an array of arrays, one inner array of hex samples per value set. A
    ///   pub with no sweep returns a flat array of hex samples, which counts as
    ///   a single set.
    static uint_t num_sets_in_json(const nlohmann::ordered_json& input)
    {
        if (!input.contains("samples") || !input["samples"].is_array()) {
            return 0;
        }
        const auto& samples = input["samples"];
        if (samples.empty()) {
            return 0;
        }
        return samples[0].is_array() ? samples.size() : 1;
    }

    /// @brief Set pub samples from json
    /// @param input JSON input
    /// @note For a result holding several parameter value sets, this reads the
    ///   first set. Use the two-argument form to select another.
    void from_json(nlohmann::ordered_json& input)
    {
        from_json(input, 0);
    }

    /// @brief Set pub samples from json, for one parameter value set
    /// @param input JSON input for one classical register
    /// @param set the index of the parameter value set to read
    /// @details Accepts both shapes the service returns: a flat array of hex
    ///   samples for a pub with no parameter sweep, and an array of arrays with
    ///   one entry per value set for a swept pub.
    void from_json(nlohmann::ordered_json& input, const uint_t set)
    {
        if (num_bits_ == 0 && input.contains("num_bits")) {
            num_bits_ = input["num_bits"];
        }

        if (!input.contains("samples") || !input["samples"].is_array()) {
            allocate(0, num_bits_);
            return;
        }

        auto& samples = input["samples"];
        // A flat array is a single value set; anything else is indexed by set.
        const bool nested = !samples.empty() && samples[0].is_array();
        if (!nested) {
            if (set > 0) {
                allocate(0, num_bits_);
                return;
            }
            read_hex_array(samples);
            return;
        }

        if (set >= samples.size()) {
            allocate(0, num_bits_);
            return;
        }
        read_hex_array(samples[set]);
    }

    /// @brief Set pub sample from hexstring
    /// @param index an index to be set
    /// @param input a sample in a hex string format
    void set_hexstring(uint_t index, std::string& input)
    {
        if (index < array_.size())
            array_[index].from_hex_string(input);
    }

protected:
    /// @brief Fill this array from a JSON array of hex samples, one per shot.
    /// @param samples the array of hex strings
    void read_hex_array(const nlohmann::ordered_json& samples)
    {
        allocate(samples.size(), num_bits_);
        for (uint_t i = 0; i < samples.size(); i++) {
            array_[i].from_hex_string(samples[i]);
        }
    }

public:
    /// @brief Return a list of bit counts
    /// @return A list of interger counts of bits appears in each shot
    reg_t bitcount(void)
    {
        reg_t count(array_.size());
        for (uint_t i = 0; i < array_.size(); i++) {
            count[i] = array_[i].popcount();
        }
        return count;
    }

};


} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_bit_array_hpp__
