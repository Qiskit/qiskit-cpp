/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2024, 2026.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.
*/

#ifndef __qiskitcpp_qasm3_exporter_hpp__
#define __qiskitcpp_qasm3_exporter_hpp__

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "circuit/quantumcircuit.hpp"

namespace Qiskit {
namespace qasm3 {
namespace detail {

using circuit::ClassicalRegister;
using circuit::get_standard_gate_name_mapping;

class CircuitInstruction {
private:
    QkCircuitInstruction instruction_{};

public:
    CircuitInstruction() = default;
    CircuitInstruction(const CircuitInstruction&) = delete;
    CircuitInstruction& operator=(const CircuitInstruction&) = delete;

    ~CircuitInstruction()
    {
        qk_circuit_instruction_clear(&instruction_);
    }

    QkCircuitInstruction* get(void)
    {
        return &instruction_;
    }

    QkCircuitInstruction& value(void)
    {
        return instruction_;
    }
};

class OpCounts {
private:
    QkOpCounts opcounts_;

public:
    explicit OpCounts(QkCircuit* circuit)
        : opcounts_(qk_circuit_count_ops(circuit))
    {}

    OpCounts(const OpCounts&) = delete;
    OpCounts& operator=(const OpCounts&) = delete;

    ~OpCounts()
    {
        qk_opcounts_clear(&opcounts_);
    }

    QkOpCounts* operator->(void)
    {
        return &opcounts_;
    }
};

class Exporter {
private:
    circuit::QuantumCircuit& circuit_;
    const std::vector<std::string>& input_parameters_;

    void validate_input_parameters(void) const
    {
        if (input_parameters_.size() != circuit_.num_parameters()) {
            throw std::invalid_argument("The number of QASM3 input parameter names must match the circuit parameter count");
        }

        std::unordered_set<std::string> names;
        for (const auto& name : input_parameters_) {
            if (name.empty()) {
                throw std::invalid_argument("QASM3 input parameter names cannot be empty");
            }
            if (!names.insert(name).second) {
                throw std::invalid_argument("QASM3 input parameter names must be unique");
            }
        }
    }

public:
    Exporter(circuit::QuantumCircuit& circuit, const std::vector<std::string>& input_parameters)
        : circuit_(circuit), input_parameters_(input_parameters)
    {}

    std::string run(void)
    {
        auto rust_circuit_ = circuit_.get_rust_circuit();
        validate_input_parameters();
        const auto& cregs_ = circuit_.cregs();
        auto num_qubits = [this]() { return circuit_.num_qubits(); };

	std::stringstream qasm3;
	qasm3 << std::setprecision(18);
	qasm3 << "OPENQASM 3.0;" << std::endl;
	qasm3 << "include \"stdgates.inc\";" << std::endl;
	for (const auto& input_parameter : input_parameters_) {
		qasm3 << "input float[64] " << input_parameter << ";" << std::endl;
	}
	auto name_map = get_standard_gate_name_mapping();
	// add header for non-standard gates
	bool cs = false;
	bool sxdg = false;
	OpCounts opcounts(rust_circuit_.get());
	for (int i = 0; i < opcounts->len; i++) {
		if (opcounts->data[i].count != 0) {
			auto op = name_map[opcounts->data[i].name].gate_map();
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

	auto recover_reg_data = [&cregs_](uint_t index) -> std::pair<std::string, uint_t>
	{
		auto it = std::upper_bound(cregs_.begin(), cregs_.end(), index,
				[](uint_t v, const ClassicalRegister& reg) { return v < reg.base_index(); });
		assert(it != cregs_.begin());
		it = std::prev(it);
		return std::make_pair(it->name(), index - it->base_index());
	};

	for (uint_t i = 0; i < nops; i++) {
		CircuitInstruction instruction;
		auto& op = instruction.value();
		qk_circuit_get_instruction(rust_circuit_.get(), i, instruction.get());
		if (op.num_clbits > 0) {
			if (op.num_qubits == op.num_clbits) {
				for (uint_t j = 0; j < op.num_qubits; j++) {
					const auto creg_data = recover_reg_data(op.clbits[j]);
					qasm3 << creg_data.first << "[" << creg_data.second << "] = " << op.name << " " << qreg_name << "[" << op.qubits[j] << "];" << std::endl;
				}
			}
		} else {
			if (strcmp(op.name, "u") == 0) {
				qasm3 << "U";
			} else {
				qasm3 << op.name;
			}
			if (op.num_params > 0) {
				qasm3 << "(";
				for (uint_t j = 0; j < op.num_params; j++) {
					char* param = qk_param_str(op.params[j]);
					qasm3 << param;
					qk_str_free(param);
					if (j != op.num_params - 1)
						qasm3 << ", ";
				}
				qasm3 << ")";
			}
			if (op.num_qubits > 0) {
				qasm3 << " ";
				for (uint_t j = 0; j < op.num_qubits; j++) {
					qasm3 << qreg_name << "[" << op.qubits[j] << "]";
					if (j != op.num_qubits - 1)
						qasm3 << ", ";
				}
			}
			qasm3 << ";" << std::endl;
		}
	}

	return qasm3.str();
    }
};

} // namespace detail

/// @brief Serialize a QuantumCircuit object as an OpenQASM3 string.
///
/// The Qiskit C API exposes the number of free parameter symbols in a circuit,
/// but cannot enumerate their names. Callers must therefore supply the names
/// used by parameter expressions in the circuit.
/// @param circuit The circuit to serialize.
/// @param input_parameters Ordered names for the circuit's free input parameters. Names must match
/// the symbols used by the circuit and be valid OpenQASM3 identifiers.
/// @return An OpenQASM3 string.
inline std::string dumps(circuit::QuantumCircuit& circuit, const std::vector<std::string>& input_parameters)
{
    return detail::Exporter(circuit, input_parameters).run();
}

/// @brief Serialize a QuantumCircuit without free parameters as an OpenQASM3 string.
/// @param circuit The circuit to serialize.
/// @return An OpenQASM3 string.
inline std::string dumps(circuit::QuantumCircuit& circuit)
{
    return dumps(circuit, {});
}

} // namespace qasm3
} // namespace Qiskit

#endif // __qiskitcpp_qasm3_exporter_hpp__
