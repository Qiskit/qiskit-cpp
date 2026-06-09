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

#include <cmath>
#include <complex>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "circuit/quantumcircuit.hpp"
#include "quantum_info/sparse_observable.hpp"
#include "utils/types.hpp"

namespace Qiskit {
namespace primitives {

/// @class EstimatorPub
/// @brief Estimator Pub (Primitive Unified Bloc) with an ISA circuit and observables.
class EstimatorPub {
protected:
    circuit::QuantumCircuit circuit_;
    nlohmann::ordered_json observables_;
    nlohmann::ordered_json parameter_values_;
    double precision_ = 0.0;
    bool has_precision_ = false;

public:
    EstimatorPub()
    {
        observables_ = nlohmann::ordered_json::object();
        parameter_values_ = nullptr;
    }

    EstimatorPub(const circuit::QuantumCircuit& circ, const std::string& observable)
    {
        circuit_ = circ;
        observables_ = observable;
        parameter_values_ = nullptr;
        require_valid();
    }

    EstimatorPub(const circuit::QuantumCircuit& circ, const std::string& observable, double precision)
        : EstimatorPub(circ, observable)
    {
        set_precision(precision);
    }

    EstimatorPub(const circuit::QuantumCircuit& circ, const nlohmann::ordered_json& observables)
    {
        circuit_ = circ;
        observables_ = observables;
        parameter_values_ = nullptr;
        require_valid();
    }

    EstimatorPub(const circuit::QuantumCircuit& circ, const nlohmann::ordered_json& observables, double precision)
        : EstimatorPub(circ, observables)
    {
        set_precision(precision);
    }

    EstimatorPub(const circuit::QuantumCircuit& circ, const std::vector<std::pair<std::string, double>>& observable)
    {
        circuit_ = circ;
        observables_ = pauli_terms_to_json(observable);
        parameter_values_ = nullptr;
        require_valid();
    }

    EstimatorPub(const circuit::QuantumCircuit& circ, const std::vector<std::pair<std::string, double>>& observable, double precision)
        : EstimatorPub(circ, observable)
    {
        set_precision(precision);
    }

    EstimatorPub(const circuit::QuantumCircuit& circ, const quantum_info::SparseObservable& observable)
    {
        circuit_ = circ;
        observables_ = sparse_observable_to_json(observable);
        parameter_values_ = nullptr;
        require_valid();
    }

    EstimatorPub(const circuit::QuantumCircuit& circ, const quantum_info::SparseObservable& observable, double precision)
        : EstimatorPub(circ, observable)
    {
        set_precision(precision);
    }

    EstimatorPub(const circuit::QuantumCircuit& circ, const std::vector<quantum_info::SparseObservable>& observables)
    {
        circuit_ = circ;
        observables_ = nlohmann::ordered_json::array();
        for (uint_t i = 0; i < observables.size(); i++) {
            observables_.push_back(sparse_observable_to_json(observables[i]));
        }
        parameter_values_ = nullptr;
        require_valid();
    }

    EstimatorPub(const circuit::QuantumCircuit& circ, const std::vector<quantum_info::SparseObservable>& observables, double precision)
        : EstimatorPub(circ, observables)
    {
        set_precision(precision);
    }

    EstimatorPub(const EstimatorPub& src)
    {
        circuit_ = src.circuit_;
        observables_ = src.observables_;
        parameter_values_ = src.parameter_values_;
        precision_ = src.precision_;
        has_precision_ = src.has_precision_;
    }

    ~EstimatorPub() {}

    const circuit::QuantumCircuit& circuit(void) const
    {
        return circuit_;
    }

    const nlohmann::ordered_json& observables(void) const
    {
        return observables_;
    }

    const nlohmann::ordered_json& parameter_values(void) const
    {
        return parameter_values_;
    }

    void set_parameter_values(const nlohmann::ordered_json& parameter_values)
    {
        parameter_values_ = parameter_values;
    }

    bool has_precision(void) const
    {
        return has_precision_;
    }

    double precision(void) const
    {
        return precision_;
    }

    void set_precision(double precision)
    {
        if (!valid_precision(precision)) {
            throw std::invalid_argument("EstimatorPub precision must be finite and non-negative.");
        }
        precision_ = precision;
        has_precision_ = true;
    }

    uint_t num_observables(void) const
    {
        if (observables_.is_array()) {
            return observables_.size();
        }
        return 1;
    }

    bool validate(std::string* message = nullptr) const
    {
        if (has_precision_ && !valid_precision(precision_)) {
            set_message(message, "EstimatorPub precision must be finite and non-negative.");
            return false;
        }
        return validate_observable_json(observables_, circuit_.num_qubits(), message);
    }

    nlohmann::ordered_json to_json(void)
    {
        require_valid();
        nlohmann::ordered_json ret = nlohmann::ordered_json::array();
        ret.push_back(circuit_.to_qasm3());
        ret.push_back(observables_);
        ret.push_back(parameter_values_);
        if (has_precision_) {
            ret.push_back(precision_);
        }
        return ret;
    }

protected:
    void require_valid(void) const
    {
        std::string message;
        if (!validate(&message)) {
            throw std::invalid_argument(message);
        }
    }

