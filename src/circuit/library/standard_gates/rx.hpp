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

// RX gates

#ifndef __qiskitcpp_circuit_library_standard_gates_rx_def_hpp__
#define __qiskitcpp_circuit_library_standard_gates_rx_def_hpp__

#include "circuit/instruction.hpp"

namespace Qiskit {
namespace circuit {

/// @class RXGate
/// @brief definition of RX gate
class RXGate : public Instruction {
protected:
public:
    RXGate(void)
    {
        name_ = "rx";
        num_qubits_ = 1;
        num_clbits_ = 0;
        map_ = QkGate_RX;
        is_standard_gate_ = true;
    }

    /// @brief Return number of parameters for this instruction
    /// @return number of parameters
    uint_t num_params(void) const override
    {
        return 1;
    }
};

/// @class CRXGate
/// @brief definition of controlled RX gate
class CRXGate : public Instruction {
protected:
public:
    CRXGate(void)
    {
        name_ = "crx";
        num_qubits_ = 2;
        num_clbits_ = 0;
        map_ = QkGate_CRX;
        is_standard_gate_ = true;
    }

    /// @brief Return number of control bits for this instruction
    /// @return number of control bits
    uint_t num_controll_bits(void) const override
    {
        return 1;
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

#endif  //__qiskitcpp_circuit_library_standard_gates_rx_def_hpp__
