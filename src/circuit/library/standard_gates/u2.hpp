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

// Single-qubit rotation about the X+Z axis.

#ifndef __qiskitcpp_circuit_library_standard_gates_u2_def_hpp__
#define __qiskitcpp_circuit_library_standard_gates_u2_def_hpp__

#include "circuit/instruction.hpp"

namespace Qiskit {
namespace circuit {

/// @class U2Gate
/// @brief definition of U2 gate
class U2Gate : public Instruction {
protected:
public:
    U2Gate(void)
    {
        name_ = "u2";
        num_qubits_ = 1;
        num_clbits_ = 0;
        map_ = QkGate_U2;
        is_standard_gate_ = true;
    }

    /// @brief Return number of parameters for this instruction
    /// @return number of parameters
    const uint_t num_params(void) const override
    {
        return 2;
    }
};

} // namespace circuit
} // namespace Qiskit

#endif  //__qiskitcpp_circuit_library_standard_gates_u2_def_hpp__
