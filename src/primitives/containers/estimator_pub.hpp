/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2026.
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
#include <stdexcept>
#include <vector>

#include "circuit/quantumcircuit.hpp"
#include "quantum_info/sparse_observable.hpp"

namespace Qiskit {
namespace primitives {

/// @class EstimatorPub
/// @brief Estimator Pub(Primitive Unified Bloc).
class EstimatorPub {
protected:
    circuit::QuantumCircuit circuit_;
    std::vector<quantum_info::SparseObservable> observables_;
    double precision_ = 0.0;

    void validate(void) const
    {
        if (!std::isfinite(precision_) || precision_ < 0.0) {
            throw std::invalid_argument("EstimatorPub: precision must be non-negative");
        }
        for (uint_t i = 0; i < observables_.size(); i++) {
            if (observables_[i].num_qubits() != circuit_.num_qubits()) {
                throw std::invalid_argument("EstimatorPub: observable/circuit width mismatch");
            }
        }
    }

public:
    /// @brief Create a new EstimatorPub
    EstimatorPub() {}

    /// @brief Create a new EstimatorPub
    /// @param circuit quantum circuit
    /// @param observable observable to estimate
    /// @param precision target precision. Zero means unset.
    EstimatorPub(const circuit::QuantumCircuit& circuit,
                 const quantum_info::SparseObservable& observable,
                 double precision = 0.0)
    {
        circuit_ = circuit.copy();
        observables_.push_back(observable);
        precision_ = precision;
        validate();
    }

    /// @brief Create a new EstimatorPub
    /// @param circuit quantum circuit
    /// @param observables observables to estimate
    /// @param precision target precision. Zero means unset.
    EstimatorPub(const circuit::QuantumCircuit& circuit,
                 const std::vector<quantum_info::SparseObservable>& observables,
                 double precision = 0.0)
    {
        circuit_ = circuit.copy();
        observables_ = observables;
        precision_ = precision;
        validate();
    }

    /// @brief Create a new EstimatorPub as a copy of src.
    EstimatorPub(const EstimatorPub& src)
    {
        circuit_ = src.circuit_.copy();
        observables_ = src.observables_;
        precision_ = src.precision_;
    }

    EstimatorPub& operator=(const EstimatorPub& src)
    {
        if (this == &src)
            return *this;
        circuit_ = src.circuit_.copy();
        observables_ = src.observables_;
        precision_ = src.precision_;
        return *this;
    }

    /// @brief Return a QuantumCircuit for this estimator pub.
    const circuit::QuantumCircuit& circuit(void) const
    {
        return circuit_;
    }

    /// @brief Return observables for this estimator pub.
    const std::vector<quantum_info::SparseObservable>& observables(void) const
    {
        return observables_;
    }

    /// @brief Return requested precision. Zero means unset.
    double precision(void) const
    {
        return precision_;
    }
};

} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_estimator_pub_def_hpp__
