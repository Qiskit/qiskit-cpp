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

// Quantum circuit class implementations


#ifndef __qiskitcpp_circuit_quantum_circuit_impl_hpp__
#define __qiskitcpp_circuit_quantum_circuit_impl_hpp__

#include <memory>
#include <functional>

#include "controlflow/__init__.hpp"

namespace qiskitcpp {
namespace circuit {


QuantumCircuit::QuantumCircuit(const uint_t num_qubits, const uint_t num_clbits, const double global_phase)
{
  num_qubits_ = num_qubits;
  num_clbits_ = num_clbits;
  global_phase_ = global_phase;

  qregs_.resize(1);
  cregs_.resize(1);

  QuantumRegister qr(num_qubits_);
  ClassicalRegister cr(num_clbits_);
  qregs_[0] = qr;
  cregs_[0] = cr;

  rust_circuit_ = qk_circuit_new((std::uint32_t)num_qubits_, (std::uint32_t)num_clbits_);
  qk_circuit_add_quantum_register(rust_circuit_, qregs_[0].get_register());
  qk_circuit_add_classical_register(rust_circuit_, cregs_[0].get_register());

  if (global_phase != 0.0) {
    qk_circuit_gate(rust_circuit_, QkGate_GlobalPhase, nullptr, &global_phase_);
  }
}

QuantumCircuit::QuantumCircuit(QuantumRegister& qreg, ClassicalRegister& creg, const double global_phase)
{
  num_qubits_ = qreg.size();
  num_clbits_ = creg.size();
  global_phase_ = global_phase;

  qregs_.resize(1);
  cregs_.resize(1);

  qregs_[0] = qreg;
  cregs_[0] = creg;

  rust_circuit_ = qk_circuit_new((std::uint32_t)num_qubits_, (std::uint32_t)num_clbits_);

  qk_circuit_add_quantum_register(rust_circuit_, qregs_[0].get_register());
  qk_circuit_add_classical_register(rust_circuit_, cregs_[0].get_register());

  if (global_phase != 0.0) {
    qk_circuit_gate(rust_circuit_, QkGate_GlobalPhase, nullptr, &global_phase_);
  }
}

QuantumCircuit::QuantumCircuit(std::vector<QuantumRegister>& qregs, std::vector<ClassicalRegister>& cregs, const double global_phase)
{
  num_qubits_ = 0;
  num_clbits_ = 0;
  global_phase_ = global_phase;

  qregs_.resize(qregs.size());
  cregs_.resize(cregs.size());

  for(int_t i=0;i<qregs.size();i++){
    qregs_[i] = qregs[i];
    qregs[i][0].get_register()->set_base_index(num_qubits_);
    qregs_[i].set_base_index(num_qubits_);
    num_qubits_ += qregs[i].size();
  }

  for(int_t i=0;i<cregs.size();i++){
    cregs_[i] = cregs[i];
    cregs[i][0].get_register()->set_base_index(num_clbits_);
    cregs_[i].set_base_index(num_clbits_);
    num_clbits_ += cregs[i].size();
  }

  rust_circuit_ = qk_circuit_new((std::uint32_t)num_qubits_, (std::uint32_t)num_clbits_);

  for(int_t i=0;i<qregs_.size();i++){
    qk_circuit_add_quantum_register(rust_circuit_, qregs_[i].get_register());
  }
  for(int_t i=0;i<cregs_.size();i++){
    qk_circuit_add_classical_register(rust_circuit_, cregs_[i].get_register());
  }

  if (global_phase != 0.0) {
    qk_circuit_gate(rust_circuit_, QkGate_GlobalPhase, nullptr, &global_phase_);
  }
}

// create new circuit with same number of qubits
QuantumCircuit::QuantumCircuit(const QuantumCircuit& circ)
{
  num_qubits_ = circ.num_qubits_;
  num_clbits_ = circ.num_clbits_;
  global_phase_ = circ.global_phase_;

  qregs_ = circ.qregs_;
  cregs_ = circ.cregs_;

  rust_circuit_ = qk_circuit_copy(circ.rust_circuit_);
  for(int_t i=0;i<qregs_.size();i++){
    qk_circuit_add_quantum_register(rust_circuit_, qregs_[i].get_register());
  }
  for(int_t i=0;i<cregs_.size();i++){
    qk_circuit_add_classical_register(rust_circuit_, cregs_[i].get_register());
  }
}

void QuantumCircuit::from_rust_circuit(rust_circuit* circ)
{
  if (rust_circuit_)
    qk_circuit_free(rust_circuit_);
  rust_circuit_ = circ;
  num_qubits_ = qk_circuit_num_qubits(circ);
  num_clbits_ = qk_circuit_num_clbits(circ);

  qregs_.resize(1);
  cregs_.resize(1);

  qregs_[0].resize(num_qubits_);
  cregs_[0].resize(num_clbits_);
  qk_circuit_add_quantum_register(rust_circuit_, qregs_[0].get_register());
  qk_circuit_add_classical_register(rust_circuit_, cregs_[0].get_register());
}

QuantumCircuit::~QuantumCircuit()
{
  if (pending_control_flow_op_){
    pending_control_flow_op_.reset();
  }

  if (rust_circuit_){
    qk_circuit_free(rust_circuit_);
  }
}

void QuantumCircuit::get_qubits(reg_t& bits)
{
  bits.clear();
  bits.reserve(num_qubits_);
  for (int_t i=0;i<qregs_.size();i++){
    for (int_t j=0;j<qregs_[i].size();j++){
      bits.push_back(qregs_[i][j]);
    }
  }
}

void QuantumCircuit::get_clbits(reg_t& bits)
{
  bits.clear();
  bits.reserve(num_clbits_);
  for (int_t i=0;i<cregs_.size();i++){
    for (int_t j=0;j<cregs_[i].size();j++){
      bits.push_back(cregs_[i][j]);
    }
  }
}

void QuantumCircuit::global_phase(const double phase)
{
  global_phase_ = phase;
  qk_circuit_gate(rust_circuit_, QkGate_GlobalPhase, nullptr, &global_phase_);
}

void QuantumCircuit::h(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_H, qubits, nullptr);
}

void QuantumCircuit::i(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_I, qubits, nullptr);
}

