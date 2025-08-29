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

// X gates

#ifndef __qiskitcpp_circuit_library_standard_gates_x_def_hpp__
#define __qiskitcpp_circuit_library_standard_gates_x_def_hpp__

#include "circuit/instruction.hpp"

namespace Qiskit {
namespace circuit {

/// @class XGate
/// @brief definition of X gate
class XGate : public Instruction {
protected:
public:
    XGate(void)
    {
        name_ = "x";
        num_qubits_ = 1;
        num_clbits_ = 0;
        map_ = QkGate_X;
        is_standard_gate_ = true;
    }
};

/// @class CXGate
/// @brief definition of controlled X gate
class CXGate : public Instruction {
protected:
public:
    CXGate(void)
    {
        name_ = "cx";
        num_qubits_ = 2;
        num_clbits_ = 0;
        map_ = QkGate_CX;
        is_standard_gate_ = true;
    }

    /// @brief Return number of control bits for this instruction
    /// @return number of control bits
    const uint_t num_controll_bits(void) const override
    {
        return 1;
    }
};

/// @class CCXGate
/// @brief definition of controlled X gate
class CCXGate : public Instruction {
protected:
public:
    CCXGate(void)
    {
        name_ = "ccx";
        num_qubits_ = 3;
        num_clbits_ = 0;
        map_ = QkGate_CCX;
        is_standard_gate_ = true;
    }

    /// @brief Return number of control bits for this instruction
    /// @return number of control bits
    const uint_t num_controll_bits(void) const override
    {
        return 2;
    }
};

/// @class C3XGate
/// @brief definition of controlled X gate
class C3XGate : public Instruction {
protected:
public:
    C3XGate(void)
    {
        name_ = "c3x";
        num_qubits_ = 4;
        num_clbits_ = 0;
        map_ = QkGate_C3X;
        is_standard_gate_ = true;
    }

    /// @brief Return number of control bits for this instruction
    /// @return number of control bits
    const uint_t num_controll_bits(void) const override
    {
        return 3;
    }
};

/// @class C3SXGate
/// @brief definition of controlled X gate
class C3SXGate : public Instruction {
protected:
public:
    C3SXGate(void)
    {
        name_ = "c3sx";
        num_qubits_ = 4;
        num_clbits_ = 0;
        map_ = QkGate_C3SX;
        is_standard_gate_ = true;
    }

    /// @brief Return number of control bits for this instruction
    /// @return number of control bits
    const uint_t num_controll_bits(void) const override
    {
        return 3;
    }
};


/// @class RCCXGate
/// @brief definition of controlled X gate
class RCCXGate : public Instruction {
protected:
public:
    RCCXGate(void)
    {
        name_ = "rccx";
        num_qubits_ = 3;
        num_clbits_ = 0;
        map_ = QkGate_RCCX;
        is_standard_gate_ = true;
    }

    /// @brief Return number of control bits for this instruction
    /// @return number of control bits
    const uint_t num_controll_bits(void) const override
    {
        return 2;
    }
};

/// @class RC3XGate
/// @brief definition of controlled X gate
class RC3XGate : public Instruction {
protected:
public:
    RC3XGate(void)
    {
        name_ = "rcccx";
        num_qubits_ = 4;
        num_clbits_ = 0;
        map_ = QkGate_RC3X;
        is_standard_gate_ = true;
    }

    /// @brief Return number of control bits for this instruction
    /// @return number of control bits
    const uint_t num_controll_bits(void) const override
    {
        return 3;
    }
};



/// @class MCXGate
/// @brief definition of multi-controlled X gate
class MCXGate : public Instruction {
protected:
public:
    MCXGate(void)
    {
        name_ = "mcx";
        num_qubits_ = 3;
        num_clbits_ = 0;
        map_ = QkGate_RC3X;
        is_standard_gate_ = true;
    }

    MCXGate(uint_t num_qubits)
    {
        name_ = "mcx";
        num_qubits_ = num_qubits;
        num_clbits_ = 0;
        map_ = QkGate_RC3X;
        is_standard_gate_ = true;
    }

    /// @brief Return number of control bits for this instruction
    /// @return number of control bits
    const uint_t num_controll_bits(void) const override
    {
        return num_qubits_ - 1;
    }
};


} // namespace circuit
} // namespace Qiskit

#endif  //__qiskitcpp_circuit_library_standard_gates_x_def_hpp__
