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

// global phase

#ifndef __qiskitcpp_circuit_library_standard_gates_global_phase_def_hpp__
#define __qiskitcpp_circuit_library_standard_gates_global_phase_def_hpp__

#include "circuit/instruction.hpp"

namespace Qiskit {
namespace circuit {

/// @class GlobalPhaseGate
/// @brief definition of global Phase
class GlobalPhaseGate : public Instruction {
protected:
public:
    GlobalPhaseGate(void)
    {
        name_ = "global_phase";
        num_qubits_ = 0;
        num_clbits_ = 0;
    }

    /// @brief Return gate enum for Qiskit C-API
    /// @return QkGate enum for this instruciton
    const QkGate gate_map(void) const override
    {
        return QkGate_GlobalPhase;
    }

    /// @brief Return number of parameters for this instruction
    /// @return number of parameters
    const uint_t num_params(void) const override
    {
        return 1;
    }
    /// @brief check if this isntruction is a standard gate
    /// @return true if the instruction is a standard gate
    const bool is_standard_gate(void) const override
    {
        return true;
    }

};


} // namespace circuit
} // namespace Qiskit

#endif  //__qiskitcpp_circuit_library_standard_gates_global_phase_def_hpp__
