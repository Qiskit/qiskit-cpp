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

#include "circuit/quantumcircuit.hpp"


namespace Qiskit {
namespace primitives {

static circuit::QuantumCircuit default_circ;

/// @class SamplerPub
/// @brief Sampler Pub(Primitive Unified Bloc)
class SamplerPub {
protected:
    circuit::QuantumCircuit& circuit_;
    std::vector<std::string> params_;
    std::vector<double> values_;
    uint_t shots_ = 0;
public:
    /// @brief Create a new SamplerPub
    SamplerPub() : circuit_(default_circ){}

    /// @brief Create a new SamplerPub
    /// @param circ a QuantumCircuit
    /// @param params parameter names
    /// @param values parameter values
    /// @param shots The total number of shots to sample for this sampler pub
    SamplerPub(circuit::QuantumCircuit& circ, std::vector<std::string> params, std::vector<double> values, uint_t shots) : circuit_(circ)
    {
        params_ = params;
        values_ = values;
        shots_ = shots;
    }

    /// @brief Create a new SamplerPub
    /// @param circ a QuantumCircuit
    /// @param shots The total number of shots to sample for this sampler pub
    SamplerPub(circuit::QuantumCircuit& circ, uint_t shots) : circuit_(circ)
    {
        shots_ = shots;
    }

    /// @brief Create a new SamplerPub as a copy of src.
    /// @param src a SamplerPub
    SamplerPub(const SamplerPub& src)  : circuit_(src.circuit_)
    {
        params_ = src.params_;
        values_ = src.values_;
        shots_ = src.shots_;
    }
    ~SamplerPub(){}

    /// @brief Return a QuantumCircuit for this sampler pub
    /// @return a QuantumCircuit
    circuit::QuantumCircuit& circuit(void)
    {
        return circuit_;
    }

    /// @brief Return a list of parameter names for this sampler pub
    std::vector<std::string>& params(void)
    {
        return params_;
    }

    /// @brief Return a list of parameter values for this sampler pub
    std::vector<double>& values(void)
    {
        return values_;
    }

    /// @brief Return the total number of shots
    uint_t shots(void)
    {
        return shots_;
    }
};

} // namespace primitives
} // namespace Qiskit


#endif //__qiskitcpp_primitives_sampler_pub_def_hpp__
