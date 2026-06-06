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

// OpenQASM 3 exporter

#ifndef __qiskitcpp_qasm3_qasm3_exporter_hpp__
#define __qiskitcpp_qasm3_qasm3_exporter_hpp__

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "circuit/quantumcircuit_def.hpp"

namespace Qiskit
{
namespace qasm3
{

class Qasm3Exporter
{
private:
	class OpCounts
	{
	private:
		QkOpCounts counts_;

	public:
		explicit OpCounts(QkCircuit *circuit) : counts_(qk_circuit_count_ops(circuit)) {}

		~OpCounts()
		{
			qk_opcounts_clear(&counts_);
		}

		OpCounts(const OpCounts &) = delete;
		OpCounts &operator=(const OpCounts &) = delete;

		const QkOpCounts &get(void) const
		{
			return counts_;
		}
	};

	class CircuitInstruction
	{
	private:
		QkCircuitInstruction instruction_;

	public:
		CircuitInstruction(QkCircuit *circuit, uint_t index)
		{
			qk_circuit_get_instruction(circuit, index, &instruction_);
		}

		~CircuitInstruction()
		{
			qk_circuit_instruction_clear(&instruction_);
		}

		CircuitInstruction(const CircuitInstruction &) = delete;
		CircuitInstruction &operator=(const CircuitInstruction &) = delete;

		QkCircuitInstruction *get(void)
		{
			return &instruction_;
		}
	};

	class ParamString
	{
	private:
		char *value_;

	public:
		explicit ParamString(QkParam *param) : value_(qk_param_str(param)) {}

		~ParamString()
		{
			qk_str_free(value_);
		}

		ParamString(const ParamString &) = delete;
		ParamString &operator=(const ParamString &) = delete;

		const char *c_str(void) const
		{
			return value_;
		}
	};

	class ParameterSymbolName
	{
	private:
		char *value_;

	public:
		ParameterSymbolName(QkCircuit *circuit, std::size_t index) : value_(qk_circuit_param_symbol_name(circuit, index)) {}

		~ParameterSymbolName()
		{
			qk_str_free(value_);
		}

		ParameterSymbolName(const ParameterSymbolName &) = delete;
		ParameterSymbolName &operator=(const ParameterSymbolName &) = delete;

		const char *c_str(void) const
		{
			return value_;
		}
	};

	std::vector<std::string> default_parameter_names(circuit::QuantumCircuit &circuit) const
	{
		const auto num_symbols = qk_circuit_num_param_symbols(circuit.rust_circuit_.get());
		std::vector<std::string> names;
		names.reserve(num_symbols);
		for (std::size_t i = 0; i < num_symbols; i++) {
			ParameterSymbolName name(circuit.rust_circuit_.get(), i);
			if (name.c_str() == nullptr) {
				throw std::runtime_error("OpenQASM 3 export failed to read circuit parameter name");
			}
			names.emplace_back(name.c_str());
		}
		return names;
	}

	void validate_parameter_names(circuit::QuantumCircuit &circuit, const std::vector<std::string> &parameter_names) const
	{
		const auto num_symbols = qk_circuit_num_param_symbols(circuit.rust_circuit_.get());
		if (parameter_names.size() != static_cast<std::size_t>(num_symbols)) {
			throw std::invalid_argument("OpenQASM 3 parameter-name count does not match circuit free-parameter count");
		}

		std::set<std::string> seen;
		for (const auto &name : parameter_names) {
			if (name.empty()) {
				throw std::invalid_argument("OpenQASM 3 parameter names cannot be empty");
			}
			if (!seen.insert(name).second) {
				throw std::invalid_argument("OpenQASM 3 parameter names cannot contain duplicates");
			}
		}
	}

	void emit_custom_gate_definitions(std::stringstream &qasm3, circuit::QuantumCircuit &circuit) const
	{
		auto name_map = circuit::get_standard_gate_name_mapping();
		bool cs = false;
		bool sxdg = false;
		OpCounts opcounts(circuit.rust_circuit_.get());
		for (int i = 0; i < opcounts.get().len; i++) {
			if (opcounts.get().data[i].count != 0) {
				auto op = name_map[opcounts.get().data[i].name].gate_map();
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
	}

	void emit_parameter_declarations(std::stringstream &qasm3, const std::vector<std::string> &parameter_names) const
	{
		for (const auto &symbol : parameter_names) {
			qasm3 << "input float[64] " << symbol << ";" << std::endl;
		}
	}

	void emit_registers_and_operations(std::stringstream &qasm3, circuit::QuantumCircuit &circuit) const
	{
		const uint_t nops = qk_circuit_num_instructions(circuit.rust_circuit_.get());

		const std::string qreg_name = "q";
		qasm3 << "qubit[" << circuit.num_qubits() << "] " << qreg_name << ";" << std::endl;
		for (const auto &creg : circuit.cregs_) {
			qasm3 << "bit[" << creg.size() << "] " << creg.name() << ";" << std::endl;
		}

		auto recover_reg_data = [&circuit](uint_t index) -> std::pair<std::string, uint_t>
		{
			auto it = std::upper_bound(circuit.cregs_.begin(), circuit.cregs_.end(), index,
					[](uint_t v, const circuit::ClassicalRegister &reg) { return v < reg.base_index(); });
			assert(it != circuit.cregs_.begin());
			it = std::prev(it);
			return std::make_pair(it->name(), index - it->base_index());
		};

		for (uint_t i = 0; i < nops; i++) {
			CircuitInstruction instruction(circuit.rust_circuit_.get(), i);
			auto op = instruction.get();
			if (op->num_clbits > 0) {
				if (op->num_qubits == op->num_clbits) {
					for (uint_t j = 0; j < op->num_qubits; j++) {
						const auto creg_data = recover_reg_data(op->clbits[j]);
						qasm3 << creg_data.first << "[" << creg_data.second << "] = " << op->name << " " << qreg_name << "[" << op->qubits[j] << "];" << std::endl;
					}
				}
			} else {
				if (std::strcmp(op->name, "u") == 0) {
					qasm3 << "U";
				} else {
					qasm3 << op->name;
				}
				if (op->num_params > 0) {
					qasm3 << "(";
					for (uint_t j = 0; j < op->num_params; j++) {
						ParamString param(op->params[j]);
						qasm3 << param.c_str();
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
		}
	}

public:
	std::string dumps(circuit::QuantumCircuit &circuit) const
	{
		circuit.add_pending_control_flow_op();
		return dumps(circuit, default_parameter_names(circuit));
	}

	std::string dumps(circuit::QuantumCircuit &circuit, const std::vector<std::string> &parameter_names) const
	{
		circuit.add_pending_control_flow_op();
		validate_parameter_names(circuit, parameter_names);

		std::stringstream qasm3;
		qasm3 << std::setprecision(18);
		qasm3 << "OPENQASM 3.0;" << std::endl;
		qasm3 << "include \"stdgates.inc\";" << std::endl;

		emit_custom_gate_definitions(qasm3, circuit);
		emit_parameter_declarations(qasm3, parameter_names);
		emit_registers_and_operations(qasm3, circuit);

		return qasm3.str();
	}
};

inline std::string dumps(circuit::QuantumCircuit &circuit)
{
	return Qasm3Exporter().dumps(circuit);
}

inline std::string dumps(circuit::QuantumCircuit &circuit, const std::vector<std::string> &parameter_names)
{
	return Qasm3Exporter().dumps(circuit, parameter_names);
}

} // namespace qasm3
} // namespace Qiskit

#endif // __qiskitcpp_qasm3_qasm3_exporter_hpp__
