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

// estimator pub definition

#ifndef __qiskitcpp_primitives_estimator_pub_def_hpp__
#define __qiskitcpp_primitives_estimator_pub_def_hpp__

#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>

#include "circuit/quantumcircuit.hpp"

namespace Qiskit {
namespace primitives {

/// @class EstimatorPub
/// @brief Estimator Pub (Primitive Unified Bloc) with an ISA circuit and Pauli observables.
class EstimatorPub {
protected:
    circuit::QuantumCircuit circuit_;
    std::vector<std::pair<std::string, double>> observables_;
    uint_t shots_ = 0;
public:
    /// @brief Create a new EstimatorPub
    EstimatorPub() {}

    /// @brief Create a new EstimatorPub
    /// @param circ an ISA QuantumCircuit
    /// @param observables Pauli-string observables as (label, coefficient) pairs
    /// @param shots The total number of shots for this estimator pub
    EstimatorPub(circuit::QuantumCircuit& circ,
                 std::vector<std::pair<std::string, double>> observables,
                 uint_t shots = 0)
    {
        circuit_ = circ;
        observables_ = observables;
        shots_ = shots;
    }

    /// @brief Create a new EstimatorPub as a copy of src.
    /// @param src an EstimatorPub
    EstimatorPub(const EstimatorPub& src)
    {
        circuit_ = src.circuit_;
        observables_ = src.observables_;
        shots_ = src.shots_;
    }

    ~EstimatorPub() {}

    /// @brief Return the QuantumCircuit for this estimator pub
    const circuit::QuantumCircuit& circuit(void) const
    {
        return circuit_;
    }

    /// @brief Return Pauli observables for this estimator pub
    const std::vector<std::pair<std::string, double>>& observables(void) const
    {
        return observables_;
    }

    /// @brief Return the total number of shots
    uint_t shots(void) const
    {
        return shots_;
    }

    /// @brief Return a JSON format of this estimator pub
    nlohmann::ordered_json to_json(void)
    {
        nlohmann::ordered_json obs = nlohmann::json::array();
        for (const auto& term : observables_) {
            obs.push_back(nlohmann::json::array({term.first, term.second}));
        }

        if (shots_ > 0) {
            return nlohmann::json::array({circuit_.to_qasm3(), obs, shots_});
        }
        return nlohmann::json::array({circuit_.to_qasm3(), obs});
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_estimator_pub_def_hpp__
