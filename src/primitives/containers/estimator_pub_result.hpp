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

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "primitives/containers/estimator_pub.hpp"
#include "utils/types.hpp"

namespace Qiskit {
namespace primitives {

/// @class EstimatorPubResult
/// @brief Result of Estimator Pub (Primitive Unified Bloc).
class EstimatorPubResult {
protected:
    std::vector<double> evs_;
    std::vector<double> stds_;
    nlohmann::ordered_json metadata_;
    EstimatorPub pub_;
    bool has_pub_ = false;

public:
    EstimatorPubResult()
    {
        metadata_ = nlohmann::ordered_json::object();
    }

    EstimatorPubResult(const EstimatorPub& pub)
    {
        pub_ = pub;
        has_pub_ = true;
        metadata_ = nlohmann::ordered_json::object();
    }

    EstimatorPubResult(const EstimatorPubResult& src)
    {
        evs_ = src.evs_;
        stds_ = src.stds_;
        metadata_ = src.metadata_;
        pub_ = src.pub_;
        has_pub_ = src.has_pub_;
    }

    const std::vector<double>& evs(void) const
    {
        return evs_;
    }

    std::vector<double>& evs(void)
    {
        return evs_;
    }

    const std::vector<double>& stds(void) const
    {
        return stds_;
    }

    std::vector<double>& stds(void)
    {
        return stds_;
    }

    const nlohmann::ordered_json& metadata(void) const
    {
        return metadata_;
    }

    nlohmann::ordered_json& metadata(void)
    {
        return metadata_;
    }

    const EstimatorPub& pub(void) const
    {
        return pub_;
    }

    void set_pub(const EstimatorPub& pub)
    {
        pub_ = pub;
        has_pub_ = true;
    }

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

        std::vector<double> evs;
        if (!parse_numeric_scalar_or_1d(data["evs"], evs, "evs")) {
            return false;
        }
        if (has_pub_ && evs.size() != pub_.num_observables()) {
            std::cerr << " EstimatorPubResult Error : evs length does not match the submitted observable count." << std::endl;
            return false;
        }

        std::vector<double> stds;
        if (data.contains("stds")) {
            if (!parse_numeric_scalar_or_1d(data["stds"], stds, "stds")) {
                return false;
            }
        } else if (data.contains("ensemble_standard_error")) {
            if (!parse_numeric_scalar_or_1d(data["ensemble_standard_error"], stds, "ensemble_standard_error")) {
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
        metadata_ = metadata;
        return true;
    }

protected:
    static bool parse_numeric_scalar_or_1d(const nlohmann::ordered_json& value,
                                           std::vector<double>& output,
                                           const std::string& name)
    {
        output.clear();
        if (value.is_number()) {
            const double number = value.get<double>();
            if (!std::isfinite(number)) {
                std::cerr << " EstimatorPubResult Error : " << name << " contains a non-finite value." << std::endl;
                return false;
            }
            output.push_back(number);
            return true;
        }
        if (!value.is_array()) {
            std::cerr << " EstimatorPubResult Error : " << name << " must be a number or one-dimensional array." << std::endl;
            return false;
        }
        for (uint_t i = 0; i < value.size(); i++) {
            if (!value[i].is_number()) {
                std::cerr << " EstimatorPubResult Error : " << name << " must be a one-dimensional numeric array." << std::endl;
                return false;
            }
            const double number = value[i].get<double>();
            if (!std::isfinite(number)) {
                std::cerr << " EstimatorPubResult Error : " << name << " contains a non-finite value." << std::endl;
                return false;
            }
            output.push_back(number);
        }
        return true;
    }

    static bool validate_metadata(const nlohmann::ordered_json& metadata)
    {
        if (metadata.contains("shots")) {
            if (!metadata["shots"].is_number_integer() && !metadata["shots"].is_number_unsigned()) {
                std::cerr << " EstimatorPubResult Error : metadata.shots must be an integer." << std::endl;
                return false;
            }
            if (metadata["shots"].is_number_integer() && metadata["shots"].get<int64_t>() < 0) {
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
