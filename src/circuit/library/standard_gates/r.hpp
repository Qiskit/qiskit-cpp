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

// R gate

#ifndef __qiskitcpp_circuit_library_standard_gates_r_def_hpp__
#define __qiskitcpp_circuit_library_standard_gates_r_def_hpp__

#include "circuit/instruction.hpp"

namespace Qiskit {
namespace circuit {

/// @class RGate
/// @brief definition of R gate
class RGate : public Instruction {
protected:
public:
    RGate(void)
    {
        name_ = "r";
        num_qubits_ = 1;
        num_clbits_ = 0;
        map_ = QkGate_R;
        is_standard_gate_ = true;
    }

    /// @brief Return number of parameters for this instruction
    /// @return number of parameters
    uint_t num_params(void) const override
    {
        return 1;
    }

};

} // namespace circuit
} // namespace Qiskit

#endif  //__qiskitcpp_circuit_library_standard_gates_r_def_hpp__
