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

// estimator pub definition

#ifndef __qiskitcpp_primitives_estimator_pub_def_hpp__
#define __qiskitcpp_primitives_estimator_pub_def_hpp__

#include <cmath>
#include <complex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "circuit/quantumcircuit.hpp"
#include "primitives/containers/circuit_serialization.hpp"
#include "primitives/containers/parameter_values.hpp"
#include "quantum_info/sparse_observable.hpp"

namespace Qiskit {
namespace primitives {

/// @class EstimatorPub
/// @brief Estimator Pub(Primitive Unified Bloc).
class EstimatorPub {
protected:
    circuit::QuantumCircuit circuit_;
    nlohmann::ordered_json observables_;
    nlohmann::ordered_json parameter_values_;
    double precision_ = 0.0;
    bool has_precision_ = false;
    uint_t num_parameter_sets_ = 0;
    CircuitSerialization serialization_ = default_circuit_serialization();

public:
    /// @brief Create a new EstimatorPub.
    EstimatorPub()
    {
        observables_ = nlohmann::ordered_json::object();
        parameter_values_ = nullptr;
    }

    /// @brief Create a new EstimatorPub from a Pauli label with unit coefficient.
    EstimatorPub(const circuit::QuantumCircuit& circ, const std::string& observable)
    {
        circuit_ = circ;
        observables_ = observable;
        parameter_values_ = nullptr;
        require_valid();
    }

    /// @brief Create a new EstimatorPub from a Pauli label with unit coefficient.
    EstimatorPub(const circuit::QuantumCircuit& circ, const std::string& observable, double precision)
        : EstimatorPub(circ, observable)
    {
        set_precision(precision);
    }

    /// @brief Create a new EstimatorPub from a Runtime-compatible observables JSON value.
    EstimatorPub(const circuit::QuantumCircuit& circ, const nlohmann::ordered_json& observables)
    {
        circuit_ = circ;
        observables_ = observables;
        parameter_values_ = nullptr;
        require_valid();
    }

    /// @brief Create a new EstimatorPub from a Runtime-compatible observables JSON value.
    EstimatorPub(const circuit::QuantumCircuit& circ, const nlohmann::ordered_json& observables, double precision)
        : EstimatorPub(circ, observables)
    {
        set_precision(precision);
    }

    /// @brief Create a new EstimatorPub from real-coefficient Pauli label terms.
    EstimatorPub(const circuit::QuantumCircuit& circ, const std::vector<std::pair<std::string, double>>& observable)
    {
        circuit_ = circ;
        observables_ = pauli_terms_to_json(observable);
        parameter_values_ = nullptr;
        require_valid();
    }

    /// @brief Create a new EstimatorPub from real-coefficient Pauli label terms.
    EstimatorPub(const circuit::QuantumCircuit& circ, const std::vector<std::pair<std::string, double>>& observable, double precision)
        : EstimatorPub(circ, observable)
    {
        set_precision(precision);
    }

    /// @brief Create a new EstimatorPub from complex-coefficient Pauli label terms.
    EstimatorPub(const circuit::QuantumCircuit& circ, const std::vector<std::pair<std::string, std::complex<double>>>& observable)
    {
        circuit_ = circ;
        observables_ = pauli_terms_to_json(observable);
        parameter_values_ = nullptr;
        require_valid();
    }

    /// @brief Create a new EstimatorPub from complex-coefficient Pauli label terms.
    EstimatorPub(const circuit::QuantumCircuit& circ, const std::vector<std::pair<std::string, std::complex<double>>>& observable, double precision)
        : EstimatorPub(circ, observable)
    {
        set_precision(precision);
    }

    /// @brief Create a new EstimatorPub from a SparseObservable.
    EstimatorPub(const circuit::QuantumCircuit& circ, const quantum_info::SparseObservable& observable)
    {
        circuit_ = circ;
        observables_ = sparse_observable_to_json(observable);
        parameter_values_ = nullptr;
        require_valid();
    }

    /// @brief Create a new EstimatorPub from a SparseObservable.
    EstimatorPub(const circuit::QuantumCircuit& circ, const quantum_info::SparseObservable& observable, double precision)
        : EstimatorPub(circ, observable)
    {
        set_precision(precision);
    }

    /// @brief Create a new EstimatorPub from a list of SparseObservable objects.
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

