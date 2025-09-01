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
#include <iomanip>
#include <cstring>

#include "controlflow/__init__.hpp"

#include "circuit/barrier.hpp"
#include "circuit/measure.hpp"
#include "circuit/reset.hpp"


namespace Qiskit {
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

  rust_circuit_ = std::shared_ptr<rust_circuit>(qk_circuit_new((std::uint32_t)num_qubits_, (std::uint32_t)num_clbits_), qk_circuit_free);
  qk_circuit_add_quantum_register(rust_circuit_.get(), qregs_[0].get_register().get());
  qk_circuit_add_classical_register(rust_circuit_.get(), cregs_[0].get_register().get());

  if (global_phase != 0.0) {
    qk_circuit_gate(rust_circuit_.get(), QkGate_GlobalPhase, nullptr, &global_phase_);
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

  rust_circuit_ = std::shared_ptr<rust_circuit>(qk_circuit_new((std::uint32_t)num_qubits_, (std::uint32_t)num_clbits_), qk_circuit_free);

  qk_circuit_add_quantum_register(rust_circuit_.get(), qregs_[0].get_register().get());
  qk_circuit_add_classical_register(rust_circuit_.get(), cregs_[0].get_register().get());

  if (global_phase != 0.0) {
    qk_circuit_gate(rust_circuit_.get(), QkGate_GlobalPhase, nullptr, &global_phase_);
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

  rust_circuit_ = std::shared_ptr<rust_circuit>(qk_circuit_new((std::uint32_t)num_qubits_, (std::uint32_t)num_clbits_), qk_circuit_free);

  for(int_t i=0;i<qregs_.size();i++){
    qk_circuit_add_quantum_register(rust_circuit_.get(), qregs_[i].get_register().get());
  }
  for(int_t i=0;i<cregs_.size();i++){
    qk_circuit_add_classical_register(rust_circuit_.get(), cregs_[i].get_register().get());
  }

  if (global_phase != 0.0) {
    qk_circuit_gate(rust_circuit_.get(), QkGate_GlobalPhase, nullptr, &global_phase_);
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

  rust_circuit_ = circ.rust_circuit_;
}

QuantumCircuit QuantumCircuit::copy(void)
{
  QuantumCircuit copied;
  copied.rust_circuit_ = std::shared_ptr<rust_circuit>(qk_circuit_copy(rust_circuit_.get()), qk_circuit_free);

  copied.num_qubits_ = num_qubits_;
  copied.num_clbits_ = num_clbits_;
  copied.global_phase_ = global_phase_;

  copied.qregs_ = qregs_;
  copied.cregs_ = cregs_;

  return copied;
}


void QuantumCircuit::from_rust_circuit(std::shared_ptr<rust_circuit> circ)
{
  if (rust_circuit_)
    rust_circuit_.reset();
  rust_circuit_ = circ;
  num_qubits_ = qk_circuit_num_qubits(circ.get());
  num_clbits_ = qk_circuit_num_clbits(circ.get());

  qregs_.resize(1);
  cregs_.resize(1);

  qregs_[0].resize(num_qubits_);
  cregs_[0].resize(num_clbits_);
}

QuantumCircuit::~QuantumCircuit()
{
  if (pending_control_flow_op_){
    pending_control_flow_op_.reset();
  }

  if (rust_circuit_){
    rust_circuit_.reset();
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
  qk_circuit_gate(rust_circuit_.get(), QkGate_GlobalPhase, nullptr, &global_phase_);
}

void QuantumCircuit::h(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_H, qubits, nullptr);
}

void QuantumCircuit::i(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_I, qubits, nullptr);
}

void QuantumCircuit::x(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_X, qubits, nullptr);
}

void QuantumCircuit::y(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_Y, qubits, nullptr);
}

void QuantumCircuit::z(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_Z, qubits, nullptr);
}

void QuantumCircuit::p(const double phase, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_Phase, qubits, &phase);
}

void QuantumCircuit::p(const Parameter &phase, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
//  qc_p(rust_circuit_.get(), theta.expr_, qubit);
}

void QuantumCircuit::r(const double theta, const double phi, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  double params[] = {theta, phi};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_R, qubits, params);
}

void QuantumCircuit::r(const Parameter &theta, const Parameter &phi, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
//  qc_r(rust_circuit_.get(), theta.expr_, phi.expr_, qubit);
}

void QuantumCircuit::rx(const double theta, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_RX, qubits, &theta);
}

void QuantumCircuit::rx(const Parameter &theta, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_rx(rust_circuit_.get(), theta.expr_, qubit);
}

