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

// Quantum circuit class definition


#ifndef __qiskitcpp_circuit_quantum_circuit_def_hpp__
#define __qiskitcpp_circuit_quantum_circuit_def_hpp__

#include <memory>
#include <functional>

#include "utils/types.hpp"
#include "circuit/parameter.hpp"
#include "circuit/classical/expr.hpp"
#include "circuit/classicalregister.hpp"
#include "circuit/quantumregister.hpp"

#include <complex>
#include "qiskit.h"

// this macro replaces Complex64 used in rust FFI
#define Complex64 std::complex<double>

// qiskit C-API circuit data
using rust_circuit = ::QkCircuit;

namespace Qiskit {
namespace circuit {


class ControlFlowOp;
class IfElseOp;

static Parameter null_param;

/// @class QuantumCircuit
/// @brief Qiskit representation of a quantum circuit.
class QuantumCircuit {
protected:
  uint_t num_qubits_;
  uint_t num_clbits_;
  // global phase will be used in the future release of Qiskit C-API
  double global_phase_;

  std::vector<QuantumRegister> qregs_;
  std::vector<ClassicalRegister> cregs_;

  rust_circuit* rust_circuit_ = nullptr;

  std::shared_ptr<ControlFlowOp> pending_control_flow_op_ = nullptr;
public:
  static std::unordered_map<std::string, QkGate> gate_map_;


  /// @brief Create a new QuantumCircuit
  QuantumCircuit() {}

  /// @brief Create a new QuantumCircuit
  /// @param num_qubits The number of qubits in the circuit
  /// @param num_clbits The number of clbits in the circuit
  /// @param global_phase The global phase of the circuit, measured in radians
  QuantumCircuit(const uint_t num_qubits, const uint_t num_clbits, const double global_phase = 0.0);

  /// @brief Create a new QuantumCircuit
  /// @param qreg A QuantumRegister
  /// @param creg A ClassicalRegister
  /// @param global_phase The global phase of the circuit, measured in radians
  QuantumCircuit(QuantumRegister& qreg, ClassicalRegister& creg, const double global_phase = 0.0);

  /// @brief Create a new QuantumCircuit
  /// @param qregs A list of QuantumRegister
  /// @param cregs A list of ClassicalRegister
  /// @param global_phase The global phase of the circuit, measured in radians
  QuantumCircuit(std::vector<QuantumRegister>& qregs, std::vector<ClassicalRegister>& cregs, const double global_phase = 0.0);

  /// @brief Create a new QuantumCircuit as a copy of circ.
  /// @param circ copy source
  QuantumCircuit(const QuantumCircuit& circ);

  ~QuantumCircuit();

  /// @brief Return number of qubits
  /// @return number of qubits
  uint_t num_qubits(void) const { return num_qubits_; }

  /// @brief Return number of classical bits
  /// @return number of classical bits
  uint_t num_clbits(void) const { return num_clbits_; }

  rust_circuit* get_rust_circuit(const bool update = true)
  {
    if(update)
      add_pending_control_flow_op();
    return rust_circuit_;
  }

  void from_rust_circuit(rust_circuit* circ);

  /// @brief set global phase
  /// @param (phase) global phase value
  void global_phase(const double phase);

  /// @brief Apply HGate
  /// @param (qubit) The qubit to apply the gate to.
  void h(const uint_t qubit);

  /// @brief Apply IGate
  /// @param (qubit) The qubit to apply the gate to.
  void i(const uint_t qubit);

  /// @brief Apply XGate
  /// @param (qubit) The qubit to apply the gate to.
  void x(const uint_t qubit);

  /// @brief Apply YGate
  /// @param (qubit) The qubit to apply the gate to.
  void y(const uint_t qubit);

  /// @brief Apply ZGate
  /// @param (qubit) The qubit to apply the gate to.
  void z(const uint_t qubit);

  /// @brief Apply PhaseGate
  /// @param (phase) Phase.
  /// @param (qubit) The qubit to apply the gate to.
  void p(const double phase, const uint_t qubit);

