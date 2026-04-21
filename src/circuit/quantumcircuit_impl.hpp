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

// Quantum circuit class


#ifndef __qiskitcpp_circuit_quantum_circuit_impl_hpp__
#define __qiskitcpp_circuit_quantum_circuit_impl_hpp__

#include <memory>
#include <functional>
#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <cassert>

#include "controlflow/__init__.hpp"

namespace Qiskit {
namespace circuit {


inline IfElseOp& QuantumCircuit::if_test(const uint32_t clbit, const uint32_t value, const std::function<void(QuantumCircuit &)> body)
{
    pre_add_gate();

    auto op = std::shared_ptr<IfElseOp>(new IfElseOp(*this, clbit, value));
    pending_control_flow_op_ = op;

    body(op->true_body());

    return *op;
}


inline void QuantumCircuit::add_pending_control_flow_op(void)
{
    if (pending_control_flow_op_)
    {
        pending_control_flow_op_->add_control_flow_op(*this);
        pending_control_flow_op_.reset();
        pending_control_flow_op_ = nullptr;
    }
}


}
}

#endif  // __qiskitcpp_circuit_quantum_circuit_impl_hpp__