void QuantumCircuit::ry(const double theta, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_RY, qubits, &theta);
}

void QuantumCircuit::ry(const Parameter &theta, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_ry(rust_circuit_.get(), theta.expr_, qubit);
}

void QuantumCircuit::rz(const double theta, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_RZ, qubits, &theta);
}

void QuantumCircuit::rz(const Parameter &theta, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_rz(rust_circuit_.get(), theta.expr_, qubit);
}

void QuantumCircuit::s(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_S, qubits, nullptr);
}

void QuantumCircuit::sdg(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_Sdg, qubits, nullptr);
}

void QuantumCircuit::sx(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_SX, qubits, nullptr);
}

void QuantumCircuit::sxdg(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_SXdg, qubits, nullptr);
}

void QuantumCircuit::t(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_T, qubits, nullptr);
}

void QuantumCircuit::tdg(const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_Tdg, qubits, nullptr);
}

void QuantumCircuit::u(const double theta, const double phi, const double lam, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  double params[] = {theta, phi, lam};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_U, qubits, params);
}

void QuantumCircuit::u(const Parameter &theta, const Parameter &phi, const Parameter &lam, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_u(rust_circuit_.get(), theta.expr_, phi.expr_, lam.expr_, qubit);
}

void QuantumCircuit::u1(const double theta, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  double params[] = {theta};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_U1, qubits, params);
}

void QuantumCircuit::u1(const Parameter &theta, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_u(rust_circuit_.get(), theta.expr_, phi.expr_, lam.expr_, qubit);
}

void QuantumCircuit::u2(const double phi, const double lam, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  double params[] = {phi, lam};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_U2, qubits, params);
}

void QuantumCircuit::u2(const Parameter &phi, const Parameter &lam, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_u(rust_circuit_.get(), theta.expr_, phi.expr_, lam.expr_, qubit);
}

void QuantumCircuit::u3(const double theta, const double phi, const double lam, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  double params[] = {theta, phi, lam};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_U3, qubits, params);
}

void QuantumCircuit::u3(const Parameter &theta, const Parameter &phi, const Parameter &lam, const uint_t qubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_u(rust_circuit_.get(), theta.expr_, phi.expr_, lam.expr_, qubit);
}

void QuantumCircuit::unitary(const std::vector<complex_t>& unitary, const reg_t& qubits)
{
  pre_add_gate();
  std::vector<std::uint32_t> qubits32(qubits.size());
  for (int_t i=0; i < qubits.size(); i++)
    qubits32[i] = (std::uint32_t)qubits[i];

  qk_circuit_unitary(rust_circuit_.get(), (const QkComplex64*)unitary.data(), qubits32.data(), (std::uint32_t)qubits.size(), true);
}

void QuantumCircuit::ch(const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_CH, qubits, nullptr);
}

void QuantumCircuit::cx(const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_CX, qubits, nullptr);
}

void QuantumCircuit::cy(const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_CY, qubits, nullptr);
}

void QuantumCircuit::cz(const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_CZ, qubits, nullptr);
}

void QuantumCircuit::dcx(const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_DCX, qubits, nullptr);
}

void QuantumCircuit::ecr(const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_ECR, qubits, nullptr);
}

void QuantumCircuit::swap(const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_Swap, qubits, nullptr);
}

void QuantumCircuit::iswap(const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_ISwap, qubits, nullptr);
}

void QuantumCircuit::cp(const double phase, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_CPhase, qubits, &phase);
}

void QuantumCircuit::cp(const Parameter &phase, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  //TO DO support parameter expression
//  qc_p(rust_circuit_.get(), theta.expr_, qubit);
}

void QuantumCircuit::crx(const double theta, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_CRX, qubits, &theta);
}

void QuantumCircuit::crx(const Parameter &theta, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  //TO DO support parameter expression
//  qc_p(rust_circuit_.get(), theta.expr_, qubit);
}

void QuantumCircuit::cry(const double theta, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_CRY, qubits, &theta);
}

void QuantumCircuit::cry(const Parameter &theta, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  //TO DO support parameter expression
//  qc_p(rust_circuit_.get(), theta.expr_, qubit);
}

void QuantumCircuit::crz(const double theta, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_CRZ, qubits, &theta);
}

void QuantumCircuit::crz(const Parameter &theta, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  //TO DO support parameter expression
//  qc_p(rust_circuit_.get(), theta.expr_, qubit);
}

void QuantumCircuit::cs(const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_CS, qubits, nullptr);
}

