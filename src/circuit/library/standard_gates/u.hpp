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

// Generic single-qubit rotation gate with 3 Euler angles

#ifndef __qiskitcpp_circuit_library_standard_gates_u_def_hpp__
#define __qiskitcpp_circuit_library_standard_gates_u_def_hpp__

#include "circuit/instruction.hpp"

namespace Qiskit {
namespace circuit {

/// @class UGate
/// @brief definition of U gate
class UGate : public Instruction {
protected:
public:
    UGate(void)
    {
        name_ = "u";
        num_qubits_ = 1;
        num_clbits_ = 0;
        map_ = QkGate_U;
        is_standard_gate_ = true;
    }

    /// @brief Return number of parameters for this instruction
    /// @return number of parameters
    const uint_t num_params(void) const override
    {
        return 3;
    }
};

/// @class CUGate
/// @brief definition of controlled U gate
class CUGate : public Instruction {
protected:
public:
    CUGate(void)
    {
        name_ = "cu";
        num_qubits_ = 2;
        num_clbits_ = 0;
        map_ = QkGate_CU;
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
        return 3;
    }
};


} // namespace circuit
} // namespace Qiskit

#endif  //__qiskitcpp_circuit_library_standard_gates_u_def_hpp__
