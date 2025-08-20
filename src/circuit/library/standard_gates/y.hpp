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

// Y gates

#ifndef __qiskitcpp_circuit_library_standard_gates_y_def_hpp__
#define __qiskitcpp_circuit_library_standard_gates_y_def_hpp__

#include "circuit/instruction.hpp"

namespace Qiskit {
namespace circuit {

/// @class YGate
/// @brief definition of Y gate
class YGate : public Instruction {
protected:
public:
    YGate(void)
    {
        name_ = "y";
        num_qubits_ = 1;
        num_clbits_ = 0;
    }

    /// @brief Return gate enum for Qiskit C-API
    /// @return QkGate enum for this instruciton
    const QkGate gate_map(void) const override
    {
        return QkGate_Y;
    }
    /// @brief check if this isntruction is a standard gate
    /// @return true if the instruction is a standard gate
    const bool is_standard_gate(void) const override
    {
        return true;
    }
};

/// @class CYGate
/// @brief definition of controlled Y gate
class CYGate : public Instruction {
protected:
public:
    CYGate(void)
    {
        name_ = "cy";
        num_qubits_ = 2;
        num_clbits_ = 0;
    }

    /// @brief Return gate enum for Qiskit C-API
    /// @return QkGate enum for this instruciton
    const QkGate gate_map(void) const override
    {
        return QkGate_CY;
    }

    /// @brief Return number of control bits for this instruction
    /// @return number of control bits
    const uint_t num_controll_bits(void) const override
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

#endif  //__qiskitcpp_circuit_library_standard_gates_y_def_hpp__