void QuantumCircuit::csdg(const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_CSdg, qubits, nullptr);
}

void QuantumCircuit::csx(const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_CSX, qubits, nullptr);
}

void QuantumCircuit::cu(const double theta, const double phi, const double lam, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  double params[] = {theta, phi, lam};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_CU, qubits, params);
}

void QuantumCircuit::cu(const Parameter &theta, const Parameter &phi, const Parameter &lam, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_u(rust_circuit_.get(), theta.expr_, phi.expr_, lam.expr_, qubit);
}

void QuantumCircuit::cu1(const double theta, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  double params[] = {theta};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_CU1, qubits, params);
}

void QuantumCircuit::cu1(const Parameter &theta, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_u(rust_circuit_.get(), theta.expr_, phi.expr_, lam.expr_, qubit);
}

void QuantumCircuit::cu3(const double theta, const double phi, const double lam, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  double params[] = {theta, phi, lam};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_CU3, qubits, params);
}

void QuantumCircuit::cu3(const Parameter &theta, const Parameter &phi, const Parameter &lam, const uint_t cqubit, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_u(rust_circuit_.get(), theta.expr_, phi.expr_, lam.expr_, qubit);
}

void QuantumCircuit::rxx(const double theta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_RXX, qubits, &theta);
}

void QuantumCircuit::rxx(const Parameter& theta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_rzz(rust_circuit_.get(), theta.expr_, qubit1, qubit2);
}

void QuantumCircuit::ryy(const double theta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_RYY, qubits, &theta);
}

void QuantumCircuit::ryy(const Parameter& theta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_rzz(rust_circuit_.get(), theta.expr_, qubit1, qubit2);
}

void QuantumCircuit::rzz(const double theta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_RZZ, qubits, &theta);
}

void QuantumCircuit::rzz(const Parameter& theta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_rzz(rust_circuit_.get(), theta.expr_, qubit1, qubit2);
}

void QuantumCircuit::rzx(const double theta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_RZX, qubits, &theta);
}

void QuantumCircuit::rzx(const Parameter& theta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  //TO DO support parameter expression
  //qc_rzz(rust_circuit_.get(), theta.expr_, qubit1, qubit2);
}

void QuantumCircuit::xx_minus_yy(const double theta, const double beta, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
  double params[] = {theta, beta};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_XXMinusYY, qubits, params);
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
  qk_circuit_gate(rust_circuit_.get(), QkGate_XXPlusYY, qubits, params);
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
  qk_circuit_gate(rust_circuit_.get(), QkGate_CCX, qubits, nullptr);
}

void QuantumCircuit::ccz(const uint_t cqubit1, const uint_t cqubit2, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_CCZ, qubits, nullptr);
}

void QuantumCircuit::cswap(const uint_t cqubit, const uint_t qubit1, const uint_t qubit2)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)qubit1, (std::uint32_t)qubit2};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_CSwap, qubits, nullptr);
}

void QuantumCircuit::rccx(const uint_t cqubit1, const uint_t cqubit2, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_RCCX, qubits, nullptr);
}

void QuantumCircuit::cccx(const uint_t cqubit1, const uint_t cqubit2, const uint_t cqubit3, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)cqubit3, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_C3X, qubits, nullptr);
}

void QuantumCircuit::cccsx(const uint_t cqubit1, const uint_t cqubit2, const uint_t cqubit3, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)cqubit3, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_C3SX, qubits, nullptr);
}

void QuantumCircuit::rcccx(const uint_t cqubit1, const uint_t cqubit2, const uint_t cqubit3, const uint_t tqubit)
{
  std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)cqubit3, (std::uint32_t)tqubit};
  pre_add_gate();
  qk_circuit_gate(rust_circuit_.get(), QkGate_RC3X, qubits, nullptr);
}

void QuantumCircuit::measure(const uint_t qubit, const uint_t cbit)
{
  pre_add_gate();
  qk_circuit_measure(rust_circuit_.get(), (std::uint32_t)qubit, (std::uint32_t)cbit);
}

void QuantumCircuit::measure(QuantumRegister& qreg, ClassicalRegister& creg)
{
  pre_add_gate();
  int size = qreg.size();
  if (size > creg.size())
    size = creg.size();
  // TO DO implement multi-bits measure in C-API
  for (int_t i = 0; i< size; i++) {
    qk_circuit_measure(rust_circuit_.get(), (std::uint32_t)qreg[i], (std::uint32_t)creg[i]);
  }
}

