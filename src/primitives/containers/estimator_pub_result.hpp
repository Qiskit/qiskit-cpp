/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2017, 2026.
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

#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "primitives/containers/estimator_pub.hpp"

namespace Qiskit {
namespace primitives {

/// @class EstimatorPubResult
/// @brief Result of Estimator Pub(Primitive Unified Bloc).
/// @details A pub submitted with several parameter value sets produces one
///   expectation value per value set and observable. The values are stored flat
///   in row-major order, so entry ``set * num_observables + observable``, and
///   ``operator[]`` returns the values of one set as in Qiskit Python.
class EstimatorPubResult {
protected:
    // Expectation values, flattened over (value set, observable).
    std::vector<double> evs_;
    std::vector<double> stds_;
    // The number of parameter value sets the values are split across.
    uint_t num_sets_ = 1;
    nlohmann::ordered_json metadata_;
    EstimatorPub pub_;
    bool has_pub_ = false;

public:
    /// @brief Create a new EstimatorPubResult.
    EstimatorPubResult()
    {
        metadata_ = nlohmann::ordered_json::object();
    }

    /// @brief Create a new EstimatorPubResult.
    /// @param pub a pub for this result.
    EstimatorPubResult(const EstimatorPub& pub)
    {
        pub_ = pub;
        has_pub_ = true;
        metadata_ = nlohmann::ordered_json::object();
    }

    /// @brief Create a new EstimatorPubResult as a copy of src.
    /// @param src copy source.
    EstimatorPubResult(const EstimatorPubResult& src)
    {
        evs_ = src.evs_;
        stds_ = src.stds_;
        num_sets_ = src.num_sets_;
        metadata_ = src.metadata_;
        pub_ = src.pub_;
        has_pub_ = src.has_pub_;
    }

    /// @brief Result expectation values for this pub.
    /// @return expectation values.
    const std::vector<double>& evs(void) const
    {
        return evs_;
    }

    /// @brief Result expectation values for this pub.
    /// @return expectation values.
    std::vector<double>& evs(void)
    {
        return evs_;
    }

    /// @brief Result standard deviations for this pub.
    /// @return standard deviations.
    const std::vector<double>& stds(void) const
    {
        return stds_;
    }

    /// @brief Result standard deviations for this pub.
    /// @return standard deviations.
    std::vector<double>& stds(void)
    {
        return stds_;
    }

    /// @brief Return the number of parameter value sets in this result.
    /// @return the number of sets; one for a pub with no parameter sweep
    uint_t size(void) const
    {
        return num_sets_;
    }

    /// @brief Return the number of expectation values per parameter value set.
    /// @return the number of observables each value set was evaluated against
    uint_t num_observables(void) const
    {
        if (num_sets_ == 0) {
            return 0;
        }
        return evs_.size() / num_sets_;
    }

    /// @class DataSet
    /// @brief The expectation values and standard deviations of one value set.
    /// @details Holds copies, so it stays valid independently of the result it
    ///   came from.
    class DataSet {
        std::vector<double> evs_;
        std::vector<double> stds_;

    public:
        DataSet(const std::vector<double>& evs, const std::vector<double>& stds)
            : evs_(evs), stds_(stds) {}

        /// @brief Expectation values of this value set, one per observable.
        const std::vector<double>& evs(void) const { return evs_; }

        /// @brief Standard deviations of this value set, one per observable.
        /// @return the deviations, or an empty list if the result has none
        const std::vector<double>& stds(void) const { return stds_; }

        /// @brief Expectation value for one observable of this value set.
        /// @param observable the index of the observable
        /// @throws std::out_of_range if the index is past the end
        double operator[](const uint_t observable) const
        {
            if (observable >= evs_.size()) {
                std::stringstream message;
                message << "EstimatorPubResult value set holds " << evs_.size()
                        << " expectation values, cannot access " << observable;
                throw std::out_of_range(message.str());
            }
            return evs_[observable];
        }
    };

