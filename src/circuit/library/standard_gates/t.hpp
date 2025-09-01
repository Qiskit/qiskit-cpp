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

// T gates

#ifndef __qiskitcpp_circuit_library_standard_gates_t_def_hpp__
#define __qiskitcpp_circuit_library_standard_gates_t_def_hpp__

#include "circuit/instruction.hpp"

namespace Qiskit {
namespace circuit {

/// @class TGate
/// @brief definition of T gate
class TGate : public Instruction {
protected:
public:
    TGate(void)
    {
        name_ = "t";
        num_qubits_ = 1;
        num_clbits_ = 0;
        map_ = QkGate_T;
        is_standard_gate_ = true;
    }
};

/// @class TdgGate
/// @brief definition of T-adjoint gate
class TdgGate : public Instruction {
protected:
public:
    TdgGate(void)
    {
        name_ = "tdg";
        num_qubits_ = 1;
        num_clbits_ = 0;
        map_ = QkGate_Tdg;
        is_standard_gate_ = true;
    }
};

} // namespace circuit
} // namespace Qiskit

#endif  //__qiskitcpp_circuit_library_standard_gates_t_def_hpp__
