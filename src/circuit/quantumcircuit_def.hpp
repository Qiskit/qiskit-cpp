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
#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <cassert>

#include "utils/types.hpp"
#include "circuit/parameter.hpp"
//#include "circuit/classical/expr.hpp"
#include "circuit/classicalregister.hpp"
#include "circuit/quantumregister.hpp"
#include "circuit/library/standard_gates/standard_gates.hpp"
#include "circuit/circuitinstruction.hpp"

#include "circuit/barrier.hpp"
#include "circuit/measure.hpp"
#include "circuit/reset.hpp"

#include <complex>
#include "qiskit.h"

// this macro replaces Complex64 used in rust FFI
#define Complex64 std::complex<double>

// qiskit C-API circuit data
using rust_circuit = ::QkCircuit;


#define QC_COMPARE_EPS (1e-15)


namespace Qiskit
{
namespace circuit
{

class ControlFlowOp;
class IfElseOp;

static Parameter null_param;

/// @class QuantumCircuit
/// @brief Qiskit representation of a quantum circuit.
class QuantumCircuit
{
protected:
	uint_t num_qubits_;			// number of qubits
	uint_t num_clbits_;			// number of classical bits
	double global_phase_ = 0.0; // initial global phase

	std::vector<QuantumRegister> qregs_;   // quantum registers
	std::vector<ClassicalRegister> cregs_; // classical registers

	std::shared_ptr<rust_circuit> rust_circuit_ = nullptr; // shared pointer to the circuit for Rust

	std::shared_ptr<ControlFlowOp> pending_control_flow_op_ = nullptr; // shared pointer to control flow object

	reg_t qubit_map_;									 // qubit map caused by transpiling
	std::vector<std::pair<uint_t, uint_t>> measure_map_; // a list of pair of qubit and clbit for measure
public:
	/// @brief Create a new QuantumCircuit
	QuantumCircuit() {}

	/// @brief Create a new QuantumCircuit
	/// @param num_qubits The number of qubits in the circuit
	/// @param num_clbits The number of clbits in the circuit
	/// @param global_phase The global phase of the circuit, measured in radians
	QuantumCircuit(const uint_t num_qubits, const uint_t num_clbits, const double global_phase = 0.0)
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

		rust_circuit_ = std::shared_ptr<rust_circuit>(qk_circuit_new(0, 0), qk_circuit_free);
		qk_circuit_add_quantum_register(rust_circuit_.get(), qregs_[0].get_register().get());
		qk_circuit_add_classical_register(rust_circuit_.get(), cregs_[0].get_register().get());

		if (global_phase != 0.0) {
			qk_circuit_gate(rust_circuit_.get(), QkGate_GlobalPhase, nullptr, &global_phase_);
		}
	}

	/// @brief Create a new QuantumCircuit
	/// @param qreg A QuantumRegister
	/// @param creg A ClassicalRegister
	/// @param global_phase The global phase of the circuit, measured in radians
	QuantumCircuit(QuantumRegister &qreg, ClassicalRegister &creg, const double global_phase = 0.0)
	{
		num_qubits_ = qreg.size();
		num_clbits_ = creg.size();
		global_phase_ = global_phase;

		qregs_.resize(1);
		cregs_.resize(1);

		qregs_[0] = qreg;
		cregs_[0] = creg;

		rust_circuit_ = std::shared_ptr<rust_circuit>(qk_circuit_new(0, 0), qk_circuit_free);

		qk_circuit_add_quantum_register(rust_circuit_.get(), qregs_[0].get_register().get());
		qk_circuit_add_classical_register(rust_circuit_.get(), cregs_[0].get_register().get());

		if (global_phase != 0.0) {
			qk_circuit_gate(rust_circuit_.get(), QkGate_GlobalPhase, nullptr, &global_phase_);
		}
	}

	/// @brief Create a new QuantumCircuit
	/// @param qregs A list of QuantumRegister
	/// @param cregs A list of ClassicalRegister
	/// @param global_phase The global phase of the circuit, measured in radians
	QuantumCircuit(std::vector<QuantumRegister> qregs, std::vector<ClassicalRegister> cregs, const double global_phase = 0.0)
	{
		num_qubits_ = 0;
		num_clbits_ = 0;
		global_phase_ = global_phase;

		qregs_.resize(qregs.size());
		cregs_.resize(cregs.size());

		for (uint_t i = 0; i < qregs.size(); i++) {
			qregs_[i] = qregs[i];
			qregs[i][0].get_register()->set_base_index(num_qubits_);
			qregs_[i].set_base_index(num_qubits_);
			num_qubits_ += qregs[i].size();
		}

		for (uint_t i = 0; i < cregs.size(); i++) {
			cregs_[i] = cregs[i];
			cregs[i][0].get_register()->set_base_index(num_clbits_);
			cregs_[i].set_base_index(num_clbits_);
			num_clbits_ += cregs[i].size();
		}

		rust_circuit_ = std::shared_ptr<rust_circuit>(qk_circuit_new(0, 0), qk_circuit_free);

		for (uint_t i = 0; i < qregs_.size(); i++) {
			qk_circuit_add_quantum_register(rust_circuit_.get(), qregs_[i].get_register().get());
		}
		for (uint_t i = 0; i < cregs_.size(); i++) {
			qk_circuit_add_classical_register(rust_circuit_.get(), cregs_[i].get_register().get());
		}

		if (global_phase != 0.0) {
			qk_circuit_gate(rust_circuit_.get(), QkGate_GlobalPhase, nullptr, &global_phase_);
		}
	}

	/// @brief Create a new reference to Quantum Circuit
	/// @details Copy constructor of QuantumCircuit does not copy the circuit,
	///          but copies shared pointer to Rust's circuit
	///          If you want to make a copy of the circuit,
	///          please call QuantumCircuit::copy explicitly.
	/// @param circ a Quantum Circuit to be copied the refrence in the new object
	QuantumCircuit(const QuantumCircuit &circ)
	{
		num_qubits_ = circ.num_qubits_;
		num_clbits_ = circ.num_clbits_;
		global_phase_ = circ.global_phase_;

		qregs_ = circ.qregs_;
		cregs_ = circ.cregs_;

		rust_circuit_ = circ.rust_circuit_;

		measure_map_ = circ.measure_map_;
		qubit_map_ = circ.qubit_map_;
	}

	~QuantumCircuit()
	{
		if (pending_control_flow_op_) {
			pending_control_flow_op_.reset();
		}

		if (rust_circuit_) {
			rust_circuit_.reset();
		}
	}

	/// @brief Return number of qubits
	/// @return number of qubits
	uint_t num_qubits(void) const
	{
		return num_qubits_;
	}

	/// @brief Return number of classical bits
	/// @return number of classical bits
	uint_t num_clbits(void) const
	{
		return num_clbits_;
	}