void QuantumCircuit::x(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_X, qubits, nullptr);
}

void QuantumCircuit::y(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_Y, qubits, nullptr);
}

void QuantumCircuit::z(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_Z, qubits, nullptr);
}

void QuantumCircuit::p(const double phase, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_Phase, qubits, &phase);
}

void QuantumCircuit::p(const Parameter &phase, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
//  qc_p(rust_circuit_, theta.expr_, qubit);
}

void QuantumCircuit::r(const double theta, const double phi, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  double params[] = {theta, phi};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_R, qubits, params);
}

void QuantumCircuit::r(const Parameter &theta, const Parameter &phi, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
//  qc_r(rust_circuit_, theta.expr_, phi.expr_, qubit);
}

void QuantumCircuit::rx(const double theta, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_RX, qubits, &theta);
}

void QuantumCircuit::rx(const Parameter &theta, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_rx(rust_circuit_, theta.expr_, qubit);
}

void QuantumCircuit::ry(const double theta, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_RY, qubits, &theta);
}

void QuantumCircuit::ry(const Parameter &theta, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_ry(rust_circuit_, theta.expr_, qubit);
}

void QuantumCircuit::rz(const double theta, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_RZ, qubits, &theta);
}

void QuantumCircuit::rz(const Parameter &theta, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_rz(rust_circuit_, theta.expr_, qubit);
}

void QuantumCircuit::s(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_S, qubits, nullptr);
}

void QuantumCircuit::sdg(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_Sdg, qubits, nullptr);
}

void QuantumCircuit::sx(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_SX, qubits, nullptr);
}

