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

// QRMI estimator Runtime V2 payload helpers

#ifndef __qiskitcpp_providers_qrmi_estimator_payload_hpp__
#define __qiskitcpp_providers_qrmi_estimator_payload_hpp__

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "primitives/containers/estimator_pub.hpp"
#include "utils/types.hpp"

namespace Qiskit {
namespace providers {

/// @class QRMIEstimatorPayload
/// @brief Build Runtime V2 estimator payloads for QRMI submission.
class QRMIEstimatorPayload {
public:
    static const std::string& estimator_program_id(void)
    {
        static const std::string program_id = "estimator";
        return program_id;
    }

    static nlohmann::ordered_json build_estimator_input(std::vector<primitives::EstimatorPub>& input_pubs,
                                                        double precision)
    {
        if (!std::isfinite(precision) || precision < 0.0) {
            throw std::invalid_argument("Estimator run precision must be finite and non-negative.");
        }

        nlohmann::ordered_json estimator;
        nlohmann::ordered_json estimator_pubs = nlohmann::ordered_json::array();
        for (uint_t i = 0; i < input_pubs.size(); i++) {
            std::string message;
            if (!input_pubs[i].validate(&message)) {
                throw std::invalid_argument(message);
            }
            if (input_pubs[i].circuit().has_measurements()) {
                throw std::invalid_argument("Estimator circuits must not contain measurement operations.");
            }
            if (!input_pubs[i].has_precision()) {
                input_pubs[i].set_precision(precision);
            }
            estimator_pubs.push_back(input_pubs[i].to_json());
        }

        estimator["pubs"] = estimator_pubs;
        estimator["version"] = 2;
        estimator["support_qiskit"] = false;
        estimator["options"] = nlohmann::ordered_json::object();
        return estimator;
    }
};

} // namespace providers
} // namespace Qiskit

#endif //__qiskitcpp_providers_qrmi_estimator_payload_hpp__