    static bool valid_precision(double precision)
    {
        return std::isfinite(precision) && precision >= 0.0;
    }

    static void set_message(std::string* message, const std::string& value)
    {
        if (message) {
            *message = value;
        }
    }

    static bool valid_pauli_label(const std::string& label, uint_t width, std::string* message)
    {
        if (label.size() != width) {
            set_message(message, "Estimator observable width must match the circuit width.");
            return false;
        }
        for (uint_t i = 0; i < label.size(); i++) {
            const char ch = label[i];
            if (ch != 'I' && ch != 'X' && ch != 'Y' && ch != 'Z') {
                set_message(message, "Estimator observables only support Pauli labels containing I, X, Y, or Z.");
                return false;
            }
        }
        return true;
    }

    static bool validate_observable_json(const nlohmann::ordered_json& observable, uint_t width, std::string* message)
    {
        if (observable.is_string()) {
            return valid_pauli_label(observable.get<std::string>(), width, message);
        }
        if (observable.is_object()) {
            if (observable.empty()) {
                set_message(message, "EstimatorPub must contain at least one observable.");
                return false;
            }
            for (auto it = observable.begin(); it != observable.end(); ++it) {
                if (!valid_pauli_label(it.key(), width, message)) {
                    return false;
                }
                if (!it.value().is_number()) {
                    set_message(message, "Estimator observable coefficients must be real numbers.");
                    return false;
                }
                const double coeff = it.value().get<double>();
                if (!std::isfinite(coeff)) {
                    set_message(message, "Estimator observable coefficients must be finite.");
                    return false;
                }
            }
            return true;
        }
        if (observable.is_array()) {
            if (observable.empty()) {
                set_message(message, "EstimatorPub must contain at least one observable.");
                return false;
            }
            for (uint_t i = 0; i < observable.size(); i++) {
                if (observable[i].is_array()) {
                    set_message(message, "Estimator observable arrays must not be nested.");
                    return false;
                }
                if (!validate_observable_json(observable[i], width, message)) {
                    return false;
                }
            }
            return true;
        }
        set_message(message, "Estimator observables must be a Pauli label, Pauli map, or array of observables.");
        return false;
    }

    static nlohmann::ordered_json pauli_terms_to_json(const std::vector<std::pair<std::string, double>>& observable)
    {
        if (observable.empty()) {
            throw std::invalid_argument("EstimatorPub must contain at least one observable.");
        }
        nlohmann::ordered_json ret = nlohmann::ordered_json::object();
        for (uint_t i = 0; i < observable.size(); i++) {
            const double coeff = observable[i].second;
            if (!std::isfinite(coeff)) {
                throw std::invalid_argument("Estimator observable coefficients must be finite.");
            }
            ret[observable[i].first] = coeff;
        }
        return ret;
    }

    static nlohmann::ordered_json sparse_observable_to_json(const quantum_info::SparseObservable& observable)
    {
        nlohmann::ordered_json ret = nlohmann::ordered_json::object();
        const uint_t num_qubits = observable.num_qubits();
        const uint_t num_terms = observable.num_terms();
        const auto coeffs = observable.coeffs();
        const auto bit_terms = observable.bit_terms();
        const auto indices = observable.indices();
        const auto boundaries = observable.boundaries();

        if (num_terms == 0) {
            throw std::invalid_argument("EstimatorPub must contain at least one observable.");
        }
        if (coeffs.size() != num_terms || boundaries.size() != num_terms + 1) {
            throw std::invalid_argument("SparseObservable has inconsistent term metadata.");
        }

        for (uint_t term = 0; term < num_terms; term++) {
            if (boundaries[term] > boundaries[term + 1] ||
                boundaries[term + 1] > bit_terms.size() ||
                boundaries[term + 1] > indices.size()) {
                throw std::invalid_argument("SparseObservable has inconsistent term boundaries.");
            }

            const auto coeff = coeffs[term];
            if (!std::isfinite(coeff.real()) || !std::isfinite(coeff.imag())) {
                throw std::invalid_argument("Estimator observable coefficients must be finite.");
            }
            if (std::abs(coeff.imag()) > 1e-12) {
                throw std::invalid_argument("Estimator observable coefficients must be real.");
            }

            std::string label(num_qubits, 'I');
            for (uint_t i = boundaries[term]; i < boundaries[term + 1]; i++) {
                if (indices[i] >= num_qubits) {
                    throw std::invalid_argument("SparseObservable term index is outside the observable width.");
                }
                char pauli = 'I';
                switch (bit_terms[i]) {
                case QkBitTerm_X:
                    pauli = 'X';
                    break;
                case QkBitTerm_Y:
                    pauli = 'Y';
                    break;
                case QkBitTerm_Z:
                    pauli = 'Z';
                    break;
                default:
                    throw std::invalid_argument("Estimator SparseObservable terms must be Pauli I, X, Y, or Z.");
                }
                label[num_qubits - 1 - indices[i]] = pauli;
            }

            if (ret.contains(label)) {
                ret[label] = ret[label].get<double>() + coeff.real();
            } else {
                ret[label] = coeff.real();
            }
        }
        return ret;
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_estimator_pub_def_hpp__
