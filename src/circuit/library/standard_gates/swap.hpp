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

// Swap gates

#ifndef __qiskitcpp_circuit_library_standard_gates_swap_def_hpp__
#define __qiskitcpp_circuit_library_standard_gates_swap_def_hpp__

#include "circuit/instruction.hpp"

namespace Qiskit {
namespace circuit {

/// @class SwapGate
/// @brief definition of H gate
class SwapGate : public Instruction {
protected:
public:
    SwapGate(void)
    {
        name_ = "swap";
        num_qubits_ = 2;
        num_clbits_ = 0;
        map_ = QkGate_Swap;
        is_standard_gate_ = true;
    }
};

/// @class CSwapGate
/// @brief definition of controlled Swap gate
class CSwapGate : public Instruction {
protected:
public:
    CSwapGate(void)
    {
        name_ = "cswap";
        num_qubits_ = 3;
        num_clbits_ = 0;
        map_ = QkGate_CSwap;
        is_standard_gate_ = true;
    }

    /// @brief Return number of control bits for this instruction
    /// @return number of control bits
    const uint_t num_controll_bits(void) const override
    {
        return 1;
    }
};


} // namespace circuit
} // namespace Qiskit

#endif  //__qiskitcpp_circuit_library_standard_gates_swap_def_hpp__
