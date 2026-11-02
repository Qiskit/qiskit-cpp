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

// sampler pub definition

#ifndef __qiskitcpp_primitives_sampler_pub_def_hpp__
#define __qiskitcpp_primitives_sampler_pub_def_hpp__

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <stdexcept>
#include <vector>

#include "circuit/quantumcircuit.hpp"
#include "primitives/containers/circuit_serialization.hpp"
#include "primitives/containers/parameter_values.hpp"


namespace Qiskit {
namespace primitives {

static circuit::QuantumCircuit default_circ;

/// @class SamplerPub
/// @brief Sampler Pub(Primitive Unified Bloc)
class SamplerPub {
protected:
    circuit::QuantumCircuit circuit_;
    // One list of values per binding set, each in the order the circuit's
    // parameters are bound. Several sets make the pub a sweep.
    std::vector<std::vector<double>> parameter_values_;
    uint_t shots_ = 0;
    CircuitSerialization serialization_ = default_circuit_serialization();
public:
    /// @brief Create a new SamplerPub
    SamplerPub() {}

    /// @brief Create a new SamplerPub
    /// @param circ a QuantumCircuit
    /// @param shots The total number of shots to sample for this sampler pub
    SamplerPub(circuit::QuantumCircuit& circ, uint_t shots = 0)
    {
        circuit_ = circ;
        shots_ = shots;
    }

    /// @brief Create a new SamplerPub for a parameterized circuit
    /// @param circ a QuantumCircuit
    /// @param values parameter values, in the order the circuit's parameters
    ///   are bound. Must hold one value per parameter of the circuit.
    /// @param shots The total number of shots to sample for this sampler pub
    /// @throws std::invalid_argument if the number of values does not match
    ///   ``circ.num_parameters()``
    SamplerPub(circuit::QuantumCircuit& circ, const std::vector<double>& values,
               uint_t shots = 0)
    {
        circuit_ = circ;
        shots_ = shots;
        set_parameter_values(values);
    }

    /// @brief Create a new SamplerPub sweeping over several parameter sets
    /// @param circ a QuantumCircuit
    /// @param values one list of parameter values per binding set, each holding
    ///   one value per parameter of the circuit
    /// @param shots The total number of shots to sample for this sampler pub
    /// @throws std::invalid_argument if any set does not match
    ///   ``circ.num_parameters()``
    SamplerPub(circuit::QuantumCircuit& circ,
               const std::vector<std::vector<double>>& values, uint_t shots = 0)
    {
        circuit_ = circ;
        shots_ = shots;
        set_parameter_values(values);
    }

    /// @brief Create a new SamplerPub as a copy of src.
    /// @param src a SamplerPub
    SamplerPub(const SamplerPub& src)
    {
        circuit_ = src.circuit_;
        parameter_values_ = src.parameter_values_;
        shots_ = src.shots_;
        serialization_ = src.serialization_;
    }
    ~SamplerPub(){}

    /// @brief Return a QuantumCircuit for this sampler pub
    /// @return a quantum circuit
    const circuit::QuantumCircuit& circuit(void) const
    {
        return circuit_;
    }

    /// @brief Return the parameter value sets of this sampler pub
    /// @return one list of values per binding set
    const std::vector<std::vector<double>>& parameter_values(void) const
    {
        return parameter_values_;
    }

    /// @brief Set the parameter values of this sampler pub
    /// @param values parameter values, in the order the circuit's parameters
    ///   are bound. Must hold one value per parameter of the circuit.
    /// @throws std::invalid_argument if the number of values does not match
    ///   the circuit's parameter count
    void set_parameter_values(const std::vector<double>& values)
    {
        // An empty list for a circuit with no parameters means "nothing to
        // bind" rather than "one empty binding set", so it is stored as no sets
        // at all and num_parameter_sets() reports zero.
        if (values.empty() && circuit_.num_parameters() == 0) {
            parameter_values_.clear();
            return;
        }
        set_parameter_values(std::vector<std::vector<double>>(1, values));
    }

    /// @brief Set the parameter values of this sampler pub as a sweep
    /// @param values one list of parameter values per binding set
    /// @throws std::invalid_argument if any set does not match the circuit's
    ///   parameter count
    void set_parameter_values(const std::vector<std::vector<double>>& values)
    {
        // Checked here rather than at submission so that a mismatch is reported
        // where the values were given.
        ParameterValues::validate(values, circuit_.num_parameters());
        parameter_values_ = values;
    }

    /// @brief Return the number of binding sets this pub sweeps over
    /// @return the number of sets; zero for an unparameterized circuit
    uint_t num_parameter_sets(void) const
    {
        return parameter_values_.size();
    }

    /// @brief Return the total number of shots
    uint_t shots(void)
    {
        return shots_;
    }

    /// @brief Validate this pub.
    /// @param message optional output error message.
    /// @return true if valid.
    bool validate(std::string* message = nullptr) const
    {
        try {
            ParameterValues::validate(parameter_values_, circuit_.num_parameters());
        } catch (const std::invalid_argument& exc) {
            if (message != nullptr) {
                *message = exc.what();
            }
            return false;
        }
        return true;
    }

    /// @brief Return the serialization used for this pub's circuit
    /// @return the serialization
    CircuitSerialization serialization(void) const
    {
        return serialization_;
    }

    /// @brief Set the serialization used for this pub's circuit
    /// @param serialization the serialization to use
    /// @note QPY is the default where available and round-trips the circuit
    ///   losslessly. OpenQASM 3 remains accepted by Qiskit Runtime and can be
    ///   selected here as a fallback.
    void set_serialization(const CircuitSerialization serialization)
    {
        serialization_ = serialization;
    }

    /// @brief Return a JSON format of this sampler pub
    /// @return a JSON format sampler pub
    /// @throws std::invalid_argument if the parameter values do not match the
    ///   circuit's parameter count
    nlohmann::ordered_json to_json(void)
    {
        // The circuit may have gained or lost parameters since the values were
        // set, so the count is rechecked here before anything is submitted.
        const uint_t num_parameters = circuit_.num_parameters();

        // Built up element by element rather than from an initializer list so
        // that the circuit stays an ordered_json throughout. Going through the
        // unordered nlohmann::json would re-sort the keys of the QPY payload
        // object.
        nlohmann::ordered_json ret = nlohmann::ordered_json::array();
        ret.push_back(serialize_pub_circuit(circuit_, serialization_));
        ret.push_back(ParameterValues::to_json(parameter_values_, num_parameters));
        if (shots_ > 0) {
            ret.push_back(shots_);
        }
        return ret;
    }

};

} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_sampler_pub_def_hpp__