    /// @brief Create a new EstimatorPub from a list of SparseObservable objects.
    EstimatorPub(const circuit::QuantumCircuit& circ, const std::vector<quantum_info::SparseObservable>& observables, double precision)
        : EstimatorPub(circ, observables)
    {
        set_precision(precision);
    }

    /// @brief Create a new EstimatorPub as a copy of src.
    EstimatorPub(const EstimatorPub& src)
    {
        circuit_ = src.circuit_;
        observables_ = src.observables_;
        parameter_values_ = src.parameter_values_;
        precision_ = src.precision_;
        has_precision_ = src.has_precision_;
        num_parameter_sets_ = src.num_parameter_sets_;
        serialization_ = src.serialization_;
    }

    ~EstimatorPub() {}

    /// @brief Return a QuantumCircuit for this estimator pub.
    /// @return a quantum circuit
    const circuit::QuantumCircuit& circuit(void) const
    {
        return circuit_;
    }

    /// @brief Return observables for this estimator pub.
    /// @return Runtime-compatible observables JSON
    const nlohmann::ordered_json& observables(void) const
    {
        return observables_;
    }

    /// @brief Return parameter values for this estimator pub.
    /// @return Runtime-compatible parameter-values JSON
    const nlohmann::ordered_json& parameter_values(void) const
    {
        return parameter_values_;
    }

    /// @brief Set Runtime-compatible parameter values for this estimator pub.
    /// @param parameter_values Runtime-compatible parameter-values JSON
    /// @note Takes the payload as-is. Prefer the overloads below, which take
    ///   the values themselves and check them against the circuit.
    /// @param num_parameter_sets the number of binding sets the payload holds,
    ///   which cannot be recovered from the encoded payload. Leave at 1 for a
    ///   payload that is not a sweep.
    void set_parameter_values(const nlohmann::ordered_json& parameter_values,
                              const uint_t num_parameter_sets = 1)
    {
        parameter_values_ = parameter_values;
        num_parameter_sets_ = parameter_values.is_null() ? 0 : num_parameter_sets;
    }

    /// @brief Set the parameter values of this estimator pub.
    /// @param values parameter values, in the order the circuit's parameters
    ///   are bound. Must hold one value per parameter of the circuit.
    /// @throws std::invalid_argument if the number of values does not match
    ///   the circuit's parameter count
    void set_parameter_values(const std::vector<double>& values)
    {
        set_parameter_values(std::vector<std::vector<double>>(1, values));
    }

    /// @brief Set the parameter values of this estimator pub as a sweep.
    /// @param values one list of parameter values per binding set
    /// @throws std::invalid_argument if any set does not match the circuit's
    ///   parameter count
    void set_parameter_values(const std::vector<std::vector<double>>& values)
    {
        // Checked here rather than at submission so that a mismatch is reported
        // where the values were given.
        ParameterValues::validate(values, circuit_.num_parameters());
        parameter_values_ = ParameterValues::to_json(values, circuit_.num_parameters());
        num_parameter_sets_ = values.size();
    }

    /// @brief Return the number of binding sets this pub sweeps over.
    /// @return the number of sets; zero when no values have been set
    /// @details Tracked separately from the payload because the payload is an
    ///   opaque encoded array once built, and a result needs the set count to
    ///   split the values it receives.
    uint_t num_parameter_sets(void) const
    {
        return num_parameter_sets_;
    }

    /// @brief Return whether this pub has an explicit precision.
    /// @return true if an explicit target precision has been set.
    bool has_precision(void) const
    {
        return has_precision_;
    }

    /// @brief Return the explicit target precision for this pub.
    /// @return target precision value.
    double precision(void) const
    {
        return precision_;
    }

    /// @brief Set an explicit target precision for this pub.
    /// @param precision target precision value.
    void set_precision(double precision)
    {
        if (!valid_precision(precision)) {
            throw std::invalid_argument("EstimatorPub precision must be finite and non-negative.");
        }
        precision_ = precision;
        has_precision_ = true;
    }

    /// @brief Return the serialization used for this pub's circuit.
    /// @return the serialization
    CircuitSerialization serialization(void) const
    {
        return serialization_;
    }

    /// @brief Set the serialization used for this pub's circuit.
    /// @param serialization the serialization to use
    /// @note QPY is the default where available and round-trips the circuit
    ///   losslessly. OpenQASM 3 remains accepted by Qiskit Runtime and can be
    ///   selected here as a fallback.
    void set_serialization(const CircuitSerialization serialization)
    {
        serialization_ = serialization;
    }