void QuantumCircuit::sxdg(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_SXdg, qubits, nullptr);
}

void QuantumCircuit::t(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_T, qubits, nullptr);
}

void QuantumCircuit::tdg(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_Tdg, qubits, nullptr);
}

void QuantumCircuit::u(const double theta, const double phi, const double lam, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  double params[] = {theta, phi, lam};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_U, qubits, params);
}

void QuantumCircuit::u(const Parameter &theta, const Parameter &phi, const Parameter &lam, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_u(rust_circuit_, theta.expr_, phi.expr_, lam.expr_, qubit);
}

void QuantumCircuit::u1(const double theta, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  double params[] = {theta};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_U1, qubits, params);
}

void QuantumCircuit::u1(const Parameter &theta, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_u(rust_circuit_, theta.expr_, phi.expr_, lam.expr_, qubit);
}

void QuantumCircuit::u2(const double phi, const double lam, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  double params[] = {phi, lam};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_U2, qubits, params);
}

void QuantumCircuit::u2(const Parameter &phi, const Parameter &lam, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_u(rust_circuit_, theta.expr_, phi.expr_, lam.expr_, qubit);
}

void QuantumCircuit::u3(const double theta, const double phi, const double lam, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  double params[] = {theta, phi, lam};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_U3, qubits, params);
}

void QuantumCircuit::u3(const Parameter &theta, const Parameter &phi, const Parameter &lam, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_u(rust_circuit_, theta.expr_, phi.expr_, lam.expr_, qubit);
}

void QuantumCircuit::unitary(const std::vector<complex_t>& unitary, const reg_t& qubits)
{
  pre_add_gate();
  std::vector<std::uint32_t> qubits32(qubits.size());
  for (int_t i=0; i < qubits.size(); i++)
    qubits32[i] = (std::uint32_t)qubits[i];

  qk_circuit_unitary(rust_circuit_, (const QkComplex64*)unitary.data(), qubits32.data(), (std::uint32_t)qubits.size(), true);
}

void QuantumCircuit::ch(const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CH, qubits, nullptr);
}

void QuantumCircuit::cx(const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CX, qubits, nullptr);
}

void QuantumCircuit::cy(const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CY, qubits, nullptr);
}

void QuantumCircuit::cz(const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CZ, qubits, nullptr);
}

void QuantumCircuit::dcx(const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_DCX, qubits, nullptr);
}

void QuantumCircuit::ecr(const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_ECR, qubits, nullptr);
}

void QuantumCircuit::swap(const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_Swap, qubits, nullptr);
}

void QuantumCircuit::iswap(const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_ISwap, qubits, nullptr);
}

void QuantumCircuit::cp(const double phase, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CPhase, qubits, &phase);
}

void QuantumCircuit::cp(const Parameter &phase, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  //TO DO support parameter expression
//  qc_p(rust_circuit_, theta.expr_, qubit);
}

void QuantumCircuit::crx(const double theta, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CRX, qubits, &theta);
}

void QuantumCircuit::crx(const Parameter &theta, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  //TO DO support parameter expression
//  qc_p(rust_circuit_, theta.expr_, qubit);
}

void QuantumCircuit::cry(const double theta, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CRY, qubits, &theta);
}

void QuantumCircuit::cry(const Parameter &theta, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  //TO DO support parameter expression
//  qc_p(rust_circuit_, theta.expr_, qubit);
}

void QuantumCircuit::crz(const double theta, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CRZ, qubits, &theta);
}

void QuantumCircuit::crz(const Parameter &theta, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  //TO DO support parameter expression
//  qc_p(rust_circuit_, theta.expr_, qubit);
}

void QuantumCircuit::cs(const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CS, qubits, nullptr);
}

void QuantumCircuit::csdg(const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CSdg, qubits, nullptr);
}

void QuantumCircuit::csx(const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CSX, qubits, nullptr);
}

void QuantumCircuit::cu(const double theta, const double phi, const double lam, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  double params[] = {theta, phi, lam};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CU, qubits, params);
}

