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

#ifndef __qiskitcpp_circuit_if_else_hpp__
#define __qiskitcpp_circuit_if_else_hpp__


#include "circuit/controlflow/control_flow.hpp"

namespace qiskitcpp {
namespace circuit {


/// @class IfElseOp
/// @brief A circuit operation which executes a program(true_body_) if a provided condition evaluates to true, and optionally evaluates another program(false_body_) otherwise.
class IfElseOp : public ControlFlowOp
{
protected:
  qiskitcpp::circuit::QuantumCircuit true_body_;
  qiskitcpp::circuit::QuantumCircuit false_body_;
  bool test_else_ = false;
public:
  /// @brief Create a new IfElseOp circuit operator
  /// @param (circ)
  /// @param (clbit)
  /// @param (value)
  IfElseOp(qiskitcpp::circuit::QuantumCircuit& circ, uint32_t clbit, uint32_t value) : ControlFlowOp(clbit, value), true_body_(circ), false_body_(circ) {}

  /// @brief Return true body available in this operator
  /// @return true body
  qiskitcpp::circuit::QuantumCircuit& true_body() { return true_body_; }

  /// @brief Return false body available in this operator
  /// @return false body
  qiskitcpp::circuit::QuantumCircuit& false_body() { return false_body_; }

  // add false body
  /// @brief add false body
  void else_(std::function<void(qiskitcpp::circuit::QuantumCircuit&)> body);

  void add_control_flow_op(qiskitcpp::circuit::QuantumCircuit& circ) override;
};

void IfElseOp::else_(std::function<void(qiskitcpp::circuit::QuantumCircuit&)> body)
{
  body(false_body_);
  test_else_ = true;
}

void IfElseOp::add_control_flow_op(qiskitcpp::circuit::QuantumCircuit& circ)
{
  if(test_else_){
    // if_else
    rust_circuit* true_circ = true_body_.get_rust_circuit();
    rust_circuit* false_circ = false_body_.get_rust_circuit();

    // TO DO : Add if_else support in Qiskit C-API (maybe in the future?)
//    qc_if_else(circ.get_rust_circuit(false), clbit_, value_, true_circ, false_circ);
  }
  else{
    // if_test
    rust_circuit* true_circ = true_body_.get_rust_circuit();

    // TO DO : Add if_else support in Qiskit C-API (maybe in the future?)
    //qc_if_test(circ.get_rust_circuit(false), clbit_, value_, true_circ);
  }
}



} // namespace circuit
} // namespace qiskitcpp

#endif  // __qiskitcpp_circuit_if_else_hpp__

