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

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "primitives/containers/bit_array.hpp"

namespace Qiskit {
namespace primitives {

/// @class SamplerPubResult
/// @brief Result of Sampler Pub(Primitive Unified Bloc).
/// @details A pub submitted with several parameter value sets produces one set
///   of results per value set. Qiskit represents this as a leading axis on the
///   result array, "the preceding axes correspond to the shape of the pub that
///   was executed"; here each set is a separate entry, indexed with
///   ``operator[]`` as in Qiskit Python. A pub with a single value set, or none,
///   has exactly one entry, so the plain ``data()`` accessors keep working.
class SamplerPubResult {
protected:
    // One map of creg name to bitstrings per parameter value set.
    std::vector<std::unordered_map<std::string, BitArray>> data_;
    SamplerPub pub_;
public:
    /// @brief Create a new SamplerPubResult
    SamplerPubResult()
    {
        // A result always holds at least one set, so that data() is usable
        // before any result has been read.
        data_.resize(1);
    }

    /// @brief Create a new SamplerPubResult
    /// @param pub a pub for this result
    SamplerPubResult(SamplerPub& pub)
    {
        set_pub(pub);
    }

    /// @brief Create a new SamplerPubResult as a copy of src.
    /// @param src copy source.
    SamplerPubResult(const SamplerPubResult& src)
    {
        data_ = src.data_;
        pub_ = src.pub_;
    }

    /// @brief Return the number of parameter value sets in this result.
    /// @return the number of sets; one for a pub with no parameter sweep
    uint_t size(void) const
    {
        return data_.size();
    }

    /// @class DataSet
    /// @brief The result data of one parameter value set.
    /// @details Borrows the map held by the result it came from, so it must not
    ///   outlive it.
    class DataSet {
        std::unordered_map<std::string, BitArray>* data_;
        const SamplerPub* pub_;

    public:
        DataSet(std::unordered_map<std::string, BitArray>* data, const SamplerPub* pub)
            : data_(data), pub_(pub) {}

        /// @brief Result data for the first creg of the pub.
        /// @return the bitarray for the first creg
        BitArray& data(void)
        {
            return (*data_)[pub_->circuit().cregs()[0].name()];
        }

        /// @brief Result data for a creg of the pub.
        /// @param name the name of the creg
        /// @return the bitarray for that creg
        BitArray& data(const std::string& name)
        {
            return (*data_)[name];
        }

        /// @brief Result data for a creg of the pub.
        /// @param creg the creg to be returned
        /// @return the bitarray for that creg
        BitArray& data(const circuit::ClassicalRegister& creg)
        {
            return (*data_)[creg.name()];
        }

        /// @brief Result data for the first creg of the pub.
        /// @return the bitarray for the first creg
        BitArray& operator[](const std::string& name)
        {
            return (*data_)[name];
        }
    };

    /// @brief Return the result data of one parameter value set.
    /// @param index the index of the value set
    /// @return the data of that set
    /// @throws std::out_of_range if the index is past the end
    /// @note Mirrors indexing a Qiskit ``SamplerPubResult``, where the index
    ///   selects an entry of the pub's parameter sweep.
    DataSet operator[](const uint_t index)
    {
        if (index >= data_.size()) {
            std::stringstream message;
            message << "SamplerPubResult has " << data_.size()
                    << " parameter value sets, cannot access set " << index;
            throw std::out_of_range(message.str());
        }
        return DataSet(&data_[index], &pub_);
    }

    /// @brief Result data for the pub.
    /// @return the bitarray for the first creg of the first value set
    /// @note For a pub with a parameter sweep, this reads the first value set
    ///   only; use ``operator[]`` to reach the others.
    BitArray& data(void)
    {
        return data_[0][pub_.circuit().cregs()[0].name()];
    }

    /// @brief Result data for the pub.
    /// @param name the name of the creg
    /// @return the bitarray for the creg name in the first value set
    BitArray& data(const std::string& name)
    {
        return data_[0][name];
    }

    /// @brief Result data for the pub.
    /// @param creg the creg to be returned
    /// @return the bitarray for the creg in the first value set
    BitArray& data(const circuit::ClassicalRegister& creg)
    {
        return data_[0][creg.name()];
    }

    /// @brief Result data for the pub, for one parameter value set.
    /// @param name the name of the creg
    /// @param set the index of the parameter value set
    /// @return the bitarray for that creg in that set
    /// @throws std::out_of_range if the set index is past the end
    BitArray& data(const std::string& name, const uint_t set)
    {
        return (*this)[set].data(name);
    }