void QuantumCircuit::cu(const Parameter &theta, const Parameter &phi, const Parameter &lam, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_u(rust_circuit_, theta.expr_, phi.expr_, lam.expr_, qubit);
}

void QuantumCircuit::cu1(const double theta, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  double params[] = {theta};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CU1, qubits, params);
}

void QuantumCircuit::cu1(const Parameter &theta, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_u(rust_circuit_, theta.expr_, phi.expr_, lam.expr_, qubit);
}

void QuantumCircuit::cu3(const double theta, const double phi, const double lam, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  double params[] = {theta, phi, lam};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CU3, qubits, params);
}

void QuantumCircuit::cu3(const Parameter &theta, const Parameter &phi, const Parameter &lam, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_u(rust_circuit_, theta.expr_, phi.expr_, lam.expr_, qubit);
}

void QuantumCircuit::rxx(const double theta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_RXX, qubits, &theta);
}

void QuantumCircuit::rxx(const Parameter& theta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_rzz(rust_circuit_, theta.expr_, qubit1, qubit2);
}

void QuantumCircuit::ryy(const double theta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_RYY, qubits, &theta);
}

void QuantumCircuit::ryy(const Parameter& theta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_rzz(rust_circuit_, theta.expr_, qubit1, qubit2);
}

void QuantumCircuit::rzz(const double theta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_RZZ, qubits, &theta);
}

void QuantumCircuit::rzz(const Parameter& theta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_rzz(rust_circuit_, theta.expr_, qubit1, qubit2);
}

void QuantumCircuit::rzx(const double theta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_RZX, qubits, &theta);
}

void QuantumCircuit::rzx(const Parameter& theta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_rzz(rust_circuit_, theta.expr_, qubit1, qubit2);
}

void QuantumCircuit::xx_minus_yy(const double theta, const double beta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  double params[] = {theta, beta};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_XXMinusYY, qubits, params);
}

void QuantumCircuit::xx_minus_yy(const Parameter &theta, const Parameter &beta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  //TO DO support parameter expression
}

void QuantumCircuit::xx_plus_yy(const double theta, const double beta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  double params[] = {theta, beta};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_XXPlusYY, qubits, params);
}

void QuantumCircuit::xx_plus_yy(const Parameter &theta, const Parameter &beta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  //TO DO support parameter expression
}

void QuantumCircuit::ccx(const uint_t cqubit1, const uint_t cqubit2, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CCX, qubits, nullptr);
}

void QuantumCircuit::ccz(const uint_t cqubit1, const uint_t cqubit2, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CCZ, qubits, nullptr);
}

void QuantumCircuit::cswap(const uint_t cqubit, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_CSwap, qubits, nullptr);
}

void QuantumCircuit::rccx(const uint_t cqubit1, const uint_t cqubit2, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_RCCX, qubits, nullptr);
}

void QuantumCircuit::cccx(const uint_t cqubit1, const uint_t cqubit2, const uint_t cqubit3, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)cqubit3, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_C3X, qubits, nullptr);
}

void QuantumCircuit::cccsx(const uint_t cqubit1, const uint_t cqubit2, const uint_t cqubit3, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)cqubit3, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_C3SX, qubits, nullptr);
}

void QuantumCircuit::rcccx(const uint_t cqubit1, const uint_t cqubit2, const uint_t cqubit3, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)cqubit3, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_, QkGate_RC3X, qubits, nullptr);
}

void QuantumCircuit::measure(const uint_t qubit, const uint_t cbit)
{
  pre_add_gate();
  qk_circuit_measure(rust_circuit_, (std::uint32_t)qubit, (std::uint32_t)cbit);
}

void QuantumCircuit::measure(QuantumRegister& qreg, ClassicalRegister& creg)
{
  pre_add_gate();
  int size = qreg.size();
  if (size > creg.size())
    size = creg.size();
  // TO DO implement multi-bits measure in C-API
  for (int_t i = 0; i< size; i++) {
    qk_circuit_measure(rust_circuit_, (std::uint32_t)qreg[i], (std::uint32_t)creg[i]);
  }
}