	/// @brief Return number of qregs
	/// @return number of qregs
	uint_t num_qregs(void) const
	{
		return qregs_.size();
	}

	/// @brief Return number of cregs
	/// @return number of cregs
	uint_t num_cregs(void) const
	{
		return cregs_.size();
	}

	/// @brief Return a list of qregs
	/// @return reference to a list of qregs
	const std::vector<QuantumRegister>& qregs(void) const
	{
		return qregs_;
	}

	/// @brief Return a list of cregs
	/// @return reference to a list of cregs
	const std::vector<ClassicalRegister>& cregs(void) const
	{
		return cregs_;
	}

	std::shared_ptr<rust_circuit> get_rust_circuit(const bool update = true)
	{
		if (update)
			add_pending_control_flow_op();
		return rust_circuit_;
	}

	/// @brief Copy Quantum Circuit
	/// @return copied circuit
	QuantumCircuit copy(void)
	{
		QuantumCircuit copied;
		copied.rust_circuit_ = std::shared_ptr<rust_circuit>(qk_circuit_copy(rust_circuit_.get()), qk_circuit_free);

		copied.num_qubits_ = num_qubits_;
		copied.num_clbits_ = num_clbits_;
		copied.global_phase_ = global_phase_;

		copied.qregs_ = qregs_;
		copied.cregs_ = cregs_;

		copied.measure_map_ = measure_map_;
		copied.qubit_map_ = qubit_map_;
		return copied;
	}

	/// @brief set circuit reference of Qiskit circuit
	/// @param circ smart pointer to RUst circuit
	/// @param map layout mapping
	void set_qiskit_circuit(std::shared_ptr<rust_circuit> circ, const std::vector<uint32_t> &map)
	{
		if (rust_circuit_)
			rust_circuit_.reset();
		rust_circuit_ = circ;
		num_qubits_ = qk_circuit_num_qubits(circ.get());
		num_clbits_ = qk_circuit_num_clbits(circ.get());

		qubit_map_.resize(map.size());
		for (int i = 0; i < map.size(); i++) {
			qubit_map_[i] = (uint_t)map[i];
		}

		// get measured qubits
		uint_t nops;
		nops = qk_circuit_num_instructions(rust_circuit_.get());

		for (uint_t i = 0; i < nops; i++) {
			QkOperationKind kind = qk_circuit_instruction_kind(rust_circuit_.get(), i);
			if (kind == QkOperationKind_Measure) {
				QkCircuitInstruction *op = new QkCircuitInstruction;
				qk_circuit_get_instruction(rust_circuit_.get(), i, op);
				measure_map_.push_back(std::pair<uint_t, uint_t>((uint_t)op->qubits[0], (uint_t)op->clbits[0]));
				qk_circuit_instruction_clear(op);
			}
		}
	}

	/// @brief get qubit mapping
	/// @return qubit mapping
	const reg_t &get_qubit_map(void)
	{
		return qubit_map_;
	}

	/// @brief get qubits to be measured
	/// @return a set of qubits
	std::vector<std::pair<uint_t, uint_t>> &get_measure_map(void)
	{
		return measure_map_;
	}

	/// @brief set global phase
	/// @param phase global phase value
	void global_phase(const double phase)
	{
		global_phase_ = phase;
		qk_circuit_gate(rust_circuit_.get(), QkGate_GlobalPhase, nullptr, &global_phase_);
	}

	/// @brief Apply HGate
	/// @param qubit The qubit to apply the gate to.
	void h(const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_H, qubits, nullptr);
	}