void QuantumCircuit::reset(const uint_t qubit)
{
  pre_add_gate();
  qk_circuit_reset(rust_circuit_.get(), (std::uint32_t)qubit);
}

void QuantumCircuit::reset(QuantumRegister& qreg)
{
  pre_add_gate();
  // TO DO implement multi-bits rest in C-API
  for (int_t i = 0; i< qreg.size(); i++) {
    qk_circuit_reset(rust_circuit_.get(), (std::uint32_t)qreg[i]);
  }
}

void QuantumCircuit::barrier(const uint_t qubit) {
  pre_add_gate();
  std::uint32_t q = (std::uint32_t)qubit;
  qk_circuit_barrier(rust_circuit_.get(), &q, 1);
}

void QuantumCircuit::barrier(const reg_t& qubits) {
  pre_add_gate();
  std::vector<std::uint32_t> qubits32(qubits.size());
  for (int_t i=0;i< qubits.size(); i++) {
    qubits32[i] = (std::uint32_t)qubits[i];
  }
  qk_circuit_barrier(rust_circuit_.get(), qubits32.data(), qubits32.size());
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
  //qc_assign_parameters(rust_circuit_.get(), c_keys.data(), c_keys.size(), values.data(), values.size());
}

void QuantumCircuit::assign_parameter(const std::string key, const double val)
{
  add_pending_control_flow_op();

  // TO DO : add support for Parameter in Qiskit C-API
  //qc_assign_parameter(rust_circuit_.get(), (char*)key.c_str(), val);
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
  nops = qk_circuit_num_instructions(circ.rust_circuit_.get());

  auto name_map = get_standard_gate_name_mapping();
  for (uint_t i=0;i<nops;i++) {
    QkCircuitInstruction* op = new QkCircuitInstruction;
    qk_circuit_get_instruction(circ.rust_circuit_.get(), i, op);

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
      qk_circuit_reset(rust_circuit_.get(), vqubits[0]);
    } else if (std::string("barrier") == op->name) {
      qk_circuit_barrier(rust_circuit_.get(), vqubits.data(), vqubits.size());
    } else if (std::string("measure") == op->name) {
      qk_circuit_measure(rust_circuit_.get(), vqubits[0], vclbits[0]);
    } else {
      qk_circuit_gate(rust_circuit_.get(), name_map[op->name].gate_map(), vqubits.data(), op->params);
    }
    qk_circuit_instruction_clear(op);
  }
}

void QuantumCircuit::append(const Instruction& op, const reg_t& qubits, const std::vector<double> params)
{
  if (op.num_qubits() == qubits.size() && op.num_params() == params.size()) {
    std::vector<std::uint32_t> vqubits(qubits.size());
    for (int_t i = 0; i< qubits.size(); i++) {
      vqubits[i] = (std::uint32_t)qubits[i];
    }
    pre_add_gate();
    if (op.is_standard_gate()) {
      if (params.size() > 0)
        qk_circuit_gate(rust_circuit_.get(), op.gate_map(), vqubits.data(), params.data());
      else
        qk_circuit_gate(rust_circuit_.get(), op.gate_map(), vqubits.data(), nullptr);
    } else {
      if (std::string("reset") == op.name()) {
        qk_circuit_reset(rust_circuit_.get(), vqubits[0]);
      } else if (std::string("barrier") == op.name()) {
        qk_circuit_barrier(rust_circuit_.get(), vqubits.data(), vqubits.size());
      } else if (std::string("measure") == op.name()) {
        qk_circuit_measure(rust_circuit_.get(), vqubits[0], vqubits[0]);
      }
    }
  }
}

void QuantumCircuit::append(const Instruction& op, const std::vector<std::uint32_t>& qubits, const std::vector<double> params)
{
  if (op.num_qubits() == qubits.size() && op.num_params() == params.size()) {
    pre_add_gate();
    if (op.is_standard_gate()) {
      if (params.size() > 0)
        qk_circuit_gate(rust_circuit_.get(), op.gate_map(), qubits.data(), params.data());
      else
        qk_circuit_gate(rust_circuit_.get(), op.gate_map(), qubits.data(), nullptr);
    } else {
      if (std::string("reset") == op.name()) {
        qk_circuit_reset(rust_circuit_.get(), qubits[0]);
      } else if (std::string("barrier") == op.name()) {
        qk_circuit_barrier(rust_circuit_.get(), qubits.data(), qubits.size());
      } else if (std::string("measure") == op.name()) {
        qk_circuit_measure(rust_circuit_.get(), qubits[0], qubits[0]);
      }
    }
  }
}