void QuantumCircuit::reset(const uint_t qubit)
{
  pre_add_gate();
  qk_circuit_reset(rust_circuit_, (std::uint32_t)qubit);
}

void QuantumCircuit::reset(QuantumRegister& qreg)
{
  pre_add_gate();
  // TO DO implement multi-bits rest in C-API
  for (int_t i = 0; i< qreg.size(); i++) {
    qk_circuit_reset(rust_circuit_, (std::uint32_t)qreg[i]);
  }
}

void QuantumCircuit::barrier(const uint_t qubit) {
  pre_add_gate();
  std::uint32_t q = (std::uint32_t)qubit;
  qk_circuit_barrier(rust_circuit_, &q, 1);
}

void QuantumCircuit::barrier(const reg_t& qubits) {
  pre_add_gate();
  std::vector<std::uint32_t> qubits32(qubits.size());
  for (int_t i=0;i< qubits.size(); i++) {
    qubits32[i] = (std::uint32_t)qubits[i];
  }
  qk_circuit_barrier(rust_circuit_, qubits32.data(), qubits32.size());
}


void QuantumCircuit::add_pending_control_flow_op(void)
{
  if (pending_control_flow_op_){
    pending_control_flow_op_->add_control_flow_op(*this);
    pending_control_flow_op_.reset();
    pending_control_flow_op_ = nullptr;
  }
}


IfElseOp& QuantumCircuit::if_test(const std::uint32_t clbit, const std::uint32_t value, const std::function<void(QuantumCircuit&)> body)
{
  pre_add_gate();

  auto op = std::make_shared<IfElseOp>(*this, clbit, value);
  pending_control_flow_op_ = op;

  body(op->true_body());

  return *op;
}

uint_t QuantumCircuit::num_parameters(void) const
{
  // TO DO : add support for Parameter in Qiskit C-API
  return 0;//qc_num_parameters(rust_circuit_);
}


void QuantumCircuit::assign_parameters(const std::vector<std::string> keys, const std::vector<double> values)
{
  std::vector<char*> c_keys;
  for (int_t i = 0; i < keys.size(); i++) {
    c_keys.push_back((char*)keys[i].c_str());
  }
  add_pending_control_flow_op();

  // TO DO : add support for Parameter in Qiskit C-API
  //qc_assign_parameters(rust_circuit_, c_keys.data(), c_keys.size(), values.data(), values.size());
}

void QuantumCircuit::assign_parameter(const std::string key, const double val)
{
  add_pending_control_flow_op();

  // TO DO : add support for Parameter in Qiskit C-API
  //qc_assign_parameter(rust_circuit_, (char*)key.c_str(), val);
}

QuantumCircuit& QuantumCircuit::operator+=(QuantumCircuit& rhs)
{
  compose(rhs);
  return *this;
}

QuantumCircuit QuantumCircuit::operator+(QuantumCircuit& rhs)
{
  QuantumCircuit new_circ(*this);
  if(num_qubits_ >= rhs.num_qubits_ && num_clbits_ >= rhs.num_clbits_) {
    reg_t qubits;
    reg_t clbits;
    rhs.get_qubits(qubits);
    rhs.get_clbits(clbits);
    uint_t size = std::min(qubits.size(), clbits.size());
    std::vector<std::uint32_t> vqubits(size);
    for (int_t i = 0; i< size; i++) {
      vqubits[i] = (std::uint32_t)qubits[i];
    }
    std::vector<std::uint32_t> vclbits(size);
    for (int_t i = 0; i< size; i++) {
      vclbits[i] = (std::uint32_t)clbits[i];
    }
    new_circ.compose(rhs);
  }
  return new_circ;
}