    /// @brief Return the results of one parameter value set.
    /// @param index the index of the value set
    /// @return the expectation values and deviations of that set
    /// @throws std::out_of_range if the index is past the end
    /// @note Mirrors indexing a Qiskit estimator result, where the index
    ///   selects an entry of the pub's parameter sweep.
    DataSet operator[](const uint_t index) const
    {
        if (index >= num_sets_) {
            std::stringstream message;
            message << "EstimatorPubResult has " << num_sets_
                    << " parameter value sets, cannot access set " << index;
            throw std::out_of_range(message.str());
        }

        const uint_t width = num_observables();
        const uint_t begin = index * width;

        std::vector<double> evs(evs_.begin() + begin, evs_.begin() + begin + width);
        std::vector<double> stds;
        if (stds_.size() == evs_.size()) {
            stds.assign(stds_.begin() + begin, stds_.begin() + begin + width);
        }
        return DataSet(evs, stds);
    }

    /// @brief Expectation value for one observable of one value set.
    /// @param set the index of the parameter value set
    /// @param observable the index of the observable
    /// @return the expectation value
    /// @throws std::out_of_range if either index is past the end
    double ev(const uint_t set, const uint_t observable) const
    {
        return (*this)[set][observable];
    }

    /// @brief Result metadata for this pub.
    /// @return metadata.
    const nlohmann::ordered_json& metadata(void) const
    {
        return metadata_;
    }

    /// @brief Result metadata for this pub.
    /// @return metadata.
    nlohmann::ordered_json& metadata(void)
    {
        return metadata_;
    }

    /// @brief get pub for this result.
    /// @return pub.
    const EstimatorPub& pub(void) const
    {
        return pub_;
    }

    /// @brief set pub for this result.
    /// @param pub to be set.
    void set_pub(const EstimatorPub& pub)
    {
        pub_ = pub;
        has_pub_ = true;
        // Sized from the pub's sweep so that size() is right before any result
        // has been read; from_json confirms it against the returned values.
        num_sets_ = pub_.num_parameter_sets() > 0 ? pub_.num_parameter_sets() : 1;
    }

    /// @brief Set pub result from json.
    /// @param input Runtime result JSON.
    /// @return true if parsing succeeded.
    bool from_json(nlohmann::ordered_json& input)
    {
        if (!input.is_object()) {
            std::cerr << " EstimatorPubResult Error : JSON result must be an object." << std::endl;
            return false;
        }

        if (!input.contains("data") || !input["data"].is_object()) {
            std::cerr << " EstimatorPubResult Error : JSON result does not contain data section." << std::endl;
            return false;
        }

        auto data = input["data"];
        if (!data.contains("evs")) {
            std::cerr << " EstimatorPubResult Error : JSON data does not contain evs." << std::endl;
            return false;
        }

        // A pub submitted with several parameter value sets returns one value
        // per set and observable. Only such a pub may report its values nested,
        // so the sweep has to be known before parsing.
        const uint_t submitted_sets = has_pub_ && pub_.num_parameter_sets() > 0
            ? pub_.num_parameter_sets() : 1;
        const bool nesting_allowed = submitted_sets > 1;

        std::vector<double> evs;
        if (!parse_numeric_scalar_or_1d(data["evs"], evs, "evs", nesting_allowed)) {
            return false;
        }

        uint_t num_sets = 1;
        if (has_pub_) {
            const uint_t num_observables = pub_.num_observables();
            const uint_t expected = num_observables * submitted_sets;

            if (evs.size() != expected) {
                std::cerr << " EstimatorPubResult Error : evs holds " << evs.size()
                    << " values but the pub submitted " << submitted_sets
                    << " parameter value set(s) against " << num_observables
                    << " observable(s), expecting " << expected << "." << std::endl;
                return false;
            }
            num_sets = submitted_sets;
        }

        std::vector<double> stds;
        if (data.contains("stds")) {
            if (!parse_numeric_scalar_or_1d(data["stds"], stds, "stds", nesting_allowed)) {
                return false;
            }
        } else if (data.contains("ensemble_standard_error")) {
            if (!parse_numeric_scalar_or_1d(data["ensemble_standard_error"], stds, "ensemble_standard_error", nesting_allowed)) {
                return false;
            }
        }

        if (!stds.empty() && stds.size() != evs.size()) {
            std::cerr << " EstimatorPubResult Error : stds length does not match evs length." << std::endl;
            return false;
        }

        nlohmann::ordered_json metadata = nlohmann::ordered_json::object();
        if (input.contains("metadata")) {
            if (!input["metadata"].is_object()) {
                std::cerr << " EstimatorPubResult Error : metadata must be an object." << std::endl;
                return false;
            }
            metadata = input["metadata"];
            if (!validate_metadata(metadata)) {
                return false;
            }
        }

        evs_ = evs;
        stds_ = stds;
        num_sets_ = num_sets;
        metadata_ = metadata;
        return true;
    }

protected:
    /// @brief Read a scalar, a 1-D array, or a nested array of numbers.
    /// @param value the JSON value to read
    /// @param output receives the numbers, flattened row-major
    /// @param name the field name, for diagnostics
    /// @param nesting_allowed whether the value may be an array of arrays
    /// @return true if parsing succeeded
    /// @details A pub submitted with a parameter sweep over more than one
    ///   observable reports one inner array per value set, which is flattened
    ///   in (value set, observable) order to match how the result stores it.
    ///   Nesting is only accepted when the pub actually swept: for a pub with a
    ///   single value set it is malformed, and accepting it would mask a result
    ///   that does not belong to the pub. Every inner array must be the same
    ///   length, since the values form a rectangular array.
    static bool parse_numeric_scalar_or_1d(const nlohmann::ordered_json& value, std::vector<double>& output, const std::string& name, const bool nesting_allowed)
    {
        output.clear();

        if (value.is_number()) {
            return append_number(value, output, name);
        }

        if (!value.is_array()) {
            std::cerr << " EstimatorPubResult Error : " << name << " must be a number or an array of numbers." << std::endl;
            return false;
        }

        // Nesting is decided by the first element: the value is either a flat
        // array of numbers or an array of equal-length inner arrays.
        const bool nested = !value.empty() && value[0].is_array();
        if (!nested) {
            for (uint_t i = 0; i < value.size(); i++) {
                if (!append_number(value[i], output, name)) {
                    return false;
                }
            }
            return true;
        }

        if (!nesting_allowed) {
            std::cerr << " EstimatorPubResult Error : " << name
                << " must be a number or one-dimensional array for a pub with a single"
                   " parameter value set." << std::endl;
            return false;
        }

        const uint_t width = value[0].size();
        for (uint_t i = 0; i < value.size(); i++) {
            if (!value[i].is_array()) {
                std::cerr << " EstimatorPubResult Error : " << name
                    << " mixes nested and plain entries." << std::endl;
                return false;
            }
            if (value[i].size() != width) {
                std::cerr << " EstimatorPubResult Error : " << name
                    << " entry " << i << " holds " << value[i].size()
                    << " values but entry 0 holds " << width << "." << std::endl;
                return false;
            }
            for (uint_t j = 0; j < width; j++) {
                if (!append_number(value[i][j], output, name)) {
                    return false;
                }
            }
        }

        return true;
    }

