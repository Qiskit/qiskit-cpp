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
#include <cctype>
#include <cstring>
#include <iomanip>
#include <map>
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

	static bool is_identifier_start(unsigned char c)
	{
		return std::isalpha(c) || c == '_';
	}

	static bool is_identifier_char(unsigned char c)
	{
		return std::isalnum(c) || c == '_';
	}

	static std::string sanitize_identifier(const std::string &preferred, const std::string &fallback_base)
	{
		const auto &source = preferred.empty() ? fallback_base : preferred;
		std::string sanitized;
		for (const auto ch : source) {
			const auto value = static_cast<unsigned char>(ch);
			sanitized.push_back(is_identifier_char(value) ? ch : '_');
		}
		if (sanitized.empty()) {
			sanitized = fallback_base.empty() ? "id" : fallback_base;
		}
		if (!is_identifier_start(static_cast<unsigned char>(sanitized[0]))) {
			sanitized.insert(sanitized.begin(), '_');
		}
		return sanitized;
	}

	class IdentifierAllocator
	{
	private:
		std::set<std::string> used_;

	public:
		void reserve(const std::string &name)
		{
			used_.insert(name);
		}

		std::string allocate(const std::string &preferred, const std::string &fallback_base)
		{
			const auto base = sanitize_identifier(preferred, fallback_base);
			if (used_.find(base) == used_.end()) {
				used_.insert(base);
				return base;
			}
			for (std::size_t suffix = 0;; suffix++) {
				const auto candidate = base + "_" + std::to_string(suffix);
				if (used_.find(candidate) == used_.end()) {
					used_.insert(candidate);
					return candidate;
				}
			}
		}
	};

	struct ExportContext
	{
		std::vector<std::string> parameter_symbols;
		std::vector<std::string> parameter_names;
		std::map<std::string, std::string> parameter_name_map;
		std::string qreg_name;
		std::vector<std::string> creg_names;
	};

	static bool is_parameter_function_name(const std::string &name)
	{
		static const char *function_names[] = {
			"abs", "acos", "asin", "atan", "conj", "conjugate", "cos",
			"exp", "log", "sign", "sin", "sqrt", "tan"
		};
		return std::find(function_names, function_names + sizeof(function_names) / sizeof(function_names[0]), name)
			!= function_names + sizeof(function_names) / sizeof(function_names[0]);
	}

	static bool is_function_call(const std::string &expression, std::size_t token_end, const std::string &token)
	{
		if (!is_parameter_function_name(token)) {
			return false;
		}
		while (token_end < expression.size() && std::isspace(static_cast<unsigned char>(expression[token_end]))) {
			token_end++;
		}
		return token_end < expression.size() && expression[token_end] == '(';
	}

	void reserve_openqasm_identifiers(IdentifierAllocator &allocator) const
	{
		static const char *reserved_words[] = {
			"OPENQASM", "include", "def", "extern", "box", "let", "break",
			"continue", "if", "else", "end", "return", "for", "while", "in",
			"cal", "defcal", "defcalgrammar", "gphase", "inv", "pow", "ctrl",
			"negctrl", "durationof", "sizeof", "stretch",
			"input", "output", "const", "readonly", "mutable", "qreg", "qubit",
			"creg", "bool", "bit", "int", "uint", "float", "angle", "complex",
			"array", "void", "duration", "delay", "reset", "measure", "barrier",
			"gate", "true", "false", "pi", "tau", "euler", "U"
		};
		for (const auto word : reserved_words) {
			allocator.reserve(word);
		}

		static const char *function_names[] = {
			"abs", "acos", "asin", "atan", "conj", "conjugate", "cos",
			"exp", "log", "sign", "sin", "sqrt", "tan"
		};
		for (const auto name : function_names) {
			allocator.reserve(name);
		}

		for (const auto &entry : circuit::get_standard_gate_name_mapping()) {
			allocator.reserve(entry.first);
		}
		allocator.reserve("_gate_q_0");
		allocator.reserve("_gate_q_1");
		allocator.reserve("_gate_q_2");
		allocator.reserve("_gate_q_3");
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
			const auto first = static_cast<unsigned char>(name[0]);
			if (!is_identifier_start(first)) {
				throw std::invalid_argument("invalid OpenQASM identifier: " + name);
			}
			for (const auto ch : name) {
				const auto value = static_cast<unsigned char>(ch);
				if (!is_identifier_char(value)) {
					throw std::invalid_argument("invalid OpenQASM identifier: " + name);
				}
			}
		}
	}

	ExportContext build_export_context(
		circuit::QuantumCircuit &circuit,
		const std::vector<std::string> &parameter_names,
		bool explicit_parameter_names) const
	{
		if (explicit_parameter_names) {
			validate_parameter_names(circuit, parameter_names);
		}

		ExportContext context;
		context.parameter_symbols = circuit.parameter_symbols();
		const auto num_symbols = qk_circuit_num_param_symbols(circuit.rust_circuit_.get());
		if (context.parameter_symbols.size() != static_cast<std::size_t>(num_symbols)) {
			throw std::invalid_argument(
				"Qiskit C++ cannot recover unique circuit parameter symbols until "
				"the Qiskit C API can enumerate parameter names.");
		}

		IdentifierAllocator allocator;
		reserve_openqasm_identifiers(allocator);

		context.parameter_names.reserve(context.parameter_symbols.size());
		for (std::size_t i = 0; i < context.parameter_symbols.size(); i++) {
			const auto &preferred = explicit_parameter_names ? parameter_names[i] : context.parameter_symbols[i];
			const auto emitted = allocator.allocate(preferred, "theta");
			if (explicit_parameter_names && emitted != preferred) {
				throw std::invalid_argument("OpenQASM 3 explicit parameter name conflicts with a reserved identifier: " + preferred);
			}
			context.parameter_names.push_back(emitted);
			context.parameter_name_map[context.parameter_symbols[i]] = emitted;
		}

		const bool physical_qubits = !circuit.qubit_map_.empty();
		if (!physical_qubits) {
			context.qreg_name = allocator.allocate("q", "q");
		}

		context.creg_names.resize(circuit.cregs_.size());
		for (std::size_t i = 0; i < circuit.cregs_.size(); i++) {
			if (circuit.cregs_[i].size() == 0) {
				continue;
			}
			context.creg_names[i] = allocator.allocate(circuit.cregs_[i].name(), "c");
		}
		return context;
	}

	std::string format_parameter_expression(
		const std::string &expression,
		const ExportContext &context) const
	{
		std::string formatted;
		for (std::size_t i = 0; i < expression.size();) {
			const auto c = static_cast<unsigned char>(expression[i]);
			if (!is_identifier_start(c)) {
				formatted.push_back(expression[i]);
				i++;
				continue;
			}

			const std::size_t start = i;
			i++;
			while (i < expression.size() && is_identifier_char(static_cast<unsigned char>(expression[i]))) {
				i++;
			}

			const auto token = expression.substr(start, i - start);
			const auto mapped = context.parameter_name_map.find(token);
			if (mapped != context.parameter_name_map.end() && !is_function_call(expression, i, token)) {
				formatted += mapped->second;
			} else {
				formatted += token;
			}
		}
		return formatted;
	}

	void emit_operation_parameters(
		std::stringstream &qasm3,
		QkCircuitInstruction *op,
		const ExportContext &context) const
	{
		if (op->num_params == 0) {
			return;
		}
		qasm3 << "(";
		for (uint_t j = 0; j < op->num_params; j++) {
			ParamString param(op->params[j]);
			qasm3 << format_parameter_expression(param.c_str(), context);
			if (j != op->num_params - 1)
				qasm3 << ", ";
		}
		qasm3 << ")";
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
					qasm3 << "  cp(p0) _gate_q_0, _gate_q_1;" << std::endl;
					qasm3 << "}" << std::endl;
					break;
				case QkGate_CU3:
					qasm3 << "gate cu3(p0, p1, p2) _gate_q_0, _gate_q_1 {" << std::endl;
					qasm3 << "  cu(p0, p1, p2, 0) _gate_q_0, _gate_q_1;" << std::endl;
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

	void emit_registers_and_operations(std::stringstream &qasm3, circuit::QuantumCircuit &circuit, const ExportContext &context) const
	{
		const uint_t nops = qk_circuit_num_instructions(circuit.rust_circuit_.get());

		const bool physical_qubits = !circuit.qubit_map_.empty();
		auto qubit_ref = [physical_qubits, &context](uint_t index) -> std::string
		{
			if (physical_qubits) {
				return std::string("$") + std::to_string(index);
			}
			return context.qreg_name + "[" + std::to_string(index) + "]";
		};

		if (!physical_qubits) {
			qasm3 << "qubit[" << circuit.num_qubits() << "] " << context.qreg_name << ";" << std::endl;
		}
		for (std::size_t i = 0; i < circuit.cregs_.size(); i++) {
			if (circuit.cregs_[i].size() == 0) {
				continue;
			}
			qasm3 << "bit[" << circuit.cregs_[i].size() << "] " << context.creg_names[i] << ";" << std::endl;
		}

		auto recover_reg_data = [&circuit, &context](uint_t index) -> std::pair<std::string, uint_t>
		{
			auto it = std::upper_bound(circuit.cregs_.begin(), circuit.cregs_.end(), index,
					[](uint_t v, const circuit::ClassicalRegister &reg) { return v < reg.base_index(); });
			assert(it != circuit.cregs_.begin());
			it = std::prev(it);
			const auto reg_index = static_cast<std::size_t>(std::distance(circuit.cregs_.begin(), it));
			return std::make_pair(context.creg_names[reg_index], index - it->base_index());
		};

		for (uint_t i = 0; i < nops; i++) {
			CircuitInstruction instruction(circuit.rust_circuit_.get(), i);
			auto op = instruction.get();
			const auto kind = qk_circuit_instruction_kind(circuit.rust_circuit_.get(), i);
			if (op->num_clbits > 0) {
				if (kind != QkOperationKind_Measure || op->num_qubits != op->num_clbits) {
					throw std::runtime_error(
						std::string("OpenQASM 3 export does not support clbit operation: ") + op->name);
				}
				for (uint_t j = 0; j < op->num_qubits; j++) {
					const auto creg_data = recover_reg_data(op->clbits[j]);
					qasm3 << creg_data.first << "[" << creg_data.second << "] = measure " << qubit_ref(op->qubits[j]) << ";" << std::endl;
				}
			} else {
				if (std::strcmp(op->name, "global_phase") == 0) {
					qasm3 << "gphase";
					emit_operation_parameters(qasm3, op, context);
					qasm3 << ";" << std::endl;
					continue;
				}
				if (std::strcmp(op->name, "u") == 0) {
					qasm3 << "U";
				} else {
					qasm3 << op->name;
				}
				emit_operation_parameters(qasm3, op, context);
				if (op->num_qubits > 0) {
					qasm3 << " ";
					for (uint_t j = 0; j < op->num_qubits; j++) {
						qasm3 << qubit_ref(op->qubits[j]);
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
		return dumps(circuit, {}, false);
	}

	std::string dumps(circuit::QuantumCircuit &circuit, const std::vector<std::string> &parameter_names) const
	{
		circuit.add_pending_control_flow_op();
		return dumps(circuit, parameter_names, true);
	}

private:
	std::string dumps(circuit::QuantumCircuit &circuit, const std::vector<std::string> &parameter_names, bool explicit_parameter_names) const
	{
		const auto context = build_export_context(circuit, parameter_names, explicit_parameter_names);
		std::stringstream qasm3;
		qasm3 << std::setprecision(18);
		qasm3 << "OPENQASM 3.0;" << std::endl;
		qasm3 << "include \"stdgates.inc\";" << std::endl;

		emit_custom_gate_definitions(qasm3, circuit);
		emit_parameter_declarations(qasm3, context.parameter_names);
		emit_registers_and_operations(qasm3, circuit, context);

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
