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

// I gate

#ifndef __qiskitcpp_circuit_library_standard_gates_i_def_hpp__
#define __qiskitcpp_circuit_library_standard_gates_i_def_hpp__

#include "circuit/instruction.hpp"

namespace Qiskit {
namespace circuit {

/// @class IGate
/// @brief definition of I gate
class IGate : public Instruction {
protected:
public:
    IGate(void)
    {
        name_ = "id";
        num_qubits_ = 1;
        num_clbits_ = 0;
        map_ = QkGate_I;
        is_standard_gate_ = true;
    }
};


} // namespace circuit
} // namespace Qiskit

#endif  //__qiskitcpp_circuit_library_standard_gates_i_def_hpp__
