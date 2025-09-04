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

// instruction interface


#ifndef __qiskitcpp_circuit_instruction_def_hpp__
#define __qiskitcpp_circuit_instruction_def_hpp__

#include <memory>
#include <functional>
#include <complex>

#include "utils/types.hpp"
#include "circuit/classicalregister.hpp"
#include "circuit/quantumregister.hpp"

#include "qiskit.h"

namespace Qiskit {
namespace circuit {

/// @class Instruction
/// @brief A generic quantum instruction.
class Instruction {
protected:
    std::string name_;
    uint_t num_qubits_;
    uint_t num_clbits_;
    std::vector<double> params_;
    std::string label_;
    QkGate map_ = QkGate_I;
    bool is_standard_gate_ = false;
public:
    /// @brief Create a new instruction
    Instruction()
    {
        num_qubits_ = 1;
        num_clbits_ = 0;
    }

    /// @brief Create a new instruction
    /// @param name The name of the instruction
    /// @param num_qubits The number of qubits in the instruction
    /// @param num_clbits The number of clbits in the instruction
    Instruction(std::string& name, uint_t num_qubits, uint_t num_clbits)
    {
        name_ = name;
        num_qubits_ = num_qubits;
        num_clbits_ = num_clbits;
    }

    /// @brief Create a new instruction
    /// @param other The instruction to be copoed
    Instruction(const Instruction& other)
    {
        name_ = other.name_;
        num_qubits_ = other.num_qubits_;
        num_clbits_ = other.num_clbits_;
        params_ = other.params_;
        label_ = other.label_;
        map_ = other.map_;
        is_standard_gate_ = other.is_standard_gate_;
    }

    /// @brief Return name of the instruction
    /// @return name of the instruction
    const std::string& name(void) const
    {
        return name_;
    }
    /// @brief Return number of qubits
    /// @return number of qubits
    uint_t num_qubits(void) const
    {
        return num_qubits_;
    }
    /// @brief Return number of clbits
    /// @return number of clbits
    uint_t num_clbits(void) const
    {
        return num_clbits_;
    }
    /// @brief set number of qubits
    /// @param num_qubits number of qubits
    void set_num_qubits(uint_t num_qubits)
    {
        num_qubits_ = num_qubits;
    }
    /// @brief set number of clbits
    /// @param num_clbits number of clbits
    void set_num_clbits(uint_t num_clbits)
    {
        num_clbits_ = num_clbits;
    }

    /// @brief Return parameters of the instruction
    /// @return parameters of the instruction
    const std::vector<double>& params(void) const
    {
        return params_;
    }

    /// @brief set parameters to the instruction
    /// @param parameters to be set
    void set_params(const std::vector<double>& params)
    {
        params_ = params;
    }

    /// @brief Return label of the instruction
    /// @return label of the instruction
    std::string& label(void)
    {
        return label_;
    }

    /// @brief Return number of control bits for this instruction
    /// @return number of control bits
    virtual uint_t num_controll_bits(void) const
    {
        return 0;
    }

    /// @brief Return number of parameters for this instruction
    /// @return number of parameters
    virtual uint_t num_params(void) const
    {
        return params_.size();
    }

    /// @brief Return gate enum for Qiskit C-API
    /// @return QkGate enum for this instruciton
    QkGate gate_map(void) const
    {
        return map_;
    }

    /// @brief check if this isntruction is a standard gate
    /// @return true if the instruction is a standard gate
    virtual bool is_standard_gate(void) const
    {
        return is_standard_gate_;
    }

};






} // namespace circuit
} // namespace Qiskit


#endif  // __qiskitcpp_circuit_instruction_def_hpp__
