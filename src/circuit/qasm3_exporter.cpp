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

// Implementation of `Qasm3Exporter` and `QuantumCircuit::to_qasm3`
// (refactored out of `QuantumCircuit::to_qasm3` per issue #146).

#include "circuit/qasm3_exporter.hpp"
#include "circuit/quantumcircuit_def.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "circuit/classicalregister.hpp"
#include "circuit/library/standard_gates/standard_gates.hpp"

namespace Qiskit {
namespace circuit {

Qasm3Exporter::Qasm3Exporter(QuantumCircuit *circ) : circ_(circ) {}

std::string Qasm3Exporter::run(void) {
    std::stringstream out;
    out << std::setprecision(18);
    write_header(out);
    write_gate_definitions(out);
    write_registers(out);
    write_instructions(out);
    return out.str();
}

rust_circuit *Qasm3Exporter::rust_circuit_ptr(void) const {
    return circ_->get_rust_circuit(false).get();
}

std::pair<std::string, uint_t> Qasm3Exporter::recover_creg_data(
    const std::vector<ClassicalRegister> &cregs, uint_t index) {
    auto it = std::upper_bound(cregs.begin(), cregs.end(), index,
            [](uint_t v, const ClassicalRegister& reg) { return v < reg.base_index(); });
    assert(it != cregs.begin());
    it = std::prev(it);
    return std::make_pair(it->name(), index - it->base_index());
}

std::string Qasm3Exporter::param_to_string(const QkParam *param) {
    char *raw = qk_param_str(param);
    std::string result;
    if (raw != nullptr) {
        result = raw;
        qk_str_free(raw);
    }
    return result;
}

void Qasm3Exporter::write_header(std::stringstream &out) const {
    out << "OPENQASM 3.0;" << std::endl;
    out << "include \"stdgates.inc\";" << std::endl;
}

void Qasm3Exporter::write_gate_definitions(std::stringstream &out) {
    auto name_map = get_standard_gate_name_mapping();
    bool cs = false;
    bool sxdg = false;

    QkOpCounts opcounts = qk_circuit_count_ops(rust_circuit_ptr());
    for (int i = 0; i < opcounts.len; i++) {
        if (opcounts.data[i].count == 0) {
            continue;
        }
        const auto op = name_map[opcounts.data[i].name].gate_map();
        switch (op) {
        case QkGate_R:
            out << "gate r(p0, p1) _gate_q_0 {" << std::endl;
            out << "  U(p0, -pi/2 + p1, pi/2 - p1) _gate_q_0;" << std::endl;
            out << "}" << std::endl;
            break;
        case QkGate_SXdg:
        case QkGate_RYY:
        case QkGate_XXPlusYY:
        case QkGate_XXMinusYY:
            if (!sxdg) {
                out << "gate sxdg _gate_q_0 {" << std::endl;
                out << "  s _gate_q_0;" << std::endl;
                out << "  h _gate_q_0;" << std::endl;
                out << "  s _gate_q_0;" << std::endl;
                out << "}" << std::endl;
                sxdg = true;
            }
            if (op == QkGate_RYY) {
                out << "gate ryy(p0) _gate_q_0, _gate_q_1 {" << std::endl;
                out << "  sxdg _gate_q_0;" << std::endl;
                out << "  sxdg _gate_q_1;" << std::endl;
                out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
                out << "  rz(p0) _gate_q_1;" << std::endl;
                out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
                out << "  sx _gate_q_0;" << std::endl;
                out << "  sx _gate_q_1;" << std::endl;
                out << "}" << std::endl;
            }
            if (op == QkGate_XXPlusYY) {
                out << "gate xx_plus_yy(p0, p1) _gate_q_0, _gate_q_1 {" << std::endl;
                out << "  rz(p1) _gate_q_0;" << std::endl;
                out << "  sdg _gate_q_1;" << std::endl;
                out << "  sx _gate_q_1;" << std::endl;
                out << "  s _gate_q_1;" << std::endl;
                out << "  s _gate_q_0;" << std::endl;
                out << "  cx _gate_q_1, _gate_q_0;" << std::endl;
                out << "  ry((-0.5)*p0) _gate_q_1;" << std::endl;
                out << "  ry((-0.5)*p0) _gate_q_0;" << std::endl;
                out << "  cx _gate_q_1, _gate_q_0;" << std::endl;
                out << "  sdg _gate_q_0;" << std::endl;
                out << "  sdg _gate_q_1;" << std::endl;
                out << "  sxdg _gate_q_1;" << std::endl;
                out << "  s _gate_q_1;" << std::endl;
                out << "  rz(-p1) _gate_q_0;" << std::endl;
                out << "}" << std::endl;
            }
            if (op == QkGate_XXMinusYY) {
                out << "gate xx_minus_yy(p0, p1) _gate_q_0, _gate_q_1 {" << std::endl;
                out << "  rz(-p1) _gate_q_1;" << std::endl;
                out << "  sdg _gate_q_0;" << std::endl;
                out << "  sx _gate_q_0;" << std::endl;
                out << "  s _gate_q_0;" << std::endl;
                out << "  s _gate_q_1;" << std::endl;
                out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
                out << "  ry(0.5*p0) _gate_q_0;" << std::endl;
                out << "  ry((-0.5)*p0) _gate_q_1;" << std::endl;
                out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
                out << "  sdg _gate_q_1;" << std::endl;
                out << "  sdg _gate_q_0;" << std::endl;
                out << "  sxdg _gate_q_0;" << std::endl;
                out << "  s _gate_q_0;" << std::endl;
                out << "  rz(p1) _gate_q_1;" << std::endl;
                out << "}" << std::endl;
            }
            break;
        case QkGate_DCX:
            out << "gate dcx _gate_q_0, _gate_q_1 {" << std::endl;
            out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            out << "  cx _gate_q_1, _gate_q_0;" << std::endl;
            out << "}" << std::endl;
            break;
        case QkGate_ECR:
            out << "gate ecr _gate_q_0, _gate_q_1 {" << std::endl;
            out << "  s _gate_q_0;" << std::endl;
            out << "  sx _gate_q_1;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            out << "  x _gate_q_0;" << std::endl;
            out << "}" << std::endl;
            break;
        case QkGate_ISwap:
            out << "gate iswap _gate_q_0, _gate_q_1 {" << std::endl;
            out << "  s _gate_q_0;" << std::endl;
            out << "  s _gate_q_1;" << std::endl;
            out << "  h _gate_q_0;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            out << "  cx _gate_q_1, _gate_q_0;" << std::endl;
            out << "  h _gate_q_1;" << std::endl;
            out << "}" << std::endl;
            break;
        case QkGate_CSX:
        case QkGate_CS:
            if (!cs) {
                out << "gate cs _gate_q_0, _gate_q_1 {" << std::endl;
                out << "  t _gate_q_0;" << std::endl;
                out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
                out << "  tdg _gate_q_1;" << std::endl;
                out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
                out << "  t _gate_q_1;" << std::endl;
                out << "}" << std::endl;
                cs = true;
            }
            if (op == QkGate_CSX) {
                out << "gate csx _gate_q_0, _gate_q_1 {" << std::endl;
                out << "  h _gate_q_1;" << std::endl;
                out << "  cs _gate_q_0, _gate_q_1;" << std::endl;
                out << "  h _gate_q_1;" << std::endl;
                out << "}" << std::endl;
            }
            break;
        case QkGate_CSdg:
            out << "gate csdg _gate_q_0, _gate_q_1 {" << std::endl;
            out << "  tdg _gate_q_0;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            out << "  t _gate_q_1;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            out << "  tdg _gate_q_1;" << std::endl;
            out << "}" << std::endl;
            break;
        case QkGate_CCZ:
            out << "gate ccz _gate_q_0, _gate_q_1, _gate_q_2 {" << std::endl;
            out << "  h _gate_q_2;" << std::endl;
            out << "  ccx _gate_q_0, _gate_q_1, _gate_q_2;" << std::endl;
            out << "  h _gate_q_2;" << std::endl;
            out << "}" << std::endl;
            break;
        case QkGate_RXX:
            out << "gate rxx(p0) _gate_q_0, _gate_q_1 {" << std::endl;
            out << "  h _gate_q_0;" << std::endl;
            out << "  h _gate_q_1;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            out << "  rz(p0) _gate_q_1;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            out << "  h _gate_q_1;" << std::endl;
            out << "  h _gate_q_0;" << std::endl;
            out << "}" << std::endl;
            break;
        case QkGate_RZX:
            out << "gate rzx(p0) _gate_q_0, _gate_q_1 {" << std::endl;
            out << "  h _gate_q_1;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            out << "  rz(p0) _gate_q_1;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            out << "  h _gate_q_1;" << std::endl;
            out << "}" << std::endl;
            break;
        case QkGate_RZZ:
            out << "gate rzz(p0) _gate_q_0, _gate_q_1 {" << std::endl;
            out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            out << "  rz(p0) _gate_q_1;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            out << "}" << std::endl;
            break;
        case QkGate_RCCX:
            out << "gate rccx _gate_q_0, _gate_q_1, _gate_q_2 {" << std::endl;
            out << "  h _gate_q_2;" << std::endl;
            out << "  t _gate_q_2;" << std::endl;
            out << "  cx _gate_q_1, _gate_q_2;" << std::endl;
            out << "  tdg _gate_q_2;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_2;" << std::endl;
            out << "  t _gate_q_2;" << std::endl;
            out << "  cx _gate_q_1, _gate_q_2;" << std::endl;
            out << "  tdg _gate_q_2;" << std::endl;
            out << "  h _gate_q_2;" << std::endl;
            out << "}" << std::endl;
            break;
        case QkGate_C3X:
            out << "gate mcx _gate_q_0, _gate_q_1, _gate_q_2, _gate_q_3 {" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  p(pi/8) _gate_q_0;" << std::endl;
            out << "  p(pi/8) _gate_q_1;" << std::endl;
            out << "  p(pi/8) _gate_q_2;" << std::endl;
            out << "  p(pi/8) _gate_q_3;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            out << "  p(-pi/8) _gate_q_1;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            out << "  cx _gate_q_1, _gate_q_2;" << std::endl;
            out << "  p(-pi/8) _gate_q_2;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_2;" << std::endl;
            out << "  p(pi/8) _gate_q_2;" << std::endl;
            out << "  cx _gate_q_1, _gate_q_2;" << std::endl;
            out << "  p(-pi/8) _gate_q_2;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_2;" << std::endl;
            out << "  cx _gate_q_2, _gate_q_3;" << std::endl;
            out << "  p(-pi/8) _gate_q_3;" << std::endl;
            out << "  cx _gate_q_1, _gate_q_3;" << std::endl;
            out << "  p(pi/8) _gate_q_3;" << std::endl;
            out << "  cx _gate_q_2, _gate_q_3;" << std::endl;
            out << "  p(-pi/8) _gate_q_3;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_3;" << std::endl;
            out << "  p(pi/8) _gate_q_3;" << std::endl;
            out << "  cx _gate_q_2, _gate_q_3;" << std::endl;
            out << "  p(-pi/8) _gate_q_3;" << std::endl;
            out << "  cx _gate_q_1, _gate_q_3;" << std::endl;
            out << "  p(pi/8) _gate_q_3;" << std::endl;
            out << "  cx _gate_q_2, _gate_q_3;" << std::endl;
            out << "  p(-pi/8) _gate_q_3;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_3;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "}" << std::endl;
            break;
        case QkGate_C3SX:
            out << "gate c3sx _gate_q_0, _gate_q_1, _gate_q_2, _gate_q_3 {" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  cp(pi/8) _gate_q_0, _gate_q_3;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  cp(-pi/8) _gate_q_1, _gate_q_3;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_1;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  cp(pi/8) _gate_q_1, _gate_q_3;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  cx _gate_q_1, _gate_q_2;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  cp(-pi/8) _gate_q_2, _gate_q_3;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_2;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  cp(pi/8) _gate_q_2, _gate_q_3;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  cx _gate_q_1, _gate_q_2;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  cp(-pi/8) _gate_q_2, _gate_q_3;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_2;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  cp(pi/8) _gate_q_2, _gate_q_3;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "}" << std::endl;
            break;
        case QkGate_RC3X:
            out << "gate rcccx _gate_q_0, _gate_q_1, _gate_q_2, _gate_q_3 {" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  t _gate_q_3;" << std::endl;
            out << "  cx _gate_q_2, _gate_q_3;" << std::endl;
            out << "  tdg _gate_q_3;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_3;" << std::endl;
            out << "  t _gate_q_3;" << std::endl;
            out << "  cx _gate_q_1, _gate_q_3;" << std::endl;
            out << "  tdg _gate_q_3;" << std::endl;
            out << "  cx _gate_q_0, _gate_q_3;" << std::endl;
            out << "  t _gate_q_3;" << std::endl;
            out << "  cx _gate_q_1, _gate_q_3;" << std::endl;
            out << "  tdg _gate_q_3;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "  t _gate_q_3;" << std::endl;
            out << "  cx _gate_q_2, _gate_q_3;" << std::endl;
            out << "  tdg _gate_q_3;" << std::endl;
            out << "  h _gate_q_3;" << std::endl;
            out << "}" << std::endl;
            break;
        case QkGate_CU1:
            out << "gate cu1(p0) _gate_q_0, _gate_q_1 {" << std::endl;
            out << "  cp(p0) _gate_q_0 _gate_q_1;" << std::endl;
            out << "}" << std::endl;
            break;
        case QkGate_CU3:
            out << "gate cu3(p0, p1, p2) _gate_q_0, _gate_q_1 {" << std::endl;
            out << "  cu(p0, p1, p2, 0) _gate_q_0 _gate_q_1;" << std::endl;
            out << "}" << std::endl;
            break;
        default:
            break;
        }
    }
    qk_opcounts_clear(&opcounts);
}

