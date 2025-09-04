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

// SX gates

#ifndef __qiskitcpp_circuit_library_standard_gates_sx_def_hpp__
#define __qiskitcpp_circuit_library_standard_gates_sx_def_hpp__

#include "circuit/instruction.hpp"

namespace Qiskit {
namespace circuit {

/// @class SXGate
/// @brief definition of SX gate
class SXGate : public Instruction {
protected:
public:
    SXGate(void)
    {
        name_ = "sx";
        num_qubits_ = 1;
        num_clbits_ = 0;
        map_ = QkGate_SX;
        is_standard_gate_ = true;
    }
};

/// @class SdgGate
/// @brief definition of SX-adjoint gate
class SXdgGate : public Instruction {
protected:
public:
    SXdgGate(void)
    {
        name_ = "sxdg";
        num_qubits_ = 1;
        num_clbits_ = 0;
        map_ = QkGate_SXdg;
        is_standard_gate_ = true;
    }
};


/// @class CSXGate
/// @brief definition of controlled SX gate
class CSXGate : public Instruction {
protected:
public:
    CSXGate(void)
    {
        name_ = "csx";
        num_qubits_ = 2;
        num_clbits_ = 0;
        map_ = QkGate_CSX;
        is_standard_gate_ = true;
    }

    /// @brief Return number of control bits for this instruction
    /// @return number of control bits
    uint_t num_controll_bits(void) const override
    {
        return 1;
    }
};


} // namespace circuit
} // namespace Qiskit

#endif  //__qiskitcpp_circuit_library_standard_gates_sx_def_hpp__
