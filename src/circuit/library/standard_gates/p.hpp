/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2024.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.
*/

// phase gates

#ifndef __qiskitcpp_circuit_library_standard_gates_p_def_hpp__
#define __qiskitcpp_circuit_library_standard_gates_p_def_hpp__

#include "circuit/instruction.hpp"

namespace Qiskit {
namespace circuit {

/// @class PhaseGate
/// @brief definition of phase gate
class PhaseGate : public Instruction {
protected:
public:
    PhaseGate(void)
    {
        name_ = "p";
        num_qubits_ = 1;
        num_clbits_ = 0;
        map_ = QkGate_Phase;
        is_standard_gate_ = true;
    }

    /// @brief Return number of parameters for this instruction
    /// @return number of parameters
    const uint_t num_params(void) const override
    {
        return 1;
    }
};

/// @class CPhaseGate
/// @brief definition of controlled phase gate
class CPhaseGate : public Instruction {
protected:
public:
    CPhaseGate(void)
    {
        name_ = "cp";
        num_qubits_ = 2;
        num_clbits_ = 0;
        map_ = QkGate_CPhase;
        is_standard_gate_ = true;
    }

    /// @brief Return number of control bits for this instruction
    /// @return number of control bits
    const uint_t num_controll_bits(void) const override
    {
        return 1;
    }

    /// @brief Return number of parameters for this instruction
    /// @return number of parameters
    const uint_t num_params(void) const override
    {
        return 1;
    }
};


} // namespace circuit
} // namespace Qiskit

#endif  //__qiskitcpp_circuit_library_standard_gates_p_def_hpp__