void QuantumCircuit::append(const CircuitInstruction& inst)
{
  std::vector<std::uint32_t> vqubits(inst.qubits().size());
  for (int_t i = 0; i< inst.qubits().size(); i++) {
    vqubits[i] = (std::uint32_t)inst.qubits()[i];
  }
  pre_add_gate();
  if (inst.instruction().is_standard_gate()) {
    if (inst.instruction().num_params() > 0)
      qk_circuit_gate(rust_circuit_.get(), inst.instruction().gate_map(), vqubits.data(), inst.instruction().params().data());
    else
      qk_circuit_gate(rust_circuit_.get(), inst.instruction().gate_map(), vqubits.data(), nullptr);
  } else {
    if (std::string("reset") == inst.instruction().name()) {
      qk_circuit_reset(rust_circuit_.get(), vqubits[0]);
    } else if (std::string("barrier") == inst.instruction().name()) {
      qk_circuit_barrier(rust_circuit_.get(), vqubits.data(), vqubits.size());
    } else if (std::string("measure") == inst.instruction().name()) {
      qk_circuit_measure(rust_circuit_.get(), vqubits[0], (std::uint32_t)inst.clbits()[0]);
    }
  }
}


uint_t QuantumCircuit::num_instructions(void)
{
  return qk_circuit_num_instructions(rust_circuit_.get());
}

CircuitInstruction QuantumCircuit::operator[] (uint_t i)
{
  if (i < qk_circuit_num_instructions(rust_circuit_.get())) {
    auto name_map = get_standard_gate_name_mapping();
    QkCircuitInstruction* op = new QkCircuitInstruction;
    qk_circuit_get_instruction(rust_circuit_.get(), i, op);
    reg_t qubits(op->qubits, op->qubits + op->num_qubits);
    reg_t clbits(op->clbits, op->qubits + op->num_clbits);
    std::vector<double> params(op->params, op->params + op->num_params);
    auto gate = name_map.find(op->name);
    if (gate == name_map.end()) {
      if (strcmp(op->name, "measure") == 0) {
        qk_circuit_instruction_clear(op);
        auto inst = Measure();
        return CircuitInstruction(inst, qubits, clbits);
      } else if (strcmp(op->name, "reset") == 0) {
        qk_circuit_instruction_clear(op);
        auto inst = Reset();
        return CircuitInstruction(inst, qubits, clbits);
      } else if (strcmp(op->name, "barrier") == 0) {
        qk_circuit_instruction_clear(op);
        auto inst = Barrier();
        return CircuitInstruction(inst, qubits, clbits);
      }
    } else {
      // standard gates
      auto inst = gate->second;
      if (params.size() > 0) {
        inst.set_params(params);
      }
      qk_circuit_instruction_clear(op);
      return CircuitInstruction(inst, qubits);
    }
    qk_circuit_instruction_clear(op);
  }
  return CircuitInstruction();
}