    /// @brief Return the number of observables in this pub.
    /// @return number of observables.
    uint_t num_observables(void) const
    {
        if (observables_.is_array()) {
            return observables_.size();
        }
        return 1;
    }

    /// @brief Validate this pub.
    /// @param message optional output error message.
    /// @return true if valid.
    bool validate(std::string* message = nullptr) const
    {
        if (has_precision_ && !valid_precision(precision_)) {
            set_message(message, "EstimatorPub precision must be finite and non-negative.");
            return false;
        }

        return validate_observable_json(observables_, circuit_.num_qubits(), message);
    }

    /// @brief Validate this pub for submission.
    /// @param message optional output error message.
    /// @return true if the pub can be submitted.
    /// @details Stricter than ``validate()``: it also requires parameter values
    ///   for a parameterized circuit. Kept separate because the constructors
    ///   validate, and values can only be set once the pub exists.
    bool validate_for_submission(std::string* message = nullptr) const
    {
        if (!validate(message)) {
            return false;
        }

        // A parameterized circuit cannot run without values to bind, and the
        // server has no way to supply them.
        if (circuit_.num_parameters() > 0 && parameter_values_.is_null()) {
            std::stringstream detail;
            detail << "EstimatorPub circuit has " << circuit_.num_parameters()
                   << " parameters but no parameter values were set.";
            set_message(message, detail.str());
            return false;
        }

        return true;
    }

    /// @brief Return a JSON format of this estimator pub.
    /// @return a JSON format estimator pub.
    /// @throws std::invalid_argument if the pub is not valid
    nlohmann::ordered_json to_json(void)
    {
        // The stricter check: a payload is only built for a pub that can
        // actually run, which means a parameterized circuit must have values.
        std::string message;
        if (!validate_for_submission(&message)) {
            throw std::invalid_argument(message);
        }

        nlohmann::ordered_json ret = nlohmann::ordered_json::array();
        ret.push_back(serialize_pub_circuit(circuit_, serialization_));
        ret.push_back(observables_);
        // An unparameterized circuit needs no values; the schema accepts null
        // for that case, which is what the constructors leave in place.
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

    /// @brief Convert a list of Pauli label terms into a Runtime observables value.
    /// @param observable the terms to sum
    /// @return a JSON object mapping each distinct label to its total coefficient
    /// @throws std::invalid_argument if the list is empty or a coefficient is
    ///   not finite
    /// @details A label repeated in the list has its coefficients **summed**,
    ///   so ``{{"Z", 0.5}, {"Z", 0.25}}`` gives ``0.75 * Z``. This matches
    ///   ``SparseObservable::from_list``, which appends a separate term per
    ///   entry, and Qiskit Python, whose estimator coerces a mapping through
    ///   ``SparseObservable.from_list`` so that the terms sum as well. The
    ///   terms keep the order in which their labels first appear.
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
            // Accumulate rather than assign: a repeated label is another term
            // of the same sum, not a correction to the earlier one.
            const auto found = ret.find(observable[i].first);
            if (found == ret.end()) {
                ret[observable[i].first] = coeff;
            } else {
                const double total = found->get<double>() + coeff;
                if (!std::isfinite(total)) {
                    throw std::invalid_argument("Estimator observable coefficients must be finite.");
                }
                *found = total;
            }
        }
        return ret;
    }

    static nlohmann::ordered_json pauli_terms_to_json(const std::vector<std::pair<std::string, std::complex<double>>>& observable)
    {
        std::vector<std::pair<std::string, double>> real_terms;
        real_terms.reserve(observable.size());
        for (uint_t i = 0; i < observable.size(); i++) {
            const auto coeff = observable[i].second;
            if (!std::isfinite(coeff.real()) || !std::isfinite(coeff.imag())) {
                throw std::invalid_argument("Estimator observable coefficients must be finite.");
            }
            if (std::abs(coeff.imag()) > 1e-12) {
                throw std::invalid_argument("Estimator observable coefficients must be real.");
            }
            real_terms.push_back(std::make_pair(observable[i].first, coeff.real()));
        }
        return pauli_terms_to_json(real_terms);
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
            if (boundaries[term] > boundaries[term + 1] || boundaries[term + 1] > bit_terms.size() || boundaries[term + 1] > indices.size()) {
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
