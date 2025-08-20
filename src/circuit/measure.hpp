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

// measure operation

#ifndef __qiskitcpp_circuit_measure_def_hpp__
#define __qiskitcpp_circuit_measure_def_hpp__

#include "circuit/instruction.hpp"

namespace Qiskit {
namespace circuit {

/// @class Measure
/// @brief definition of Measure
class Measure : public Instruction {
protected:
public:
    Measure(void)
    {
        name_ = "measure";
        num_qubits_ = 1;
        num_clbits_ = 1;
    }
};


} // namespace circuit
} // namespace Qiskit

#endif  //__qiskitcpp_circuit_measure_def_hpp__
