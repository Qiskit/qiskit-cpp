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

// circuit instruction container


#ifndef __qiskitcpp_circuit_circuit_instruction_def_hpp__
#define __qiskitcpp_circuit_circuit_instruction_def_hpp__

#include <memory>
#include <functional>
#include <complex>

#include "circuit/instruction.hpp"

namespace Qiskit {
namespace circuit {

/// @class CircuitInstruction
/// @brief An instruction container for quantum circuit
class CircuitInstruction {
protected:
    Instruction inst_;
    reg_t qubits_;
    reg_t clbits_;
    std::string label_;
public:
    /// @brief Create a new instruction
    CircuitInstruction()
    {
    }

    /// @brief Create a new circuit instruction
    /// @param inst The instruction
    /// @param qubits The list of qubits for the instruction
    /// @param clbits The list of clbits for the instruction
    CircuitInstruction(const Instruction& inst, reg_t qubits, reg_t clbits = reg_t())
    {
        inst_ = inst;
        qubits_ = qubits;
        clbits_ = clbits;
    }

    /// @brief Create a new instruction
    /// @param other The instruction to be copoed
    CircuitInstruction(const CircuitInstruction& other)
    {
        inst_ = other.inst_;
        qubits_ = other.qubits_;
        clbits_ = other.clbits_;
        label_ = other.label_;
    }

    /// @brief Return instruction object of the instruction
    /// @return the instruction
    const Instruction& instruction(void) const
    {
        return inst_;
    }
    /// @brief Return qubits
    /// @return qubits
    const reg_t& qubits(void) const
    {
        return qubits_;
    }
    /// @brief Return clbits
    /// @return clbits
    const reg_t& clbits(void) const
    {
        return clbits_;
    }

    /// @brief Return number of qubits
    /// @return number of qubits
    uint_t num_qubits(void) const
    {
        return qubits_.size();
    }
    /// @brief Return number of clbits
    /// @return number of clbits
    uint_t num_clbits(void) const
    {
        return clbits_.size();
    }
    /// @brief Return label of the instruction
    /// @return label of the instruction
    std::string& label(void)
    {
        return label_;
    }
};

} // namespace circuit
} // namespace Qiskit

#endif  // __qiskitcpp_circuit_circuit_instruction_def_hpp__