  /// @brief Apply PhaseGate
  /// @param (phase) Phase.
  /// @param (qubit) The qubit to apply the gate to.
  void p(const Parameter& phase, const uint_t qubit);

  /// @brief Apply RGate
  /// @param (theta) The angle of the rotation.
  /// @param (phi) The angle of the axis of rotation in the x-y plane.
  /// @param (qubit) The qubit to apply the gate to.
  void r(const double theta, const double phi, const uint_t qubit);

  /// @brief Apply RGate
  /// @param (theta) The angle of the rotation
  /// @param (phi) The angle of the axis of rotation in the x-y plane
  /// @param (qubit) The qubit to apply the gate to
  void r(const Parameter &theta, const Parameter &phi, const uint_t qubit);

  /// @brief Apply RXGate
  /// @param (theta) The angle of the rotation
  /// @param (qubit) The qubit to apply the gate to
  void rx(const double theta, const uint_t qubit);

  /// @brief Apply RXGate
  /// @param (theta) The angle of the rotation
  /// @param (qubit) The qubit to apply the gate to
  void rx(const Parameter &theta, const uint_t qubit);

  /// @brief Apply RYGate
  /// @param (theta) The angle of the rotation
  /// @param (qubit) The qubit to apply the gate to
  void ry(const double theta, const uint_t qubit);

  /// @brief Apply RYGate
  /// @param (theta) The angle of the rotation
  /// @param (qubit) The qubit to apply the gate to
  void ry(const Parameter &theta, const uint_t qubit);

  /// @brief Apply RZGate
  /// @param (theta) The angle of the rotation
  /// @param (qubit) The qubit to apply the gate to
  void rz(const double theta, const uint_t qubit);

  /// @brief Apply RZGate
  /// @param (theta) The angle of the rotation
  /// @param (qubit) The qubit to apply the gate to
  void rz(const Parameter &theta, const uint_t qubit);

  /// @brief Apply SGate
  /// @param (qubit) The qubit to apply the gate to.
  void s(const uint_t qubit);

  /// @brief Apply SdgGate
  /// @param (qubit) The qubit to apply the gate to.
  void sdg(const uint_t qubit);

  /// @brief Apply SXGate
  /// @param (qubit) The qubit to apply the gate to.
  void sx(const uint_t qubit);

  /// @brief Apply SXdgGate
  /// @param (qubit) The qubit to apply the gate to.
  void sxdg(const uint_t qubit);

  /// @brief Apply TGate
  /// @param (qubit) The qubit to apply the gate to.
  void t(const uint_t qubit);

  /// @brief Apply TdgGate
  /// @param (qubit) The qubit to apply the gate to.
  void tdg(const uint_t qubit);

  /// @brief Apply UGate
  /// @param (theta) The theta rotation angle of the gate.
  /// @param (phi) The phi rotation angle of the gate.
  /// @param (lam) The lam rotation angle of the gate.
  /// @param (qubit) The qubit to apply the gate to.
  void u(const double theta, const double phi, const double lam, const uint_t qubit);

  /// @brief Apply UGate
  /// @param (theta) The theta rotation angle of the gate.
  /// @param (phi) The phi rotation angle of the gate.
  /// @param (lam) The lam rotation angle of the gate.
  /// @param (qubit) The qubit to apply the gate to.
  void u(const Parameter& theta, const Parameter& phi, const Parameter& lam, const uint_t qubit);

  /// @brief Apply U1Gate
  /// @param (theta) The theta rotation angle of the gate.
  void u1(const double theta, const uint_t qubit);

  /// @brief Apply U1Gate
  /// @param (theta) The theta rotation angle of the gate.
  /// @param (qubit) The qubit to apply the gate to.
  void u1(const Parameter& theta, const uint_t qubit);

  /// @brief Apply U2Gate
  /// @param (phi) The phi rotation angle of the gate.
  /// @param (lam) The lam rotation angle of the gate.
  /// @param (qubit) The qubit to apply the gate to.
  void u2(const double phi, const double lam, const uint_t qubit);