	/// @brief Apply IGate
	/// @param qubit The qubit to apply the gate to.
	void i(const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_I, qubits, nullptr);
	}

	/// @brief Apply XGate
	/// @param qubit The qubit to apply the gate to.
	void x(const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_X, qubits, nullptr);
	}

	/// @brief Apply YGate
	/// @param qubit The qubit to apply the gate to.
	void y(const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_Y, qubits, nullptr);
	}

	/// @brief Apply ZGate
	/// @param qubit The qubit to apply the gate to.
	void z(const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_Z, qubits, nullptr);
	}

	/// @brief Apply PhaseGate
	/// @param phase Phase.
	/// @param qubit The qubit to apply the gate to.
	void p(const double phase, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_Phase, qubits, &phase);
	}

	/// @brief Apply PhaseGate
	/// @param phase Phase.
	/// @param qubit The qubit to apply the gate to.
	void p(const Parameter &phase, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		QkParam* params[] = {phase.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_Phase, qubits, params);
	}

	/// @brief Apply RGate
	/// @param theta The angle of the rotation.
	/// @param phi The angle of the axis of rotation in the x-y plane.
	/// @param qubit The qubit to apply the gate to.
	void r(const double theta, const double phi, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		double params[] = {theta, phi};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_R, qubits, params);
	}

	/// @brief Apply RGate
	/// @param theta The angle of the rotation
	/// @param phi The angle of the axis of rotation in the x-y plane
	/// @param qubit The qubit to apply the gate to
	void r(const Parameter &theta, const Parameter &phi, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		QkParam* params[] = {theta.qiskit_param_.get(), phi.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_R, qubits, params);
	}

	/// @brief Apply RXGate
	/// @param theta The angle of the rotation
	/// @param qubit The qubit to apply the gate to
	void rx(const double theta, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_RX, qubits, &theta);
	}

	/// @brief Apply RXGate
	/// @param theta The angle of the rotation
	/// @param qubit The qubit to apply the gate to
	void rx(const Parameter &theta, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		QkParam* params[] = {theta.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_RX, qubits, params);
	}

	/// @brief Apply RYGate
	/// @param theta The angle of the rotation
	/// @param qubit The qubit to apply the gate to
	void ry(const double theta, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_RY, qubits, &theta);
	}

	/// @brief Apply RYGate
	/// @param theta The angle of the rotation
	/// @param qubit The qubit to apply the gate to
	void ry(const Parameter &theta, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		QkParam* params[] = {theta.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_RY, qubits, params);
	}

	/// @brief Apply RZGate
	/// @param theta The angle of the rotation
	/// @param qubit The qubit to apply the gate to
	void rz(const double theta, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_RZ, qubits, &theta);
	}

	/// @brief Apply RZGate
	/// @param theta The angle of the rotation
	/// @param qubit The qubit to apply the gate to
	void rz(const Parameter &theta, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		QkParam* params[] = {theta.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_RZ, qubits, params);
	}

	/// @brief Apply SGate
	/// @param qubit The qubit to apply the gate to.
	void s(const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_S, qubits, nullptr);
	}

	/// @brief Apply SdgGate
	/// @param qubit The qubit to apply the gate to.
	void sdg(const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_Sdg, qubits, nullptr);
	}

	/// @brief Apply SXGate
	/// @param qubit The qubit to apply the gate to.
	void sx(const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_SX, qubits, nullptr);
	}

	/// @brief Apply SXdgGate
	/// @param qubit The qubit to apply the gate to.
	void sxdg(const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_SXdg, qubits, nullptr);
	}

	/// @brief Apply TGate
	/// @param qubit The qubit to apply the gate to.
	void t(const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_T, qubits, nullptr);
	}

	/// @brief Apply TdgGate
	/// @param qubit The qubit to apply the gate to.
	void tdg(const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_Tdg, qubits, nullptr);
	}

	/// @brief Apply UGate
	/// @param theta The theta rotation angle of the gate.
	/// @param phi The phi rotation angle of the gate.
	/// @param lam The lam rotation angle of the gate.
	/// @param qubit The qubit to apply the gate to.
	void u(const double theta, const double phi, const double lam, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		double params[] = {theta, phi, lam};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_U, qubits, params);
	}

	/// @brief Apply UGate
	/// @param theta The theta rotation angle of the gate.
	/// @param phi The phi rotation angle of the gate.
	/// @param lam The lam rotation angle of the gate.
	/// @param qubit The qubit to apply the gate to.
	void u(const Parameter &theta, const Parameter &phi, const Parameter &lam, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		QkParam* params[] = {theta.qiskit_param_.get(), phi.qiskit_param_.get(), lam.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_U, qubits, params);
	}

	/// @brief Apply U1Gate
	/// @param theta The theta rotation angle of the gate.
	void u1(const double theta, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		double params[] = {theta};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_U1, qubits, params);
	}

	/// @brief Apply U1Gate
	/// @param theta The theta rotation angle of the gate.
	/// @param qubit The qubit to apply the gate to.
	void u1(const Parameter &theta, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		QkParam* params[] = {theta.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_U1, qubits, params);
	}

	/// @brief Apply U2Gate
	/// @param phi The phi rotation angle of the gate.
	/// @param lam The lam rotation angle of the gate.
	/// @param qubit The qubit to apply the gate to.
	void u2(const double phi, const double lam, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		double params[] = {phi, lam};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_U2, qubits, params);
	}

	/// @brief Apply U2Gate
	/// @param phi The phi rotation angle of the gate.
	/// @param lam The lam rotation angle of the gate.
	/// @param qubit The qubit to apply the gate to.
	void u2(const Parameter &phi, const Parameter &lam, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		QkParam* params[] = {phi.qiskit_param_.get(), lam.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_U2, qubits, params);
	}

	/// @brief Apply U3Gate
	/// @param theta The theta rotation angle of the gate.
	/// @param phi The phi rotation angle of the gate.
	/// @param lam The lam rotation angle of the gate.
	/// @param qubit The qubit to apply the gate to.
	void u3(const double theta, const double phi, const double lam, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		double params[] = {theta, phi, lam};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_U3, qubits, params);
	}

	/// @brief Apply U3Gate
	/// @param theta The theta rotation angle of the gate.
	/// @param phi The phi rotation angle of the gate.
	/// @param lam The lam rotation angle of the gate.
	/// @param qubit The qubit to apply the gate to.
	void u3(const Parameter &theta, const Parameter &phi, const Parameter &lam, const uint_t qubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit};
		QkParam* params[] = {theta.qiskit_param_.get(), phi.qiskit_param_.get(), lam.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_U3, qubits, params);
	}

	/// @brief Apply unitary gate specified by unitary to qubits
	/// @param unitary Unitary operator
	/// @param qubits The circuit qubits to apply the transformation to
	void unitary(const std::vector<complex_t> &unitary, const reg_t &qubits)
	{
		pre_add_gate();
		std::vector<std::uint32_t> qubits32(qubits.size());
		for (uint_t i = 0; i < qubits.size(); i++)
			qubits32[i] = (std::uint32_t)qubits[i];

		qk_circuit_unitary(rust_circuit_.get(), (const QkComplex64 *)unitary.data(), qubits32.data(), (std::uint32_t)qubits.size(), true);
	}

	/// @brief Apply CHGate
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void ch(const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CH, qubits, nullptr);
	}

	/// @brief Apply CXGate
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void cx(const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CX, qubits, nullptr);
	}

	/// @brief Apply CYGate
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void cy(const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CY, qubits, nullptr);
	}

	/// @brief Apply CZGate
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void cz(const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CZ, qubits, nullptr);
	}

	/// @brief Apply DCXGate
	/// @param qubit1 The qubit to apply the gate to
	/// @param qubit2 The qubit to apply the gate to
	void dcx(const uint_t qubit1, const uint_t qubit2)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_DCX, qubits, nullptr);
	}

	/// @brief Apply ECRGate
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void ecr(const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_ECR, qubits, nullptr);
	}

	/// @brief Apply SwapGate
	/// @param qubit1 The qubit to apply the gate to
	/// @param qubit2 The qubit to apply the gate to
	void swap(const uint_t qubit1, const uint_t qubit2)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_Swap, qubits, nullptr);
	}

	/// @brief Apply iSwapGate
	/// @param qubit1 The qubit to apply the gate to
	/// @param qubit2 The qubit to apply the gate to
	void iswap(const uint_t qubit1, const uint_t qubit2)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_ISwap, qubits, nullptr);
	}

	/// @brief Apply controlled PhaseGate
	/// @param phase Phase.
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void cp(const double phase, const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CPhase, qubits, &phase);
	}

	/// @brief Apply controlled PhaseGate
	/// @param phase Phase.
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void cp(const Parameter &phase, const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		QkParam* params[] = {phase.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_CPhase, qubits, params);
	}

	/// @brief Apply controlled RXGate
	/// @param theta The angle of the rotation
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void crx(const double theta, const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CRX, qubits, &theta);
	}

	/// @brief Apply controlled RXGate
	/// @param theta The angle of the rotation
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void crx(const Parameter &theta, const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		QkParam* params[] = {theta.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_CRX, qubits, params);
	}

	/// @brief Apply controlled RYGate
	/// @param theta The angle of the rotation
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void cry(const double theta, const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CRY, qubits, &theta);
	}

	/// @brief Apply controlled RYGate
	/// @param theta The angle of the rotation
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void cry(const Parameter &theta, const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		QkParam* params[] = {theta.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_CRY, qubits, params);
	}

	/// @brief Apply controlled RZGate
	/// @param theta The angle of the rotation
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void crz(const double theta, const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CRZ, qubits, &theta);
	}

	/// @brief Apply controlled RZGate
	/// @param theta The angle of the rotation
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void crz(const Parameter &theta, const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		QkParam* params[] = {theta.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_CRZ, qubits, params);
	}

	/// @brief Apply CSGate
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void cs(const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CS, qubits, nullptr);
	}

	/// @brief Apply CSdgGate
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void csdg(const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CSdg, qubits, nullptr);
	}

	/// @brief Apply CSXGate
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void csx(const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CSX, qubits, nullptr);
	}

	/// @brief Apply CUGate
	/// @param theta The theta rotation angle of the gate.
	/// @param phi The phi rotation angle of the gate.
	/// @param lam The lam rotation angle of the gate.
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void cu(const double theta, const double phi, const double lam, const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		double params[] = {theta, phi, lam};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CU, qubits, params);
	}

	/// @brief Apply CUGate
	/// @param theta The theta rotation angle of the gate.
	/// @param phi The phi rotation angle of the gate.
	/// @param lam The lam rotation angle of the gate.
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void cu(const Parameter &theta, const Parameter &phi, const Parameter &lam, const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		QkParam* params[] = {theta.qiskit_param_.get(), phi.qiskit_param_.get(), lam.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_CU, qubits, params);
	}

	/// @brief Apply CU1Gate
	/// @param theta The theta rotation angle of the gate.
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void cu1(const double theta, const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		double params[] = {theta};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CU1, qubits, params);
	}

	/// @brief Apply CU1Gate
	/// @param theta The theta rotation angle of the gate.
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void cu1(const Parameter &theta, const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		QkParam* params[] = {theta.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_CU1, qubits, params);
	}

	/// @brief Apply CU3Gate
	/// @param theta The theta rotation angle of the gate.
	/// @param phi The phi rotation angle of the gate.
	/// @param lam The lam rotation angle of the gate.
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void cu3(const double theta, const double phi, const double lam, const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		double params[] = {theta, phi, lam};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CU3, qubits, params);
	}

	/// @brief Apply CU3Gate
	/// @param theta The theta rotation angle of the gate.
	/// @param phi The phi rotation angle of the gate.
	/// @param lam The lam rotation angle of the gate.
	/// @param cqubit The qubit used as the control
	/// @param tqubit The qubit targeted by the gate
	void cu3(const Parameter &theta, const Parameter &phi, const Parameter &lam, const uint_t cqubit, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)tqubit};
		QkParam* params[] = {theta.qiskit_param_.get(), phi.qiskit_param_.get(), lam.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_CU3, qubits, params);
	}

	/// @brief Apply RXXGate
	/// @param theta The rotation angle of the gate
	/// @param qubit1 The qubit to apply the gate to
	/// @param qubit2 The qubit to apply the gate to
	void rxx(const double theta, const uint_t qubit1, const uint_t qubit2)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_RXX, qubits, &theta);
	}

	/// @brief Apply RXXGate
	/// @param theta The rotation angle of the gate
	/// @param qubit1 The qubit to apply the gate to
	/// @param qubit2 The qubit to apply the gate to
	void rxx(const Parameter &theta, const uint_t qubit1, const uint_t qubit2)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
		QkParam* params[] = {theta.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_RXX, qubits, params);
	}

	/// @brief Apply RYYGate
	/// @param theta The rotation angle of the gate
	/// @param qubit1 The qubit to apply the gate to
	/// @param qubit2 The qubit to apply the gate to
	void ryy(const double theta, const uint_t qubit1, const uint_t qubit2)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_RYY, qubits, &theta);
	}

	/// @brief Apply RYYGate
	/// @param theta The rotation angle of the gate
	/// @param qubit1 The qubit to apply the gate to
	/// @param qubit2 The qubit to apply the gate to
	void ryy(const Parameter &theta, const uint_t qubit1, const uint_t qubit2)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
		QkParam* params[] = {theta.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_RYY, qubits, params);
	}

	/// @brief Apply RZZGate
	/// @param theta The rotation angle of the gate
	/// @param qubit1 The qubit to apply the gate to
	/// @param qubit2 The qubit to apply the gate to
	void rzz(const double theta, const uint_t qubit1, const uint_t qubit2)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_RZZ, qubits, &theta);
	}

	/// @brief Apply RZZGate
	/// @param theta The rotation angle of the gate
	/// @param qubit1 The qubit to apply the gate to
	/// @param qubit2 The qubit to apply the gate to
	void rzz(const Parameter &theta, const uint_t qubit1, const uint_t qubit2)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
		QkParam* params[] = {theta.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_RZZ, qubits, params);
	}

	/// @brief Apply RZXGate
	/// @param theta The rotation angle of the gate
	/// @param qubit1 The qubit to apply the gate to
	/// @param qubit2 The qubit to apply the gate to
	void rzx(const double theta, const uint_t qubit1, const uint_t qubit2)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_RZX, qubits, &theta);
	}

	/// @brief Apply RZXGate
	/// @param theta The rotation angle of the gate
	/// @param qubit1 The qubit to apply the gate to
	/// @param qubit2 The qubit to apply the gate to
	void rzx(const Parameter &theta, const uint_t qubit1, const uint_t qubit2)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
		QkParam* params[] = {theta.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_RZX, qubits, params);
	}

	/// @brief Apply XXminusYY
	/// @param theta The rotation angle of the gate
	/// @param beta The phase angle of the gate
	/// @param qubit1 The qubit to apply the gate to
	/// @param qubit2 The qubit to apply the gate to
	void xx_minus_yy(const double theta, const double beta, const uint_t qubit1, const uint_t qubit2)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
		double params[] = {theta, beta};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_XXMinusYY, qubits, params);
	}

	/// @brief Apply XXminusYY
	/// @param theta The rotation angle of the gate
	/// @param beta The phase angle of the gate
	/// @param qubit1 The qubit to apply the gate to
	/// @param qubit2 The qubit to apply the gate to
	void xx_minus_yy(const Parameter &theta, const Parameter &beta, const uint_t qubit1, const uint_t qubit2)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
		QkParam* params[] = {theta.qiskit_param_.get(), beta.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_XXMinusYY, qubits, params);
	}

	/// @brief Apply XXplusYY
	/// @param theta The rotation angle of the gate
	/// @param beta The phase angle of the gate
	/// @param qubit1 The qubit to apply the gate to
	/// @param qubit2 The qubit to apply the gate to
	void xx_plus_yy(const double theta, const double beta, const uint_t qubit1, const uint_t qubit2)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
		double params[] = {theta, beta};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_XXPlusYY, qubits, params);
	}

	/// @brief Apply XXplusYY
	/// @param theta The rotation angle of the gate
	/// @param beta The phase angle of the gate
	/// @param qubit1 The qubit to apply the gate to
	/// @param qubit2 The qubit to apply the gate to
	void xx_plus_yy(const Parameter &theta, const Parameter &beta, const uint_t qubit1, const uint_t qubit2)
	{
		std::uint32_t qubits[] = {(std::uint32_t)qubit1, (std::uint32_t)qubit2};
		QkParam* params[] = {theta.qiskit_param_.get(), beta.qiskit_param_.get()};
		pre_add_gate();
		qk_circuit_parameterized_gate(rust_circuit_.get(), QkGate_XXPlusYY, qubits, params);
	}

	/// @brief Apply CCXGate
	/// @param cqubit1 The qubit used as the first control
	/// @param cqubit2 The qubit used as the second control
	/// @param tqubit The qubit targeted by the gate
	void ccx(const uint_t cqubit1, const uint_t cqubit2, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CCX, qubits, nullptr);
	}

	/// @brief Apply CCZGate
	/// @param cqubit1 The qubit used as the first control
	/// @param cqubit2 The qubit used as the second control
	/// @param tqubit The qubit targeted by the gate
	void ccz(const uint_t cqubit1, const uint_t cqubit2, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CCZ, qubits, nullptr);
	}

	/// @brief Apply CSwapGate
	/// @param cqubit The qubit used as the control
	/// @param qubit1 The qubit to apply the gate to
	/// @param qubit2 The qubit to apply the gate to
	void cswap(const uint_t cqubit, const uint_t qubit1, const uint_t qubit2)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit, (std::uint32_t)qubit1, (std::uint32_t)qubit2};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_CSwap, qubits, nullptr);
	}

	/// @brief Apply RCCXGate
	/// @param cqubit1 The qubit used as the first control
	/// @param cqubit2 The qubit used as the second control
	/// @param tqubit The qubit targeted by the gate
	void rccx(const uint_t cqubit1, const uint_t cqubit2, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_RCCX, qubits, nullptr);
	}

	/// @brief Apply C3XGate
	/// @param cqubit1 The qubit used as the first control
	/// @param cqubit2 The qubit used as the second control
	/// @param cqubit3 The qubit used as the third control
	/// @param tqubit The qubit targeted by the gate
	void cccx(const uint_t cqubit1, const uint_t cqubit2, const uint_t cqubit3, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)cqubit3, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_C3X, qubits, nullptr);
	}

	/// @brief Apply C3SXGate
	/// @param cqubit1 The qubit used as the first control
	/// @param cqubit2 The qubit used as the second control
	/// @param cqubit3 The qubit used as the third control
	/// @param tqubit The qubit targeted by the gate
	void cccsx(const uint_t cqubit1, const uint_t cqubit2, const uint_t cqubit3, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)cqubit3, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_C3SX, qubits, nullptr);
	}

	/// @brief Apply RC3XGate
	/// @param cqubit1 The qubit used as the first control
	/// @param cqubit2 The qubit used as the second control
	/// @param cqubit3 The qubit used as the third control
	/// @param tqubit The qubit targeted by the gate
	void rcccx(const uint_t cqubit1, const uint_t cqubit2, const uint_t cqubit3, const uint_t tqubit)
	{
		std::uint32_t qubits[] = {(std::uint32_t)cqubit1, (std::uint32_t)cqubit2, (std::uint32_t)cqubit3, (std::uint32_t)tqubit};
		pre_add_gate();
		qk_circuit_gate(rust_circuit_.get(), QkGate_RC3X, qubits, nullptr);
	}

	// other operations

	/// @brief Measure a quantum bit(qubit) in the Z basis into a classical bit(cbit)
	/// @param qubit The qubit to measure
	/// @param cbit The classical bit to place the measurement result in.
	void measure(const uint_t qubit, const uint_t cbit)
	{
		pre_add_gate();
		qk_circuit_measure(rust_circuit_.get(), (std::uint32_t)qubit, (std::uint32_t)cbit);
		measure_map_.push_back(std::pair<uint_t, uint_t>(qubit, cbit));
	}

	/// @brief Measure a quantum bit(qreg) in the Z basis into a classical bit(creg)
	/// @param qreg The qubit to measure
	/// @param creg The classical bit to place the measurement result in.
	void measure(QuantumRegister &qreg, ClassicalRegister &creg)
	{
		pre_add_gate();
		uint_t size = qreg.size();
		if (size > creg.size())
			size = creg.size();
		// TO DO implement multi-bits measure in C-API
		for (uint_t i = 0; i < size; i++) {
			qk_circuit_measure(rust_circuit_.get(), (std::uint32_t)qreg[i], (std::uint32_t)creg[i]);
			measure_map_.push_back(std::pair<uint_t, uint_t>(qreg[i], creg[i]));
		}
	}

	/// @brief Reset the quantum bit to their default state
	/// @param qubit The qubit to reset
	void reset(const uint_t qubit)
	{
		pre_add_gate();
		qk_circuit_reset(rust_circuit_.get(), (std::uint32_t)qubit);
	}

	/// @brief Reset the quantum bit to their default state
	/// @param qreg The qubit to reset
	void reset(QuantumRegister &qreg)
	{
		pre_add_gate();
		// TO DO implement multi-bits rest in C-API
		for (uint_t i = 0; i < qreg.size(); i++) {
			qk_circuit_reset(rust_circuit_.get(), (std::uint32_t)qreg[i]);
		}
	}

	/// @brief Insert barrier on specified qubit
	/// @param qubit The qubit to put a barrier
	void barrier(const uint_t qubit)
	{
		pre_add_gate();
		std::uint32_t q = (std::uint32_t)qubit;
		qk_circuit_barrier(rust_circuit_.get(), &q, 1);
	}

	/// @brief Insert barrier on multiple qubits
	/// @param qubits The qubits to put barrier
	void barrier(const reg_t &qubits)
	{
		pre_add_gate();
		std::vector<std::uint32_t> qubits32(qubits.size());
		for (uint_t i = 0; i < qubits.size(); i++) {
			qubits32[i] = (std::uint32_t)qubits[i];
		}
		qk_circuit_barrier(rust_circuit_.get(), qubits32.data(), (uint32_t)qubits32.size());
	}


	// control flow ops

	/// @brief IfElseOp with only a True body
	/// @param clbit
	/// @param value
	/// @param body The circuit body to be run if condition is true.
	IfElseOp &if_test(const uint32_t clbit, const uint32_t value, const std::function<void(QuantumCircuit &)> body);
	//  IfElseOp& if_test(Expr expr, std::function<void(qiskitcpp::circuit::QuantumCircuit&)> body);

	// parameter binding

	/// @brief Return the number of parameter objects in the circuit
	/// @return the number of parameter objects in the circuit
	uint_t num_parameters(void) const
	{
		return qk_circuit_num_param_symbols(rust_circuit_.get());
	}

	/// @brief Assign parameters to new parameters or values.
	/// @param keys a list of keys
	/// @param values a list of values
	void assign_parameters(const std::vector<std::string> keys, const std::vector<double> values)
	{
		std::vector<char *> c_keys;
		for (uint_t i = 0; i < keys.size(); i++) {
			c_keys.push_back((char *)keys[i].c_str());
		}
		add_pending_control_flow_op();

		// TO DO : add support for Parameter in Qiskit C-API
		// qc_assign_parameters(rust_circuit_.get(), c_keys.data(), c_keys.size(), values.data(), values.size());
	}

	/// @brief Assign parameter to new parameter or value.
	/// @param key key
	/// @param value value
	void assign_parameter(const std::string key, const double val)
	{
		add_pending_control_flow_op();

		// TO DO : add support for Parameter in Qiskit C-API
		// qc_assign_parameter(rust_circuit_.get(), (char*)key.c_str(), val);
	}

	QuantumCircuit &operator+=(QuantumCircuit &rhs)
	{
		compose(rhs);
		return *this;
	}

	QuantumCircuit operator+(QuantumCircuit &rhs)
	{
		QuantumCircuit new_circ(*this);
		if (num_qubits_ >= rhs.num_qubits_ && num_clbits_ >= rhs.num_clbits_)
		{
			reg_t qubits;
			reg_t clbits;
			rhs.get_qubits(qubits);
			rhs.get_clbits(clbits);
			uint_t size = std::min(qubits.size(), clbits.size());
			std::vector<std::uint32_t> vqubits(size);
			for (uint_t i = 0; i < size; i++)
			{
				vqubits[i] = (std::uint32_t)qubits[i];
			}
			std::vector<std::uint32_t> vclbits(size);
			for (uint_t i = 0; i < size; i++)
			{
				vclbits[i] = (std::uint32_t)clbits[i];
			}
			new_circ.compose(rhs);
		}
		return new_circ;
	}

	void compose(QuantumCircuit &rhs)
	{
		if (num_qubits_ >= rhs.num_qubits_ && num_clbits_ >= rhs.num_clbits_)
		{
			reg_t qubits;
			reg_t clbits;
			rhs.get_qubits(qubits);
			rhs.get_clbits(clbits);
			compose(rhs, qubits, clbits);
		}
	}

	/// @brief Add other circuit at the end of this circuit
	/// @param circ circuit to be added
	/// @param qubits a list of qubits to be mapped
	/// @param clits a list of clbits to be mapped
	void compose(QuantumCircuit &circ, const reg_t &qubits, const reg_t &clbits)
	{
		pre_add_gate();
		uint_t size = std::min(qubits.size(), clbits.size());
		std::vector<std::uint32_t> vqubits(size);
		for (uint_t i = 0; i < size; i++) {
			vqubits[i] = (std::uint32_t)qubits[i];
		}
		std::vector<std::uint32_t> vclbits(size);
		for (uint_t i = 0; i < size; i++) {
			vclbits[i] = (std::uint32_t)clbits[i];
		}

		uint_t nops;
		nops = qk_circuit_num_instructions(circ.rust_circuit_.get());

		auto name_map = get_standard_gate_name_mapping();
		for (uint_t i = 0; i < nops; i++) {
			QkCircuitInstruction *op = new QkCircuitInstruction;
			qk_circuit_get_instruction(circ.rust_circuit_.get(), i, op);

			std::vector<std::uint32_t> vqubits(op->num_qubits);
			std::vector<std::uint32_t> vclbits;
			for (uint_t j = 0; j < op->num_qubits; j++) {
				vqubits[j] = (std::uint32_t)qubits[op->qubits[j]];
			}
			if (op->num_clbits > 0) {
				vclbits.resize(op->num_clbits);
				for (uint_t j = 0; j < op->num_clbits; j++) {
					vclbits[j] = (std::uint32_t)clbits[op->clbits[j]];
				}
			}
			QkOperationKind kind = qk_circuit_instruction_kind(rust_circuit_.get(), i);
			if (kind == QkOperationKind_Measure) {
				qk_circuit_measure(rust_circuit_.get(), vqubits[0], vclbits[0]);
			} else if (kind == QkOperationKind_Reset) {
				qk_circuit_reset(rust_circuit_.get(), vqubits[0]);
			} else if (kind == QkOperationKind_Barrier) {
				qk_circuit_barrier(rust_circuit_.get(), vqubits.data(), (uint32_t)vqubits.size());
			} else if (kind == QkOperationKind_Gate) {
				qk_circuit_parameterized_gate(rust_circuit_.get(), name_map[op->name].gate_map(), vqubits.data(), op->params);
			} else if (kind == QkOperationKind_Unitary) {
				// TO DO : how we can get unitary matrix from Rust ?
			}
			qk_circuit_instruction_clear(op);
		}

		for (auto m : circ.measure_map_) {
			measure_map_.push_back(m);
		}
	}

	/// @brief append a gate at the end of the circuit
	/// @param op a gate to be added
	/// @param qubits a list of qubits to be mapped
	/// @param params a list of parameters
	void append(const Instruction &op, const reg_t &qubits)
	{
		if (op.num_qubits() == qubits.size()) {
			std::vector<std::uint32_t> vqubits(qubits.size());
			for (uint_t i = 0; i < qubits.size(); i++) {
				vqubits[i] = (std::uint32_t)qubits[i];
			}
			pre_add_gate();
			if (op.is_standard_gate()) {
				if (op.num_params() > 0) {
					std::vector<QkParam*> params;
					for (auto &p : op.params()) {
						params.push_back(p.qiskit_param_.get());
					}
					qk_circuit_parameterized_gate(rust_circuit_.get(), op.gate_map(), vqubits.data(), params.data());
				}
				else
					qk_circuit_gate(rust_circuit_.get(), op.gate_map(), vqubits.data(), nullptr);
			} else {
				if (std::string("reset") == op.name()) {
					qk_circuit_reset(rust_circuit_.get(), vqubits[0]);
				}
				else if (std::string("barrier") == op.name()) {
					qk_circuit_barrier(rust_circuit_.get(), vqubits.data(), (uint32_t)vqubits.size());
				}
				else if (std::string("measure") == op.name()) {
					qk_circuit_measure(rust_circuit_.get(), vqubits[0], vqubits[0]);
					measure_map_.push_back(std::pair<uint_t, uint_t>(qubits[0], qubits[0]));
				}
			}
		}
	}

	void append(const Instruction &op, const std::vector<std::uint32_t> &qubits)
	{
		if (op.num_qubits() == qubits.size()) {
			pre_add_gate();
			if (op.is_standard_gate()) {
				if (op.num_params() > 0) {
					std::vector<QkParam*> params;
					for (auto &p : op.params()) {
						params.push_back(p.qiskit_param_.get());
					}
					qk_circuit_parameterized_gate(rust_circuit_.get(), op.gate_map(), qubits.data(), params.data());
				}
				else
					qk_circuit_gate(rust_circuit_.get(), op.gate_map(), qubits.data(), nullptr);
			} else {
				if (std::string("reset") == op.name()) {
					qk_circuit_reset(rust_circuit_.get(), qubits[0]);
				}
				else if (std::string("barrier") == op.name()) {
					qk_circuit_barrier(rust_circuit_.get(), qubits.data(), (uint32_t)qubits.size());
				}
				else if (std::string("measure") == op.name()) {
					qk_circuit_measure(rust_circuit_.get(), qubits[0], qubits[0]);
					measure_map_.push_back(std::pair<uint_t, uint_t>((uint_t)qubits[0], (uint_t)qubits[0]));
				}
			}
		}
	}

	/// @brief append a gate at the end of the circuit
	/// @param inst an instruction to be added
	void append(const CircuitInstruction &inst)
	{
		std::vector<std::uint32_t> vqubits(inst.qubits().size());
		for (uint_t i = 0; i < inst.qubits().size(); i++) {
			vqubits[i] = (std::uint32_t)inst.qubits()[i];
		}
		pre_add_gate();
		if (inst.instruction().is_standard_gate()) {
			if (inst.instruction().num_params() > 0) {
				std::vector<QkParam*> params;
				for (auto &p : inst.instruction().params()) {
					params.push_back(p.qiskit_param_.get());
				}
				qk_circuit_parameterized_gate(rust_circuit_.get(), inst.instruction().gate_map(), vqubits.data(), params.data());
			}
			else
				qk_circuit_gate(rust_circuit_.get(), inst.instruction().gate_map(), vqubits.data(), nullptr);
		} else {
			if (std::string("reset") == inst.instruction().name()) {
				qk_circuit_reset(rust_circuit_.get(), vqubits[0]);
			}
			else if (std::string("barrier") == inst.instruction().name()) {
				qk_circuit_barrier(rust_circuit_.get(), vqubits.data(), (uint32_t)vqubits.size());
			}
			else if (std::string("measure") == inst.instruction().name()) {
				qk_circuit_measure(rust_circuit_.get(), vqubits[0], (std::uint32_t)inst.clbits()[0]);
				measure_map_.push_back(std::pair<uint_t, uint_t>(inst.qubits()[0], inst.clbits()[0]));
			}
		}
	}

	void append(const Instruction &op, const uint_t qubit)
	{
		reg_t qubits({qubit});
		append(op, qubits);
	}

	/// @brief get number og instructions
	/// @return number of instructions in the circuit
	uint_t num_instructions(void)
	{
		return qk_circuit_num_instructions(rust_circuit_.get());
	}

	/// @brief get instruction
	/// @param i an index to the instruction
	/// @return the instruction at index i
	CircuitInstruction operator[](uint_t i)
	{
		if (i < qk_circuit_num_instructions(rust_circuit_.get())) {
			auto name_map = get_standard_gate_name_mapping();

			QkOperationKind kind = qk_circuit_instruction_kind(rust_circuit_.get(), i);
			QkCircuitInstruction *op = new QkCircuitInstruction;
			qk_circuit_get_instruction(rust_circuit_.get(), i, op);
			reg_t qubits;
			if (op->num_qubits > 0) {
				qubits.resize(op->num_qubits);
				for (int i = 0; i < op->num_qubits; i++)
					qubits[i] = op->qubits[i];
			}

			reg_t clbits;
			if (op->num_clbits > 0) {
				clbits.resize(op->num_clbits);
				for (int i = 0; i < op->num_clbits; i++)
					clbits[i] = op->clbits[i];
			}

			std::vector<Parameter> params;
			if (op->num_params > 0) {
				params.resize(op->num_params);
				for (int i = 0; i < op->num_params; i++)
					params[i] = Parameter(qk_param_copy(op->params[i]));
			}
			std::string name = op->name;
			qk_circuit_instruction_clear(op);

			if (kind == QkOperationKind_Measure) {
				auto inst = Measure();
				return CircuitInstruction(inst, qubits, clbits);
			} else if (kind == QkOperationKind_Reset) {
				auto inst = Reset();
				return CircuitInstruction(inst, qubits, clbits);
			} else if (kind == QkOperationKind_Barrier) {
				auto inst = Barrier();
				return CircuitInstruction(inst, qubits, clbits);
			}

			// standard gates
			auto gate = name_map.find(name);
			if (gate != name_map.end()) {
				auto inst = gate->second;
				if (params.size() > 0) {
					inst.set_params(params);
				}
				return CircuitInstruction(inst, qubits);
			}
		}
		return CircuitInstruction();
	}

	// qasm3

	/// @brief Serialize a QuantumCircuit object as an OpenQASM3 string.
	/// @return An OpenQASM3 string.
	std::string to_qasm3(void)
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
				switch (op)
				{
				case QkGate_R:
					qasm3 << "gate r(p0, p1) _gate_q_0 {" << std::endl;
					qasm3 << "  U(p0, -pi/2 + p1, pi/2 - p1) _gate_q_0;" << std::endl;
					qasm3 << "}" << std::endl;
					break;
				case QkGate_SXdg:
				case QkGate_RYY:
				case QkGate_XXPlusYY:
				case QkGate_XXMinusYY:
					if (!sxdg)
					{
						qasm3 << "gate sxdg _gate_q_0 {" << std::endl;
						qasm3 << "  s _gate_q_0;" << std::endl;
						qasm3 << "  h _gate_q_0;" << std::endl;
						qasm3 << "  s _gate_q_0;" << std::endl;
						qasm3 << "}" << std::endl;
						sxdg = true;
					}
					if (op == QkGate_RYY)
					{
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
					if (op == QkGate_XXPlusYY)
					{
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
					if (op == QkGate_XXMinusYY)
					{
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
					if (!cs)
					{
						qasm3 << "gate cs _gate_q_0, _gate_q_1 {" << std::endl;
						qasm3 << "  t _gate_q_0;" << std::endl;
						qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
						qasm3 << "  tdg _gate_q_1;" << std::endl;
						qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
						qasm3 << "  t _gate_q_1;" << std::endl;
						qasm3 << "}" << std::endl;
						cs = true;
					}
					if (op == QkGate_CSX)
					{
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
				case QkGate_RZZ:
					qasm3 << "gate rzz(p0) _gate_q_0, _gate_q_1 {" << std::endl;
					qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
					qasm3 << "  rz(p0) _gate_q_1;" << std::endl;
					qasm3 << "  cx _gate_q_0, _gate_q_1;" << std::endl;
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

		// save ops
		uint_t nops;
		nops = qk_circuit_num_instructions(rust_circuit_.get());

		// Declare registers
		// After transpilation, qubit registers will be mapped to physical registers,
		// so we need to combined them in a single quantum register "q";
		const std::string qreg_name = "q";
		qasm3 << "qubit[" << num_qubits() << "] " << qreg_name << ";" << std::endl;
		for(const auto& creg : cregs_) {
			qasm3 << "bit[" << creg.size() << "] " << creg.name() << ";" << std::endl;
		}

		auto recover_reg_data = [this](uint_t index) -> std::pair<std::string, uint_t>
		{
			auto it = std::upper_bound(cregs_.begin(), cregs_.end(), index,
					[](uint_t v, const ClassicalRegister& reg) { return v < reg.base_index(); });
			assert(it != cregs_.begin());
			it = std::prev(it);
			return std::make_pair(it->name(), index - it->base_index());
		};

		for (uint_t i = 0; i < nops; i++) {
			QkCircuitInstruction *op = new QkCircuitInstruction;
			qk_circuit_get_instruction(rust_circuit_.get(), i, op);
			if (op->num_clbits > 0) {
				if (op->num_qubits == op->num_clbits) {
					for (uint_t j = 0; j < op->num_qubits; j++) {
						const auto creg_data = recover_reg_data(op->clbits[j]);
						qasm3 << creg_data.first << "[" << creg_data.second << "] = " << op->name << " " << qreg_name << "[" << op->qubits[j] << "];" << std::endl;
					}
				}
			} else {
				if (strcmp(op->name, "u") == 0) {
					qasm3 << "U";
				} else {
					qasm3 << op->name;
				}
				if (op->num_params > 0) {
					qasm3 << "(";
					for (uint_t j = 0; j < op->num_params; j++) {
						char* param = qk_param_str(op->params[j]);
						qasm3 << param;
						qk_str_free(param);
						if (j != op->num_params - 1)
							qasm3 << ", ";
					}
					qasm3 << ")";
				}
				if (op->num_qubits > 0) {
					qasm3 << " ";
					for (uint_t j = 0; j < op->num_qubits; j++) {
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

	/// @brief print circuit (this is for debug)
	void print(void) const
	{
		uint_t nops;
		nops = qk_circuit_num_instructions(rust_circuit_.get());

		for (uint_t i = 0; i < nops; i++) {
			QkOperationKind kind = qk_circuit_instruction_kind(rust_circuit_.get(), i);
			QkCircuitInstruction *op = new QkCircuitInstruction;
			qk_circuit_get_instruction(rust_circuit_.get(), i, op);
			std::cout << op->name;
			if (op->num_qubits > 0) {
				std::cout << "(";
				for (uint_t j = 0; j < op->num_qubits; j++) {
					std::cout << op->qubits[j];
					if (j != op->num_qubits - 1)
						std::cout << ", ";
				}
				std::cout << ") ";
			}
			if (op->num_clbits > 0) {
				std::cout << "(";
				for (uint_t j = 0; j < op->num_clbits; j++) {
					std::cout << op->clbits[j];
					if (j != op->num_clbits - 1)
						std::cout << ", ";
				}
				std::cout << ") ";
			}
			if (op->num_params > 0) {
				std::cout << "[";
				for (uint_t j = 0; j < op->num_params; j++) {
					char* param = qk_param_str(op->params[j]);
					std::cout << param;
					qk_str_free(param);
					if (j != op->num_params - 1)
						std::cout << ", ";
				}
				std::cout << "]";
			}
			std::cout << std::endl;
			qk_circuit_instruction_clear(op);
		}
	}

	/// @brief draw the circuit
	void draw(void) const
	{
		QkCircuitDrawerConfig conf;
		conf.bundle_cregs = true;
		conf.merge_wires = false;
		conf.fold = 0;

		char* out = qk_circuit_draw(rust_circuit_.get(), &conf);
		std::cout << out << std::endl;
		qk_str_free(out);
	}

	/// @brief compare two circuits
	/// @param other a circuit to be compared with this circuit
	/// @return true if two circuits are the same
	bool operator==(const QuantumCircuit& other) const
	{
		if (global_phase_ != other.global_phase_) {
			return false;
		}

		// compare instructions
		uint_t nops;
		uint_t nops_other;
		nops = qk_circuit_num_instructions(rust_circuit_.get());
		nops_other = qk_circuit_num_instructions(other.rust_circuit_.get());
		if (nops != nops_other) {
			return false;
		}

		for (uint_t i = 0; i < nops; i++) {
			QkCircuitInstruction *op = new QkCircuitInstruction;
			QkCircuitInstruction *op_other = new QkCircuitInstruction;
			qk_circuit_get_instruction(rust_circuit_.get(), i, op);
			qk_circuit_get_instruction(other.rust_circuit_.get(), i, op_other);

			if (std::string(op->name) != std::string(op_other->name)) {
				qk_circuit_instruction_clear(op);
				qk_circuit_instruction_clear(op_other);
				return false;
			}
			if (op->num_qubits != op_other->num_qubits || op->num_clbits != op_other->num_clbits || op->num_params != op_other->num_params) {
				qk_circuit_instruction_clear(op);
				qk_circuit_instruction_clear(op_other);
				return false;
			}
			for (int j = 0; j < op->num_qubits; j++) {
				if (op->qubits[j] != op_other->qubits[j]) {
					qk_circuit_instruction_clear(op);
					qk_circuit_instruction_clear(op_other);
					return false;
				}
			}
			for (int j = 0; j < op->num_params; j++) {
				QkParam* sub = qk_param_zero();
				qk_param_sub(sub, op->params[j], op_other->params[j]);
				double diff = qk_param_as_real(sub);
				qk_param_free(sub);
				if (fabs(diff) > QC_COMPARE_EPS) {
					qk_circuit_instruction_clear(op);
					qk_circuit_instruction_clear(op_other);
					return false;
				}
			}
			for (int j = 0; j < op->num_clbits; j++) {
				if (op->clbits[j] != op_other->clbits[j]) {
					qk_circuit_instruction_clear(op);
					qk_circuit_instruction_clear(op_other);
					return false;
				}
			}
		}
		return true;
	}

	bool operator!=(const QuantumCircuit& other) const
	{
		return !(*this == other);
	}
protected:
	void add_pending_control_flow_op(void);

	void pre_add_gate(void)
	{
		add_pending_control_flow_op();
	}

	void get_qubits(reg_t &bits)
	{
		bits.clear();
		bits.reserve(num_qubits_);
		for (uint_t i = 0; i < qregs_.size(); i++) {
			for (uint_t j = 0; j < qregs_[i].size(); j++) {
				bits.push_back(qregs_[i][j]);
			}
		}
	}

	void get_clbits(reg_t &bits)
	{
		bits.clear();
		bits.reserve(num_clbits_);
		for (uint_t i = 0; i < cregs_.size(); i++) {
			for (uint_t j = 0; j < cregs_[i].size(); j++) {
				bits.push_back(cregs_[i][j]);
			}
		}
	}
};

} // namespace circuit
} // namespace Qiskit

#endif // __qiskitcpp_circuit_quantum_circuit_def_hpp__