    /// @brief Append one finite number from JSON.
    /// @param value the JSON value, which must be a number
    /// @param output receives the number
    /// @param name the field name, for diagnostics
    /// @return true if the value is a finite number
    static bool append_number(const nlohmann::ordered_json& value, std::vector<double>& output, const std::string& name)
    {
        if (!value.is_number()) {
            std::cerr << " EstimatorPubResult Error : " << name << " must contain only numbers." << std::endl;
            return false;
        }
        const double number = value.get<double>();
        if (!std::isfinite(number)) {
            std::cerr << " EstimatorPubResult Error : " << name << " contains a non-finite value." << std::endl;
            return false;
        }
        output.push_back(number);
        return true;
    }

    static bool validate_metadata(const nlohmann::ordered_json& metadata)
    {
        if (metadata.contains("shots")) {
            if (!metadata["shots"].is_number_integer() && !metadata["shots"].is_number_unsigned()) {
                std::cerr << " EstimatorPubResult Error : metadata.shots must be an integer." << std::endl;
                return false;
            }
            if (metadata["shots"].is_number_integer() && metadata["shots"].get<int_t>() < 0) {
                std::cerr << " EstimatorPubResult Error : metadata.shots must be non-negative." << std::endl;
                return false;
            }
        }

        if (metadata.contains("target_precision")) {
            if (!metadata["target_precision"].is_number()) {
                std::cerr << " EstimatorPubResult Error : metadata.target_precision must be numeric." << std::endl;
                return false;
            }
            const double target_precision = metadata["target_precision"].get<double>();
            if (!std::isfinite(target_precision) || target_precision < 0.0) {
                std::cerr << " EstimatorPubResult Error : metadata.target_precision must be finite and non-negative." << std::endl;
                return false;
            }
        }

        return true;
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_estimator_pub_result_hpp__