  /// @brief Apply U2Gate
  /// @param (phi) The phi rotation angle of the gate.
  /// @param (lam) The lam rotation angle of the gate.
  /// @param (qubit) The qubit to apply the gate to.
  void u2(const Parameter& phi, const Parameter& lam, const uint_t qubit);

  /// @brief Apply U3Gate
  /// @param (theta) The theta rotation angle of the gate.
  /// @param (phi) The phi rotation angle of the gate.
  /// @param (lam) The lam rotation angle of the gate.
  /// @param (qubit) The qubit to apply the gate to.
  void u3(const double theta, const double phi, const double lam, const uint_t qubit);

  /// @brief Apply U3Gate
  /// @param (theta) The theta rotation angle of the gate.
  /// @param (phi) The phi rotation angle of the gate.
  /// @param (lam) The lam rotation angle of the gate.
  /// @param (qubit) The qubit to apply the gate to.
  void u3(const Parameter& theta, const Parameter& phi, const Parameter& lam, const uint_t qubit);

  /// @brief Apply unitary gate specified by unitary to qubits
  /// @param (unitary) Unitary operator
  /// @param (qubits) The circuit qubits to apply the transformation to
  void unitary(const std::vector<complex_t>& unitary, const reg_t& qubits);

  /// @brief Apply CHGate
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void ch(const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply CXGate
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void cx(const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply CYGate
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void cy(const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply CZGate
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void cz(const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply DCXGate
  /// @param (qubit1) The qubit to apply the gate to
  /// @param (qubit2) The qubit to apply the gate to
  void dcx(const uint_t qubit1, const uint_t qubit2);

  /// @brief Apply ECRGate
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void ecr(const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply SwapGate
  /// @param (qubit1) The qubit to apply the gate to
  /// @param (qubit2) The qubit to apply the gate to
  void swap(const uint_t qubit1, const uint_t qubit2);

  /// @brief Apply iSwapGate
  /// @param (qubit1) The qubit to apply the gate to
  /// @param (qubit2) The qubit to apply the gate to
  void iswap(const uint_t qubit1, const uint_t qubit2);

  /// @brief Apply controlled PhaseGate
  /// @param (phase) Phase.
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void cp(const double phase, const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply controlled PhaseGate
  /// @param (phase) Phase.
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void cp(const Parameter& phase, const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply controlled RXGate
  /// @param (theta) The angle of the rotation
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void crx(const double theta, const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply controlled RXGate
  /// @param (theta) The angle of the rotation
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void crx(const Parameter &theta, const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply controlled RYGate
  /// @param (theta) The angle of the rotation
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void cry(const double theta, const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply controlled RYGate
  /// @param (theta) The angle of the rotation
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void cry(const Parameter &theta, const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply controlled RZGate
  /// @param (theta) The angle of the rotation
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void crz(const double theta, const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply controlled RZGate
  /// @param (theta) The angle of the rotation
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void crz(const Parameter &theta, const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply CSGate
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void cs(const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply CSdgGate
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void csdg(const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply CSXGate
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void csx(const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply CUGate
  /// @param (theta) The theta rotation angle of the gate.
  /// @param (phi) The phi rotation angle of the gate.
  /// @param (lam) The lam rotation angle of the gate.
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void cu(const double theta, const double phi, const double lam, const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply CUGate
  /// @param (theta) The theta rotation angle of the gate.
  /// @param (phi) The phi rotation angle of the gate.
  /// @param (lam) The lam rotation angle of the gate.
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void cu(const Parameter& theta, const Parameter& phi, const Parameter& lam, const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply CU1Gate
  /// @param (theta) The theta rotation angle of the gate.
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void cu1(const double theta, const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply CU1Gate
  /// @param (theta) The theta rotation angle of the gate.
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void cu1(const Parameter& theta, const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply CU3Gate
  /// @param (theta) The theta rotation angle of the gate.
  /// @param (phi) The phi rotation angle of the gate.
  /// @param (lam) The lam rotation angle of the gate.
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void cu3(const double theta, const double phi, const double lam, const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply CU3Gate
  /// @param (theta) The theta rotation angle of the gate.
  /// @param (phi) The phi rotation angle of the gate.
  /// @param (lam) The lam rotation angle of the gate.
  /// @param (cqubit) The qubit used as the control
  /// @param (tqubit) The qubit targeted by the gate
  void cu3(const Parameter& theta, const Parameter& phi, const Parameter& lam, const uint_t cqubit, const uint_t tqubit);

  /// @brief Apply RXXGate
  /// @param (theta) The rotation angle of the gate
  /// @param (qubit1) The qubit to apply the gate to
  /// @param (qubit2) The qubit to apply the gate to
  void rxx(const double theta, const uint_t qubit1, const uint_t qubit2);

  /// @brief Apply RXXGate
  /// @param (theta) The rotation angle of the gate
  /// @param (qubit1) The qubit to apply the gate to
  /// @param (qubit2) The qubit to apply the gate to
  void rxx(const Parameter &theta, const uint_t qubit1, const uint_t qubit2);

  /// @brief Apply RYYGate
  /// @param (theta) The rotation angle of the gate
  /// @param (qubit1) The qubit to apply the gate to
  /// @param (qubit2) The qubit to apply the gate to
  void ryy(const double theta, const uint_t qubit1, const uint_t qubit2);

  /// @brief Apply RYYGate
  /// @param (theta) The rotation angle of the gate
  /// @param (qubit1) The qubit to apply the gate to
  /// @param (qubit2) The qubit to apply the gate to
  void ryy(const Parameter &theta, const uint_t qubit1, const uint_t qubit2);

  /// @brief Apply RZZGate
  /// @param (theta) The rotation angle of the gate
  /// @param (qubit1) The qubit to apply the gate to
  /// @param (qubit2) The qubit to apply the gate to
  void rzz(const double theta, const uint_t qubit1, const uint_t qubit2);

  /// @brief Apply RZZGate
  /// @param (theta) The rotation angle of the gate
  /// @param (qubit1) The qubit to apply the gate to
  /// @param (qubit2) The qubit to apply the gate to
  void rzz(const Parameter &theta, const uint_t qubit1, const uint_t qubit2);

  /// @brief Apply RZXGate
  /// @param (theta) The rotation angle of the gate
  /// @param (qubit1) The qubit to apply the gate to
  /// @param (qubit2) The qubit to apply the gate to
  void rzx(const double theta, const uint_t qubit1, const uint_t qubit2);

  /// @brief Apply RZXGate
  /// @param (theta) The rotation angle of the gate
  /// @param (qubit1) The qubit to apply the gate to
  /// @param (qubit2) The qubit to apply the gate to
  void rzx(const Parameter &theta, const uint_t qubit1, const uint_t qubit2);

  /// @brief Apply XXminusYY
  /// @param (theta) The rotation angle of the gate
  /// @param (beta) The phase angle of the gate
  /// @param (qubit1) The qubit to apply the gate to
  /// @param (qubit2) The qubit to apply the gate to
  void xx_minus_yy(const double theta, const double beta, const uint_t qubit1, const uint_t qubit2);

  /// @brief Apply XXminusYY
  /// @param (theta) The rotation angle of the gate
  /// @param (beta) The phase angle of the gate
  /// @param (qubit1) The qubit to apply the gate to
  /// @param (qubit2) The qubit to apply the gate to
  void xx_minus_yy(const Parameter &theta, const Parameter &beta, const uint_t qubit1, const uint_t qubit2);

  /// @brief Apply XXplusYY
  /// @param (theta) The rotation angle of the gate
  /// @param (beta) The phase angle of the gate
  /// @param (qubit1) The qubit to apply the gate to
  /// @param (qubit2) The qubit to apply the gate to
  void xx_plus_yy(const double theta, const double beta, const uint_t qubit1, const uint_t qubit2);

  /// @brief Apply XXplusYY
  /// @param (theta) The rotation angle of the gate
  /// @param (beta) The phase angle of the gate
  /// @param (qubit1) The qubit to apply the gate to
  /// @param (qubit2) The qubit to apply the gate to
  void xx_plus_yy(const Parameter &theta, const Parameter &beta, const uint_t qubit1, const uint_t qubit2);

  /// @brief Apply CCXGate
  /// @param (cqubit1) The qubit used as the first control
  /// @param (cqubit2) The qubit used as the second control
  /// @param (tqubit) The qubit targeted by the gate
  void ccx(const uint_t cqubit1, const uint_t cqubit2, const uint_t tqubit);

  /// @brief Apply CCZGate
  /// @param (cqubit1) The qubit used as the first control
  /// @param (cqubit2) The qubit used as the second control
  /// @param (tqubit) The qubit targeted by the gate
  void ccz(const uint_t cqubit1, const uint_t cqubit2, const uint_t tqubit);

  /// @brief Apply CSwapGate
  /// @param (cqubit) The qubit used as the control
  /// @param (qubit1) The qubit to apply the gate to
  /// @param (qubit2) The qubit to apply the gate to
  void cswap(const uint_t cqubit, const uint_t qubit1, const uint_t qubit2);

  /// @brief Apply RCCXGate
  /// @param (cqubit1) The qubit used as the first control
  /// @param (cqubit2) The qubit used as the second control
  /// @param (tqubit) The qubit targeted by the gate
  void rccx(const uint_t cqubit1, const uint_t cqubit2, const uint_t tqubit);

  /// @brief Apply C3XGate
  /// @param (cqubit1) The qubit used as the first control
  /// @param (cqubit2) The qubit used as the second control
  /// @param (cqubit3) The qubit used as the third control
  /// @param (tqubit) The qubit targeted by the gate
  void cccx(const uint_t cqubit1, const uint_t cqubit2, const uint_t cqubit3, const uint_t tqubit);

  /// @brief Apply C3SXGate
  /// @param (cqubit1) The qubit used as the first control
  /// @param (cqubit2) The qubit used as the second control
  /// @param (cqubit3) The qubit used as the third control
  /// @param (tqubit) The qubit targeted by the gate
  void cccsx(const uint_t cqubit1, const uint_t cqubit2, const uint_t cqubit3, const uint_t tqubit);

  /// @brief Apply RC3XGate
  /// @param (cqubit1) The qubit used as the first control
  /// @param (cqubit2) The qubit used as the second control
  /// @param (cqubit3) The qubit used as the third control
  /// @param (tqubit) The qubit targeted by the gate
  void rcccx(const uint_t cqubit1, const uint_t cqubit2, const uint_t cqubit3, const uint_t tqubit);

  // other operations

  /// @brief Measure a quantum bit(qubit) in the Z basis into a classical bit(cbit)
  /// @param (qubit) The qubit to measure
  /// @param (cbit) The classical bit to place the measurement result in.
  void measure(const uint_t qubit, const uint_t cbit);

  /// @brief Measure a quantum bit(qreg) in the Z basis into a classical bit(creg)
  /// @param (qreg) The qubit to measure
  /// @param (creg) The classical bit to place the measurement result in.
  void measure(QuantumRegister& qreg, ClassicalRegister& creg);

  /// @brief Reset the quantum bit to their default state
  /// @param (qubit) The qubit to reset
  void reset(const uint_t qubit);

  /// @brief Reset the quantum bit to their default state
  /// @param (qreg) The qubit to reset
  void reset(QuantumRegister& qreg);

  /// @brief Insert barrier on specified qubit
  /// @param (qubit) The qubit to put a barrier
  void barrier(const uint_t qubit);

  /// @brief Insert barrier on multiple qubits
  /// @param (qubits) The qubits to put barrier
  void barrier(const reg_t& qubits);

  // control flow ops

  /// @brief IfElseOp with only a True body
  /// @param (clbit)
  /// @param (value)
  /// @param (body) The circuit body to be run if condition is true.
  IfElseOp& if_test(const uint32_t clbit, const uint32_t value, const std::function<void(QuantumCircuit&)> body);
//  IfElseOp& if_test(Expr expr, std::function<void(qiskitcpp::circuit::QuantumCircuit&)> body);

  // parameter binding

  /// @brief Return the number of parameter objects in the circuit
  /// @return the number of parameter objects in the circuit
  uint_t num_parameters(void) const;

  /// @brief Assign parameters to new parameters or values.
  /// @param (keys) a list of keys
  /// @param (values) a list of values
  void assign_parameters(const std::vector<std::string> keys, const std::vector<double> values);

  /// @brief Assign parameter to new parameter or value.
  /// @param (key) key
  /// @param (value) value
  void assign_parameter(const std::string key, const double val);

  // compose
  QuantumCircuit& operator+=(QuantumCircuit& rhs);
  QuantumCircuit operator+(QuantumCircuit& rhs);
  void compose(QuantumCircuit& circ);
  void compose(QuantumCircuit& circ, const reg_t& qubits, const reg_t& clbits);

  template<typename Operator>
  void append(const Operator& op, const reg_t& qubits);

  // qasm3

  /// @brief Serialize a QuantumCircuit object as an OpenQASM 3 string.
  /// @return An OpenQASM 3 string.
  std::string to_qasm3(void);


  // print circuit
  void print(void) const;

protected:
  void add_pending_control_flow_op(void);

  void pre_add_gate(void)
  {
    add_pending_control_flow_op();
  }

  void get_qubits(reg_t& bits);
  void get_clbits(reg_t& bits);
};



// mapping of gate string and QkGate
std::unordered_map<std::string, QkGate> QuantumCircuit::gate_map_{
  {"global_phase", QkGate_GlobalPhase},
  {"h", QkGate_H},
  {"i", QkGate_I},
  {"x", QkGate_X},
  {"y", QkGate_Y},
  {"z", QkGate_Z},
  {"p", QkGate_Phase},
  {"r", QkGate_R},
  {"rx", QkGate_RX},
  {"ry", QkGate_RY},
  {"rz", QkGate_RZ},
  {"s", QkGate_S},
  {"sdg", QkGate_Sdg},
  {"sx", QkGate_SX},
  {"sxdg", QkGate_SXdg},
  {"t", QkGate_T},
  {"tdg", QkGate_Tdg},
  {"u", QkGate_U},
  {"u1", QkGate_U1},
  {"u2", QkGate_U2},
  {"u3", QkGate_U3},
  {"ch", QkGate_CH},
  {"cx", QkGate_CX},
  {"cy", QkGate_CY},
  {"cz", QkGate_CZ},
  {"dcx", QkGate_DCX},
  {"ecr", QkGate_ECR},
  {"swap",QkGate_Swap},
  {"iswap", QkGate_ISwap},
  {"cp", QkGate_CPhase},
  {"crx", QkGate_CRX},
  {"cry", QkGate_CRY},
  {"crz", QkGate_CRZ},
  {"cs", QkGate_CS},
  {"csdg", QkGate_CSdg},
  {"csx", QkGate_CSX},
  {"cu", QkGate_CU},
  {"cu1", QkGate_CU1},
  {"cu3", QkGate_CU3},
  {"rxx", QkGate_RXX},
  {"ryy", QkGate_RYY},
  {"rzz", QkGate_RZZ},
  {"rzx", QkGate_RZX},
  {"xx_minus_yy", QkGate_XXMinusYY},
  {"xx_plus_yy", QkGate_XXPlusYY},
  {"ccx", QkGate_CCX},
  {"ccz", QkGate_CCZ},
  {"cswap", QkGate_CSwap},
  {"rccx", QkGate_RCCX},
  {"mcx", QkGate_C3X},
  {"c3sx", QkGate_C3SX},
  {"rcccx", QkGate_RC3X},
};


} // namespace circuit
} // namespace Qiskit


#endif  // __qiskitcpp_circuit_quantum_circuit_def_hpp__