void QuantumCircuit::compose(QuantumCircuit& rhs)
{
  if(num_qubits_ >= rhs.num_qubits_ && num_clbits_ >= rhs.num_clbits_) {
    reg_t qubits;
    reg_t clbits;
    rhs.get_qubits(qubits);
    rhs.get_clbits(clbits);
    compose(rhs, qubits, clbits);
  }
}

void QuantumCircuit::compose(QuantumCircuit& circ, const reg_t& qubits, const reg_t& clbits)
{
  pre_add_gate();
  uint_t size = std::min(qubits.size(), clbits.size());
  std::vector<std::uint32_t> vqubits(size);
  for (int_t i = 0; i< size; i++) {
    vqubits[i] = (std::uint32_t)qubits[i];
  }
  std::vector<std::uint32_t> vclbits(size);
  for (int_t i = 0; i< size; i++) {
    vclbits[i] = (std::uint32_t)clbits[i];
  }

  uint_t nops;
  nops = qk_circuit_num_instructions(circ.rust_circuit_);

  for (uint_t i=0;i<nops;i++) {
    QkCircuitInstruction* op = new QkCircuitInstruction;
    qk_circuit_get_instruction(circ.rust_circuit_, i, op);

    std::vector<std::uint32_t> vqubits(op->num_qubits);
    std::vector<std::uint32_t> vclbits;
    for (int j=0;j<op->num_qubits;j++) {
      vqubits[j] = (std::uint32_t)qubits[op->qubits[j]];
    }
    if (op->num_clbits > 0) {
      vclbits.resize(op->num_clbits);
      for (int j=0;j<op->num_clbits;j++) {
        vclbits[j] = (std::uint32_t)clbits[op->clbits[j]];
      }
    }

    if (std::string("reset") == op->name) {
      qk_circuit_reset(rust_circuit_, vqubits[0]);
    } else if (std::string("barrier") == op->name) {
      qk_circuit_barrier(rust_circuit_, vqubits.data(), vqubits.size());
    } else if (std::string("measure") == op->name) {
      qk_circuit_measure(rust_circuit_, vqubits[0], vclbits[0]);
    } else {
      qk_circuit_gate(rust_circuit_, gate_map_[op->name], vqubits.data(), op->params);
    }
    qk_circuit_instruction_clear(op);
  }
}

template<typename Operator>
void QuantumCircuit::append(const Operator& op, const reg_t& qubits) {
  op.append(*this, qubits);
}

std::string QuantumCircuit::to_qasm3(void)
{
  add_pending_control_flow_op();

//  std::string ret = qc_to_qasm3(rust_circuit_);
//  return ret;
  return std::string("");
}


// print circuit
void QuantumCircuit::print(void) const
{
  uint_t nops;
  nops = qk_circuit_num_instructions(rust_circuit_);

  for (uint_t i=0;i<nops;i++) {
    QkCircuitInstruction* op = new QkCircuitInstruction;
    qk_circuit_get_instruction(rust_circuit_, i, op);
    std::cout << op->name;
    if (op->num_qubits > 0) {
      std::cout << "(";
      for (int j=0;j<op->num_qubits;j++) {
        std::cout << op->qubits[j];
        if (j != op->num_qubits - 1)
          std::cout << ", ";
      }
      std::cout << ") ";
    }
    if (op->num_clbits > 0) {
      std::cout << "(";
      for (int j=0;j<op->num_clbits;j++) {
        std::cout << op->clbits[j];
        if (j != op->num_clbits - 1)
          std::cout << ", ";
      }
      std::cout << ") ";
    }
    if (op->num_params > 0) {
      std::cout << "[";
      for (int j=0;j<op->num_params;j++) {
        std::cout << op->params[j];
        if (j != op->num_params - 1)
          std::cout << ", ";
      }
      std::cout << "]";
    }
    std::cout << std::endl;
    qk_circuit_instruction_clear(op);
  }
}



} // namespace circuit
} // namespace qiskitcpp


#endif  // __qiskitcpp_circuit_quantum_circuit_impl_hpp__

