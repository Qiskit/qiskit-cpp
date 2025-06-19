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

// base class for control flow instructions

#ifndef __qiskitcpp_circuit_control_flow_hpp__
#define __qiskitcpp_circuit_control_flow_hpp__

#include "circuit/quantumcircuit_def.hpp"

namespace Qiskit {
namespace circuit {


/// @class ControlFlowOp
/// @brief Abstract class to encapsulate all control flow operations
class ControlFlowOp
{
protected:
  // TO DO : this is temporary, make condition class to handle multiple conditions
  Expr expr_;
  uint32_t clbit_;
  uint32_t value_;
public:
  /// @brief Create a new instruction.
  ControlFlowOp()
  {
  }
  /// @brief Create a new instruction.
  /// @param (clbit)
  /// @param (value)
  ControlFlowOp(uint32_t clbit, uint32_t value) : clbit_(clbit), value_(value) {}

  /// @brief Create a new instruction.
  /// @param (expr) expression
  ControlFlowOp(Expr expr) : expr_(expr) {}

  virtual void add_control_flow_op(QuantumCircuit& circ) = 0;
};

} // namespace circuit
} // namespace Qiskit

#endif  // __qiskitcpp_circuit_control_flow_hpp__

