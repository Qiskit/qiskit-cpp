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

// Z gates

#ifndef __qiskitcpp_circuit_library_standard_gates_z_def_hpp__
#define __qiskitcpp_circuit_library_standard_gates_z_def_hpp__

#include "circuit/instruction.hpp"

namespace Qiskit {
namespace circuit {

/// @class ZGate
/// @brief definition of Z gate
class ZGate : public Instruction {
protected:
public:
    ZGate(void)
    {
        name_ = "z";
        num_qubits_ = 1;
        num_clbits_ = 0;
        map_ = QkGate_Z;
        is_standard_gate_ = true;
    }
};

/// @class CZGate
/// @brief definition of controlled Z gate
class CZGate : public Instruction {
protected:
public:
    CZGate(void)
    {
        name_ = "cz";
        num_qubits_ = 2;
        num_clbits_ = 0;
        map_ = QkGate_CZ;
        is_standard_gate_ = true;
    }

    /// @brief Return number of control bits for this instruction
    /// @return number of control bits
    uint_t num_controll_bits(void) const override
    {
        return 1;
    }
};

/// @class CCZGate
/// @brief definition of controlled X gate
class CCZGate : public Instruction {
protected:
public:
    CCZGate(void)
    {
        name_ = "ccz";
        num_qubits_ = 3;
        num_clbits_ = 0;
        map_ = QkGate_CCZ;
        is_standard_gate_ = true;
    }

    /// @brief Return number of control bits for this instruction
    /// @return number of control bits
    uint_t num_controll_bits(void) const override
    {
        return 2;
    }
};

} // namespace circuit
} // namespace Qiskit

#endif  //__qiskitcpp_circuit_library_standard_gates_z_def_hpp__