void Qasm3Exporter::write_registers(std::stringstream &out) const {
    const std::string qreg_name = "q";
    out << "qubit[" << circ_->num_qubits() << "] " << qreg_name << ";" << std::endl;
    for (const auto& creg : circ_->cregs()) {
        out << "bit[" << creg.size() << "] " << creg.name() << ";" << std::endl;
    }
}

void Qasm3Exporter::write_instructions(std::stringstream &out) {
    const std::string qreg_name = "q";
    const uint_t nops = qk_circuit_num_instructions(rust_circuit_ptr());

    for (uint_t i = 0; i < nops; i++) {
        QkCircuitInstruction *op = new QkCircuitInstruction;
        qk_circuit_get_instruction(rust_circuit_ptr(), i, op);

        if (op->num_clbits > 0 && op->num_qubits == op->num_clbits) {
            for (uint_t j = 0; j < op->num_qubits; j++) {
                const auto creg_data = recover_creg_data(circ_->cregs(), op->clbits[j]);
                out << creg_data.first << "[" << creg_data.second
                    << "] = " << op->name << " "
                    << qreg_name << "[" << op->qubits[j] << "];" << std::endl;
            }
        } else {
            if (std::strcmp(op->name, "u") == 0) {
                out << "U";
            } else {
                out << op->name;
            }
            if (op->num_params > 0) {
                out << "(";
                for (uint_t j = 0; j < op->num_params; j++) {
                    out << param_to_string(op->params[j]);
                    if (j != op->num_params - 1) {
                        out << ", ";
                    }
                }
                out << ")";
            }
            if (op->num_qubits > 0) {
                out << " ";
                for (uint_t j = 0; j < op->num_qubits; j++) {
                    out << qreg_name << "[" << op->qubits[j] << "]";
                    if (j != op->num_qubits - 1) {
                        out << ", ";
                    }
                }
            }
            out << ";" << std::endl;
        }
        qk_circuit_instruction_clear(op);
    }
}

// QuantumCircuit::to_qasm3 implementation.
std::string QuantumCircuit::to_qasm3(void) {
    add_pending_control_flow_op();
    return Qasm3Exporter(this).run();
}

} // namespace circuit
} // namespace Qiskit