// print circuit
void QuantumCircuit::print(void) const
{
  uint_t nops;
  nops = qk_circuit_num_instructions(rust_circuit_.get());

  for (uint_t i=0;i<nops;i++) {
    QkCircuitInstruction* op = new QkCircuitInstruction;
    qk_circuit_get_instruction(rust_circuit_.get(), i, op);
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


std::string QuantumCircuit::to_qasm3(void)
{
  add_pending_control_flow_op();

  std::stringstream qasm3;
  qasm3 << std::setprecision(18);
  qasm3 << "OPENQASM 3.0;" << std::endl;
  qasm3 << "include \"stdgates.inc\";" << std::endl;

  auto name_map = get_standard_gate_name_mapping();
  // add header for non-standard gates
  bool cs = false;
  bool sxdg = false;
  QkOpCounts opcounts = qk_circuit_count_ops(rust_circuit_.get());
  for (int i = 0; i < opcounts.len; i++) {
    if (opcounts.data[i].count != 0) {
      auto op = name_map[opcounts.data[i].name].gate_map();
      switch (op) {
        case QkGate_R:
          qasm3 << "gate r(p0, p1) _gate_q_0 {" << std::endl;
          qasm3 << "  U(p0, -pi/2 + p1, pi/2 - p1) _gate_q_0;" << std::endl;
          qasm3 << "}" << std::endl;
          break;
        case QkGate_SXdg:
        case QkGate_RYY:
        case QkGate_XXPlusYY:
        case QkGate_XXMinusYY:
          if (!sxdg) {
            qasm3 << "gate sxdg _gate_q_0 {" << std::endl;
            qasm3 << "  s _gate_q_0;" << std::endl;
            qasm3 << "  h _gate_q_0;" << std::endl;
            qasm3 << "  s _gate_q_0;" << std::endl;
            qasm3 << "}" << std::endl;
            sxdg = true;
          }
          if (op == QkGate_RYY) {
            qasm3 << "gate ryy(p0) _gate_q_0, _gate_q_1 {" << std::endl;
            qasm3 << "  sxdg _gate_q_0;" << std::endl;
            qasm3 << "  sxdg _gate_q_1;" << std::endl;
            qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            qasm3 << "  rz(p0) _gate_q_1;" << std::endl;
            qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            qasm3 << "  sx _gate_q_0;" << std::endl;
            qasm3 << "  sx _gate_q_1;" << std::endl;
            qasm3 << "}" << std::endl;
          }
          if (op == QkGate_XXPlusYY) {
            qasm3 << "gate xx_plus_yy(p0, p1) _gate_q_0, _gate_q_1 {" << std::endl;
            qasm3 << "  rz(p1) _gate_q_0;" << std::endl;
            qasm3 << "  sdg _gate_q_1;" << std::endl;
            qasm3 << "  sx _gate_q_1;" << std::endl;
            qasm3 << "  s _gate_q_1;" << std::endl;
            qasm3 << "  s _gate_q_0;" << std::endl;
            qasm3 << "  cx _gate_q_1, _gate_q_0;" << std::endl;
            qasm3 << "  ry((-0.5)*p0) _gate_q_1;" << std::endl;
            qasm3 << "  ry((-0.5)*p0) _gate_q_0;" << std::endl;
            qasm3 << "  cx _gate_q_1, _gate_q_0;" << std::endl;
            qasm3 << "  sdg _gate_q_0;" << std::endl;
            qasm3 << "  sdg _gate_q_1;" << std::endl;
            qasm3 << "  sxdg _gate_q_1;" << std::endl;
            qasm3 << "  s _gate_q_1;" << std::endl;
            qasm3 << "  rz(-p1) _gate_q_0;" << std::endl;
            qasm3 << "}" << std::endl;
          }
          if (op == QkGate_XXMinusYY) {
            qasm3 << "gate xx_minus_yy(p0, p1) _gate_q_0, _gate_q_1 {" << std::endl;
            qasm3 << "  rz(-p1) _gate_q_1;" << std::endl;
            qasm3 << "  sdg _gate_q_0;" << std::endl;
            qasm3 << "  sx _gate_q_0;" << std::endl;
            qasm3 << "  s _gate_q_0;" << std::endl;
            qasm3 << "  s _gate_q_1;" << std::endl;
            qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            qasm3 << "  ry(0.5*p0) _gate_q_0;" << std::endl;
            qasm3 << "  ry((-0.5)*p0) _gate_q_1;" << std::endl;
            qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            qasm3 << "  sdg _gate_q_1;" << std::endl;
            qasm3 << "  sdg _gate_q_0;" << std::endl;
            qasm3 << "  sxdg _gate_q_0;" << std::endl;
            qasm3 << "  s _gate_q_0;" << std::endl;
            qasm3 << "  rz(p1) _gate_q_1;" << std::endl;
            qasm3 << "}" << std::endl;
          }
          break;
        case QkGate_DCX:
          qasm3 << "gate dcx _gate_q_0, _gate_q_1 {" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
          qasm3 << "  cx _gate_q_1, _gate_q_0;" << std::endl;
          qasm3 << "}" << std::endl;
          break;
        case QkGate_ECR:
          qasm3 << "gate ecr _gate_q_0, _gate_q_1 {" << std::endl;
          qasm3 << "  s _gate_q_0;" << std::endl;
          qasm3 << "  sx _gate_q_1;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
          qasm3 << "  x _gate_q_0;" << std::endl;
          qasm3 << "}" << std::endl;
          break;
        case QkGate_ISwap:
          qasm3 << "gate iswap _gate_q_0, _gate_q_1 {" << std::endl;
          qasm3 << "  s _gate_q_0;" << std::endl;
          qasm3 << "  s _gate_q_1;" << std::endl;
          qasm3 << "  h _gate_q_0;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
          qasm3 << "  cx _gate_q_1, _gate_q_0;" << std::endl;
          qasm3 << "  h _gate_q_1;" << std::endl;
          qasm3 << "}" << std::endl;
          break;
        case QkGate_CSX:
        case QkGate_CS:
          if (!cs) {
            qasm3 << "gate cs _gate_q_0, _gate_q_1 {" << std::endl;
            qasm3 << "  t _gate_q_0;" << std::endl;
            qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            qasm3 << "  tdg _gate_q_1;" << std::endl;
            qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            qasm3 << "  t _gate_q_1;" << std::endl;
            qasm3 << "}" << std::endl;
            cs = true;
          }
          if (op == QkGate_CSX) {
            qasm3 << "gate csx _gate_q_0, _gate_q_1 {" << std::endl;
            qasm3 << "  h _gate_q_1;" << std::endl;
            qasm3 << "  cs _gate_q_0, _gate_q_1;" << std::endl;
            qasm3 << "  h _gate_q_1;" << std::endl;
            qasm3 << "}" << std::endl;
          }
          break;
        case QkGate_CSdg:
          qasm3 << "gate csdg _gate_q_0, _gate_q_1 {" << std::endl;
          qasm3 << "  tdg _gate_q_0;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
          qasm3 << "  t _gate_q_1;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
          qasm3 << "  tdg _gate_q_1;" << std::endl;
          qasm3 << "}" << std::endl;
          break;
        case QkGate_CCZ:
          qasm3 << "gate ccz _gate_q_0, _gate_q_1, _gate_q_2 {" << std::endl;
          qasm3 << "  h _gate_q_2;" << std::endl;
          qasm3 << "  ccx _gate_q_0, _gate_q_1, _gate_q_2;" << std::endl;
          qasm3 << "  h _gate_q_2;" << std::endl;
          qasm3 << "}" << std::endl;
          break;
        case QkGate_RXX:
          qasm3 << "gate rxx(p0) _gate_q_0, _gate_q_1 {" << std::endl;
          qasm3 << "  h _gate_q_0;" << std::endl;
          qasm3 << "  h _gate_q_1;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
          qasm3 << "  rz(p0) _gate_q_1;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
          qasm3 << "  h _gate_q_1;" << std::endl;
          qasm3 << "  h _gate_q_0;" << std::endl;
          qasm3 << "}" << std::endl;
          break;
        case QkGate_RZX:
          qasm3 << "gate rzx(p0) _gate_q_0, _gate_q_1 {" << std::endl;
          qasm3 << "  h _gate_q_1;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
          qasm3 << "  rz(p0) _gate_q_1;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
          qasm3 << "  h _gate_q_1;" << std::endl;
          qasm3 << "}" << std::endl;
          break;
        case QkGate_RCCX:
          qasm3 << "gate rccx _gate_q_0, _gate_q_1, _gate_q_2 {" << std::endl;
          qasm3 << "  h _gate_q_2;" << std::endl;
          qasm3 << "  t _gate_q_2;" << std::endl;
          qasm3 << "  cx _gate_q_1, _gate_q_2;" << std::endl;
          qasm3 << "  tdg _gate_q_2;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_2;" << std::endl;
          qasm3 << "  t _gate_q_2;" << std::endl;
          qasm3 << "  cx _gate_q_1, _gate_q_2;" << std::endl;
          qasm3 << "  tdg _gate_q_2;" << std::endl;
          qasm3 << "  h _gate_q_2;" << std::endl;
          qasm3 << "}" << std::endl;
          break;
        case QkGate_C3X:
          qasm3 << "gate mcx _gate_q_0, _gate_q_1, _gate_q_2, _gate_q_3 {" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  p(pi/8) _gate_q_0;" << std::endl;
          qasm3 << "  p(pi/8) _gate_q_1;" << std::endl;
          qasm3 << "  p(pi/8) _gate_q_2;" << std::endl;
          qasm3 << "  p(pi/8) _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
          qasm3 << "  p(-pi/8) _gate_q_1;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
          qasm3 << "  cx _gate_q_1, _gate_q_2;" << std::endl;
          qasm3 << "  p(-pi/8) _gate_q_2;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_2;" << std::endl;
          qasm3 << "  p(pi/8) _gate_q_2;" << std::endl;
          qasm3 << "  cx _gate_q_1, _gate_q_2;" << std::endl;
          qasm3 << "  p(-pi/8) _gate_q_2;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_2;" << std::endl;
          qasm3 << "  cx _gate_q_2, _gate_q_3;" << std::endl;
          qasm3 << "  p(-pi/8) _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_1, _gate_q_3;" << std::endl;
          qasm3 << "  p(pi/8) _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_2, _gate_q_3;" << std::endl;
          qasm3 << "  p(-pi/8) _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_3;" << std::endl;
          qasm3 << "  p(pi/8) _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_2, _gate_q_3;" << std::endl;
          qasm3 << "  p(-pi/8) _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_1, _gate_q_3;" << std::endl;
          qasm3 << "  p(pi/8) _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_2, _gate_q_3;" << std::endl;
          qasm3 << "  p(-pi/8) _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_3;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "}" << std::endl;
          break;
        case QkGate_C3SX:
          qasm3 << "gate c3sx _gate_q_0, _gate_q_1, _gate_q_2, _gate_q_3 {" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  cp(pi/8) _gate_q_0, _gate_q_3;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  cp(-pi/8) _gate_q_1, _gate_q_3;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  cp(pi/8) _gate_q_1, _gate_q_3;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_1, _gate_q_2;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  cp(-pi/8) _gate_q_2, _gate_q_3;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_2;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  cp(pi/8) _gate_q_2, _gate_q_3;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_1, _gate_q_2;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  cp(-pi/8) _gate_q_2, _gate_q_3;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_2;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  cp(pi/8) _gate_q_2, _gate_q_3;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "}" << std::endl;
          break;
        case QkGate_RC3X:
          qasm3 << "gate rcccx _gate_q_0, _gate_q_1, _gate_q_2, _gate_q_3 {" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  t _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_2, _gate_q_3;" << std::endl;
          qasm3 << "  tdg _gate_q_3;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_3;" << std::endl;
          qasm3 << "  t _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_1, _gate_q_3;" << std::endl;
          qasm3 << "  tdg _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_0, _gate_q_3;" << std::endl;
          qasm3 << "  t _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_1, _gate_q_3;" << std::endl;
          qasm3 << "  tdg _gate_q_3;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "  t _gate_q_3;" << std::endl;
          qasm3 << "  cx _gate_q_2, _gate_q_3;" << std::endl;
          qasm3 << "  tdg _gate_q_3;" << std::endl;
          qasm3 << "  h _gate_q_3;" << std::endl;
          qasm3 << "}" << std::endl;
          break;
        case QkGate_CU1:
          qasm3 << "gate cu1(p0) _gate_q_0, _gate_q_1 {" << std::endl;
          qasm3 << "  cp(p0) _gate_q_0 _gate_q_1;" << std::endl;
          qasm3 << "}" << std::endl;
          break;
        case QkGate_CU3:
          qasm3 << "gate cu3(p0, p1, p2) _gate_q_0, _gate_q_1 {" << std::endl;
          qasm3 << "  cu(p0, p1, p2, 0) _gate_q_0 _gate_q_1;" << std::endl;
          qasm3 << "}" << std::endl;
          break;
        default:
          break;
      }
    }
  }
  qk_opcounts_clear(&opcounts);

  // registers
  std::string creg_name = "c";
  std::string qreg_name = "q";
  qasm3 << "bit[" << num_clbits_ << "] " << creg_name << ";" << std::endl;
  qasm3 << "qubit[" << num_qubits_ << "] " << qreg_name << ";" << std::endl;

  // save ops
  uint_t nops;
  nops = qk_circuit_num_instructions(rust_circuit_.get());

  for (uint_t i=0;i<nops;i++) {
    QkCircuitInstruction* op = new QkCircuitInstruction;
    qk_circuit_get_instruction(rust_circuit_.get(), i, op);
    if (op->num_clbits > 0) {
      if (op->num_qubits == op->num_clbits) {
        for (int j=0;j<op->num_qubits;j++) {
          qasm3 << creg_name << "[" << op->clbits[j] << "] = " << op->name << " " << qreg_name << "[" << op->qubits[j] << "];" << std::endl;
        }
      }
    }
    else {
      if (strcmp(op->name, "u") == 0) {
        qasm3 << "U";
      } else {
        qasm3 << op->name;
      }
      if (op->num_params > 0) {
        qasm3 << "(";
        for (int j=0;j<op->num_params;j++) {
          qasm3 << op->params[j];
          if (j != op->num_params - 1)
            qasm3 << ", ";
        }
        qasm3 << ")";
      }
      if (op->num_qubits > 0) {
        qasm3 << " ";
        for (int j=0;j<op->num_qubits;j++) {
          qasm3 << qreg_name << "[" << op->qubits[j] << "]";
          if (j != op->num_qubits - 1)
            qasm3 << ", ";
        }
      }
      qasm3 << ";" << std::endl;
    }
    qk_circuit_instruction_clear(op);
  }

  return qasm3.str();
}




} // namespace circuit
} // namespace Qiskit


#endif  // __qiskitcpp_circuit_quantum_circuit_impl_hpp__

