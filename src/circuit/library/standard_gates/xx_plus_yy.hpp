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

// XX plus YY gate

#ifndef __qiskitcpp_circuit_library_standard_gates_xx_plus_yy_def_hpp__
#define __qiskitcpp_circuit_library_standard_gates_xx_plus_yy_def_hpp__

#include "circuit/instruction.hpp"

namespace Qiskit {
namespace circuit {

/// @class XXPlusYYGate
/// @brief definition of XXPlusYY gate
class XXPlusYYGate : public Instruction {
protected:
public:
    XXPlusYYGate(void)
    {
        name_ = "xx_plus_yy";
        num_qubits_ = 2;
        num_clbits_ = 0;
        map_ = QkGate_XXPlusYY;
        is_standard_gate_ = true;
    }

    /// @brief Return number of parameters for this instruction
    /// @return number of parameters
    uint_t num_params(void) const override
    {
        return 2;
    }
};

} // namespace circuit
} // namespace Qiskit

#endif  //__qiskitcpp_circuit_library_standard_gates_xx_plus_yy_def_hpp__