    /// @brief Return the counts of a creg, unioned over every value set.
    /// @param name the name of the creg
    /// @return a counts dictionary with bitstring keys
    /// @note Matches Qiskit's ``get_counts()`` with no location, which unions
    ///   the counts from every position of the result array.
    std::unordered_map<std::string, uint_t> get_counts(const std::string& name)
    {
        std::unordered_map<std::string, uint_t> ret;
        for (uint_t i = 0; i < data_.size(); i++) {
            auto entry = data_[i].find(name);
            if (entry == data_[i].end()) {
                continue;
            }
            const auto counts = entry->second.get_counts();
            for (auto count : counts) {
                ret[count.first] += count.second;
            }
        }
        return ret;
    }

    /// @brief get pub for this result
    /// @return pub
    const SamplerPub& pub(void) const
    {
        return pub_;
    }

    /// @brief set pub for this result
    /// @param pub to be set
    /// @details Sizes the result to the pub's parameter sweep, so that a pub
    ///   submitted with several value sets has one entry per set.
    void set_pub(const SamplerPub& pub)
    {
        pub_ = pub;

        const uint_t num_sets = pub_.num_parameter_sets();
        data_.clear();
        data_.resize(num_sets > 0 ? num_sets : 1);

        for (uint_t i = 0; i < data_.size(); i++) {
            for (auto creg : pub_.circuit().cregs()) {
                data_[i][creg.name()] = BitArray();
                data_[i][creg.name()].set_bits(creg.size());
            }
        }
    }

    /// @brief Set pub reuslt from json
    /// @param input Runtime result JSON for one pub
    /// @return true if parsing succeeded
    bool from_json(nlohmann::ordered_json& input)
    {
        if (!input.contains("data")) {
            std::cerr << " SamplerPubResult Error : JSON result does not contain data section " << std::endl;
            return false;
        }

        auto data = input["data"];

        for(auto creg : pub_.circuit().cregs()) {
          if(!data.contains(creg.name())) {
            std::cerr << " SamplerPubResult Error : JSON result does not contain "
                         "creg section for "
                      << creg.name()
                      << std::endl;
            return false;
          }
        }

        // The service reports the number of parameter value sets in the shape
        // of the samples array: a swept pub returns one inner array of samples
        // per value set, an unswept pub returns a flat array. The result is
        // sized from what actually arrived rather than from the pub, so that a
        // sweep is handled even when the pub was not built locally.
        uint_t num_sets = 0;
        for(auto creg : pub_.circuit().cregs()) {
          const uint_t creg_sets = BitArray::num_sets_in_json(data[creg.name()]);
          if (num_sets == 0) {
            num_sets = creg_sets;
          } else if (creg_sets != num_sets) {
            std::cerr << " SamplerPubResult Error : creg " << creg.name()
                      << " holds " << creg_sets
                      << " parameter value sets but another creg holds "
                      << num_sets << std::endl;
            return false;
          }
        }
        if (num_sets == 0) {
          num_sets = 1;
        }

        // A mismatch against the submitted sweep means the result does not
        // belong to this pub, which is worth reporting rather than silently
        // returning the wrong number of sets.
        const uint_t submitted = pub_.num_parameter_sets();
        if (submitted > 0 && submitted != num_sets) {
          std::cerr << " SamplerPubResult Error : the result holds " << num_sets
                    << " parameter value sets but the pub submitted " << submitted
                    << std::endl;
          return false;
        }

        std::vector<std::unordered_map<std::string, BitArray>> parsed(num_sets);
        for(auto creg : pub_.circuit().cregs()) {
          for (uint_t i = 0; i < num_sets; i++) {
            BitArray bits;
            bits.set_bits(creg.size());
            bits.from_json(data[creg.name()], i);
            parsed[i][creg.name()] = bits;
          }
        }

        data_ = parsed;
        return true;
    }

    /// @brief allocate bit array data
    /// @param num_samples number of samples to be allocated for each value set
    void allocate(uint_t num_samples)
    {
        if (data_.empty()) {
            data_.resize(1);
        }
        for (uint_t i = 0; i < data_.size(); i++) {
            for (auto creg : pub_.circuit().cregs()) {
                data_[i][creg.name()] = BitArray();
                data_[i][creg.name()].allocate(num_samples, creg.size());
            }
        }
    }

    /// @brief add bitstring by hexstring
    /// @param i the index of the sample within the value set
    /// @param str hexstring to be added in data
    /// @param set the index of the parameter value set
    void set_hexstring(const uint_t i, const std::string& str, const uint_t set = 0)
    {
        if (set >= data_.size()) {
            return;
        }

        BitVector bits;
        bits.from_hex_string(str);

        uint_t pos = 0;
        // split bitstring and store for each creg
        for (auto creg : pub_.circuit().cregs()) {
            data_[set][creg.name()][i] = bits.get_subset(pos, creg.size());
            pos += creg.size();
        }
    }
};

} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_sampler_pub_result_hpp__
