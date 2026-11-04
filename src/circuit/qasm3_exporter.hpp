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

#ifndef __qiskitcpp_circuit_qasm3_exporter_hpp__
#define __qiskitcpp_circuit_qasm3_exporter_hpp__

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "circuit/quantumcircuit_def.hpp"

namespace Qiskit {
namespace circuit {

class Qasm3Exporter {
protected:
  QuantumCircuit &circuit_;

  // RAII guards for Qiskit C-API handles so cleanup is exception-safe and the
  // manual qk_*_free calls cannot be missed on an early return/throw.
  class ScopedOpCounts {
    QkOpCounts counts_;

  public:
    explicit ScopedOpCounts(const QkCircuit *circuit)
        : counts_(qk_circuit_count_ops(circuit)) {}
    ~ScopedOpCounts() { qk_opcounts_clear(&counts_); }
    ScopedOpCounts(const ScopedOpCounts &) = delete;
    ScopedOpCounts &operator=(const ScopedOpCounts &) = delete;
    const QkOpCounts &get(void) const { return counts_; }
  };

  class ScopedInstruction {
    QkCircuitInstruction instruction_;

  public:
    ScopedInstruction(const QkCircuit *circuit, uint_t index) {
      qk_circuit_get_instruction(circuit, index, &instruction_);
    }
    ~ScopedInstruction() { qk_circuit_instruction_clear(&instruction_); }
    ScopedInstruction(const ScopedInstruction &) = delete;
    ScopedInstruction &operator=(const ScopedInstruction &) = delete;
    QkCircuitInstruction &get(void) { return instruction_; }
  };

  class ScopedParam {
    QkParam *param_;

  public:
    explicit ScopedParam(QkParam *param) : param_(param) {}
    ~ScopedParam() { qk_param_free(param_); }
    ScopedParam(const ScopedParam &) = delete;
    ScopedParam &operator=(const ScopedParam &) = delete;
    const QkParam *get(void) const { return param_; }
  };

  class ScopedParamStr {
    char *value_;

  public:
    explicit ScopedParamStr(const QkParam *param) : value_(qk_param_str(param)) {}
    ~ScopedParamStr() { qk_str_free(value_); }
    ScopedParamStr(const ScopedParamStr &) = delete;
    ScopedParamStr &operator=(const ScopedParamStr &) = delete;
    const char *c_str(void) const { return value_; }
  };

  static bool is_identifier_start(const unsigned char ch) {
    return ch == '_' || std::isalpha(ch) || ch >= 0x80;
  }

  static bool is_identifier_char(const unsigned char ch) {
    return is_identifier_start(ch) || std::isdigit(ch);
  }

  static bool is_qasm3_function_identifier(const std::string &token) {
    return token == "sin" || token == "cos" || token == "tan" ||
           token == "asin" || token == "acos" || token == "atan" ||
           token == "log" || token == "exp" || token == "abs" ||
           token == "sign" || token == "conj" || token == "sqrt" ||
           token == "pow";
  }

  static bool is_function_call(const std::string &expression,
                               std::size_t token_end) {
    while (token_end < expression.size() &&
           std::isspace(static_cast<unsigned char>(expression[token_end]))) {
      ++token_end;
    }
    return token_end < expression.size() && expression[token_end] == '(';
  }

  static std::set<std::string> reserved_global_identifiers(void) {
    return std::set<std::string>{
        "OPENQASM", "include", "defcalgrammar", "def", "cal", "defcal",
        "gate", "extern", "box", "let", "break", "continue", "if", "else",
        "end", "return", "for", "while", "in", "switch", "case", "default",
        "input", "output", "const", "readonly", "mutable", "qreg", "qubit",
        "creg", "bool", "bit", "int", "uint", "float", "angle", "complex",
        "array", "void", "duration", "stretch", "gphase", "inv", "pow",
        "ctrl", "negctrl", "durationof", "delay", "reset", "measure",
        "barrier", "sizeof", "true", "false", "pi", "tau", "euler", "im",
        "I", "h", "x", "y", "z", "s", "sdg", "sx", "sxdg", "t", "tdg",
        "rx", "ry", "rz", "p", "u", "U", "cx", "cy", "cz", "ch", "swap",
        "ccx", "cs", "csdg", "csx", "cp", "crx", "cry", "crz", "cswap",
        "cu", "CX", "phase", "cphase", "id", "u1", "u2", "u3"};
  }

  static std::string unique_identifier(const std::string &base,
                                       std::set<std::string> &reserved) {
    std::string candidate = base;
    for (uint_t i = 0; reserved.count(candidate) != 0; i++) {
      candidate = base + "_" + std::to_string(i);
    }
    reserved.insert(candidate);
    return candidate;
  }

  static void reserve_emitted_gate_definition_names(
      QkGate op, std::set<std::string> &reserved) {
    switch (op) {
    case QkGate_R:
      reserved.insert("r");
      break;
    case QkGate_SXdg:
      reserved.insert("sxdg");
      break;
    case QkGate_RYY:
      reserved.insert("sxdg");
      reserved.insert("ryy");
      break;
    case QkGate_XXPlusYY:
      reserved.insert("sxdg");
      reserved.insert("xx_plus_yy");
      break;
    case QkGate_XXMinusYY:
      reserved.insert("sxdg");
      reserved.insert("xx_minus_yy");
      break;
    case QkGate_DCX:
      reserved.insert("dcx");
      break;
    case QkGate_ECR:
      reserved.insert("ecr");
      break;
    case QkGate_ISwap:
      reserved.insert("iswap");
      break;
    case QkGate_CSX:
      reserved.insert("cs");
      reserved.insert("csx");
      break;
    case QkGate_CS:
      reserved.insert("cs");
      break;
    case QkGate_CSdg:
      reserved.insert("csdg");
      break;
    case QkGate_CCZ:
      reserved.insert("ccz");
      break;
    case QkGate_RXX:
      reserved.insert("rxx");
      break;
    case QkGate_RZX:
      reserved.insert("rzx");
      break;
    case QkGate_RZZ:
      reserved.insert("rzz");
      break;
    case QkGate_RCCX:
      reserved.insert("rccx");
      break;
    case QkGate_C3X:
      reserved.insert("mcx");
      break;
    case QkGate_C3SX:
      reserved.insert("c3sx");
      break;
    case QkGate_RC3X:
      reserved.insert("rcccx");
      break;
    case QkGate_CU1:
      reserved.insert("cu1");
      break;
    case QkGate_CU3:
      reserved.insert("cu3");
      break;
    default:
      break;
    }
  }

  static void harvest_parameter_identifiers(const std::string &expression,
                                            std::set<std::string> &inputs) {
    for (std::size_t i = 0; i < expression.size();) {
      const unsigned char ch = static_cast<unsigned char>(expression[i]);
      if (!is_identifier_start(ch)) {
        ++i;
        continue;
      }

      const std::size_t start = i++;
      while (i < expression.size() &&
             is_identifier_char(static_cast<unsigned char>(expression[i]))) {
        ++i;
      }

      if (start > 0 &&
          std::isdigit(static_cast<unsigned char>(expression[start - 1]))) {
        continue;
      }

      const std::string token = expression.substr(start, i - start);
      if (!(is_qasm3_function_identifier(token) &&
            is_function_call(expression, i))) {
        inputs.insert(token);
      }
    }
  }

  static void harvest_parameter(const QkParam *param,
                                std::set<std::string> &inputs) {
    ScopedParamStr param_str(param);
    harvest_parameter_identifiers(param_str.c_str(), inputs);
  }

public:
  /// @brief Collect the parameter symbol names used in a circuit.
  /// @param circuit The circuit to inspect.
  /// @return A sorted list of unique parameter symbol names.
  /// @note Backs ``QuantumCircuit::parameter_symbols``; see that method for the
  ///   stub/limitation notes. The names are harvested from the parameter
  ///   expression strings (gate parameters and global phase) because the C-API
  ///   does not yet expose the symbol list. The harvested count is cross-checked
  ///   against the Rust-side count to avoid returning a misleading list when a
  ///   symbol name cannot be recovered as an OpenQASM identifier token.
  static std::vector<std::string>
  collect_parameter_symbols(const QuantumCircuit &circuit) {
    std::set<std::string> inputs;

    const QkCircuit *rust_circuit = circuit.rust_circuit_.get();
    const uint_t nops = qk_circuit_num_instructions(rust_circuit);
    for (uint_t i = 0; i < nops; i++) {
      ScopedInstruction instruction(rust_circuit, i);
      QkCircuitInstruction &op = instruction.get();
      for (uint_t j = 0; j < op.num_params; j++) {
        harvest_parameter(op.params[j], inputs);
      }
    }

    ScopedParam global_phase(qk_circuit_global_phase(rust_circuit));
    harvest_parameter(global_phase.get(), inputs);

    const std::size_t rust_parameter_count =
        qk_circuit_num_param_symbols(rust_circuit);
    if (inputs.size() != rust_parameter_count) {
      throw std::runtime_error(
          "QuantumCircuit::parameter_symbols cannot derive the parameter "
          "symbol list from the current C-API. The temporary implementation "
          "requires parameter names that can be recovered as OpenQASM "
          "identifier tokens.");
    }

    return std::vector<std::string>(inputs.begin(), inputs.end());
  }

protected:

  std::map<std::string, std::string>
  allocate_input_names(const std::set<std::string> &inputs,
                       std::set<std::string> &reserved) const {
    std::map<std::string, std::string> names;
    for (const auto &input : inputs) {
      names[input] = unique_identifier(input, reserved);
    }
    return names;
  }

  /// @brief Whether a global phase needs to be emitted.
  /// @param phase the circuit's global phase
  /// @return false if the phase is a bound value of zero, true otherwise
  /// @details A zero phase is the identity and is left out, matching Qiskit.
  ///   An unbound (symbolic) phase is always emitted, because its value is not
  ///   known here and could be non-zero once bound.
  static bool needs_global_phase(const QkParam *phase) {
    if (phase == nullptr) {
      return false;
    }
    if (qk_param_kind(phase) == QkParamKind_ParameterExpression) {
      return true;
    }
    const double value = qk_param_as_real(phase);
    // A NaN phase is not a bound zero, so it is emitted and left for the
    // consumer to reject rather than being silently dropped here.
    return value != 0.0;
  }

  /// @brief Format a delay instruction as an OpenQASM 3 delay statement prefix.
  /// @param circuit the circuit holding the instruction
  /// @param index the index of the delay instruction
  /// @param op the instruction, already read from the circuit
  /// @return the statement up to and including the duration, e.g. "delay[100dt]"
  /// @note OpenQASM 3 has no picosecond literal, so a ``ps`` duration is
  ///   emitted in nanoseconds, as Qiskit's own exporter does. A ``dt`` duration
  ///   is emitted as an integer, since it counts backend cycles.
  static std::string format_delay(const QkCircuit *circuit, const uint_t index,
                                  const QkCircuitInstruction &op) {
    const QkDelayUnit unit = qk_circuit_delay_unit(circuit, index);
    const double duration =
        delay_duration_value(op.num_params > 0 ? op.params[0] : nullptr, unit);

    std::stringstream formatted;
    formatted << "delay[";
    if (unit == QkDelayUnit_DT) {
      formatted << static_cast<std::int64_t>(duration) << "dt";
    } else if (unit == QkDelayUnit_PS) {
      formatted << std::setprecision(18) << (duration / 1000.0) << "ns";
    } else {
      formatted << std::setprecision(18) << duration;
      switch (unit) {
      case QkDelayUnit_S:
        formatted << "s";
        break;
      case QkDelayUnit_MS:
        formatted << "ms";
        break;
      case QkDelayUnit_US:
        formatted << "us";
        break;
      case QkDelayUnit_NS:
      default:
        formatted << "ns";
        break;
      }
    }
    formatted << "]";
    return formatted.str();
  }

  std::string format_parameter(
      const QkParam *param,
      const std::map<std::string, std::string> &input_names) const {
    ScopedParamStr param_str(param);
    const std::string expression = param_str.c_str();

    std::stringstream formatted;
    for (std::size_t i = 0; i < expression.size();) {
      const unsigned char ch = static_cast<unsigned char>(expression[i]);
      if (!is_identifier_start(ch)) {
        formatted << expression[i++];
        continue;
      }

      const std::size_t start = i++;
      while (i < expression.size() &&
             is_identifier_char(static_cast<unsigned char>(expression[i]))) {
        ++i;
      }

      const std::string token = expression.substr(start, i - start);
      if (start > 0 &&
          std::isdigit(static_cast<unsigned char>(expression[start - 1]))) {
        formatted << token;
        continue;
      }

      const auto replacement = input_names.find(token);
      if (replacement != input_names.end() &&
          !(is_qasm3_function_identifier(token) &&
            is_function_call(expression, i))) {
        formatted << replacement->second;
      } else {
        formatted << token;
      }
    }

    return formatted.str();
  }

public:
  explicit Qasm3Exporter(QuantumCircuit &circuit) : circuit_(circuit) {}

  /// @brief Serialize a QuantumCircuit object as an OpenQASM3 string.
  /// @return An OpenQASM3 string.
  std::string to_qasm3(void) {
    circuit_.add_pending_control_flow_op();

    std::stringstream qasm3;
    qasm3 << std::setprecision(18);
    qasm3 << "OPENQASM 3.0;" << std::endl;
    qasm3 << "include \"stdgates.inc\";" << std::endl;
    std::set<std::string> reserved = reserved_global_identifiers();
    auto name_map = get_standard_gate_name_mapping();
    ScopedOpCounts opcounts_guard(circuit_.rust_circuit_.get());
    const QkOpCounts &opcounts = opcounts_guard.get();
    for (std::size_t i = 0; i < opcounts.len; i++) {
      if (opcounts.data[i].count != 0) {
        const auto op = name_map[opcounts.data[i].name].gate_map();
        reserve_emitted_gate_definition_names(op, reserved);
      }
    }

    const auto symbols = circuit_.parameter_symbols();
    const std::set<std::string> inputs(symbols.begin(), symbols.end());
    const auto input_names = allocate_input_names(inputs, reserved);
    for (const auto &input : input_names) {
      qasm3 << "input float[64] " << input.second << ";" << std::endl;
    }

    // add header for non-standard gates
    bool cs = false;
    bool sxdg = false;
    for (std::size_t i = 0; i < opcounts.len; i++) {
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
            qasm3 << "gate xx_plus_yy(p0, p1) _gate_q_0, _gate_q_1 {"
                  << std::endl;
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
            qasm3 << "gate xx_minus_yy(p0, p1) _gate_q_0, _gate_q_1 {"
                  << std::endl;
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
          qasm3 << "gate mcx _gate_q_0, _gate_q_1, _gate_q_2, _gate_q_3 {"
                << std::endl;
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
          qasm3 << "gate c3sx _gate_q_0, _gate_q_1, _gate_q_2, _gate_q_3 {"
                << std::endl;
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
          qasm3 << "gate rcccx _gate_q_0, _gate_q_1, _gate_q_2, _gate_q_3 {"
                << std::endl;
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

    // save ops
    uint_t nops;
    nops = qk_circuit_num_instructions(circuit_.rust_circuit_.get());

    // Declare registers
    // After transpilation, qubit registers will be mapped to physical
    // registers, so transpiled circuits emit physical qubit references ($n)
    // instead of declaring a single quantum register "q".
    const bool physical_qubits = !circuit_.qubit_map_.empty();
    const std::string qreg_name =
        physical_qubits ? std::string("q") : unique_identifier("q", reserved);
    auto qubit_ref = [physical_qubits, &qreg_name](uint_t index) -> std::string {
      if (physical_qubits) {
        return std::string("$") + std::to_string(index);
      }
      return qreg_name + "[" + std::to_string(index) + "]";
    };
    if (!physical_qubits) {
      qasm3 << "qubit[" << circuit_.num_qubits() << "] " << qreg_name << ";"
            << std::endl;
    }
    std::vector<std::string> creg_names;
    for (const auto &creg : circuit_.cregs_) {
      creg_names.push_back(unique_identifier(creg.name(), reserved));
      if (creg.size() == 0) {
        continue;
      }
      qasm3 << "bit[" << creg.size() << "] " << creg_names.back() << ";"
            << std::endl;
    }

    // Emit the circuit's global phase. It is a property of the circuit rather
    // than an instruction, so it is not reached by the instruction loop below
    // and has to be written out here, after the declarations it may reference
    // and before the operations it applies to.
    ScopedParam global_phase(qk_circuit_global_phase(circuit_.rust_circuit_.get()));
    if (needs_global_phase(global_phase.get())) {
      qasm3 << "gphase(" << format_parameter(global_phase.get(), input_names)
            << ");" << std::endl;
    }

    auto recover_reg_data =
        [this, &creg_names](uint_t index) -> std::pair<std::string, uint_t> {
      auto it =
          std::upper_bound(circuit_.cregs_.begin(), circuit_.cregs_.end(),
                           index, [](uint_t v, const ClassicalRegister &reg) {
                             return v < reg.base_index();
                           });
      assert(it != circuit_.cregs_.begin());
      it = std::prev(it);
      const auto reg_index =
          static_cast<std::size_t>(std::distance(circuit_.cregs_.begin(), it));
      return std::make_pair(creg_names[reg_index], index - it->base_index());
    };

    for (uint_t i = 0; i < nops; i++) {
      ScopedInstruction instruction(circuit_.rust_circuit_.get(), i);
      QkCircuitInstruction &op = instruction.get();
      if (qk_circuit_instruction_kind(circuit_.rust_circuit_.get(), i) ==
          QkOperationKind_Delay) {
        // A delay is a statement with its own syntax, "delay[<duration>] <qubits>;",
        // rather than a gate call, so it cannot go through the generic path below.
        qasm3 << format_delay(circuit_.rust_circuit_.get(), i, op) << " ";
        for (uint_t j = 0; j < op.num_qubits; j++) {
          qasm3 << qubit_ref(op.qubits[j]);
          if (j != op.num_qubits - 1)
            qasm3 << ", ";
        }
        qasm3 << ";" << std::endl;
        continue;
      }
      if (op.num_clbits > 0) {
        if (op.num_qubits == op.num_clbits) {
          for (uint_t j = 0; j < op.num_qubits; j++) {
            const auto creg_data = recover_reg_data(op.clbits[j]);
            qasm3 << creg_data.first << "[" << creg_data.second
                  << "] = " << op.name << " " << qubit_ref(op.qubits[j]) << ";"
                  << std::endl;
          }
        }
      } else {
        if (strcmp(op.name, "u") == 0) {
          qasm3 << "U";
        } else if (strcmp(op.name, "global_phase") == 0) {
          qasm3 << "gphase";
        } else {
          qasm3 << op.name;
        }
        if (op.num_params > 0) {
          qasm3 << "(";
          for (uint_t j = 0; j < op.num_params; j++) {
            qasm3 << format_parameter(op.params[j], input_names);
            if (j != op.num_params - 1)
              qasm3 << ", ";
          }
          qasm3 << ")";
        }
        if (op.num_qubits > 0) {
          qasm3 << " ";
          for (uint_t j = 0; j < op.num_qubits; j++) {
            qasm3 << qubit_ref(op.qubits[j]);
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

inline std::vector<std::string> QuantumCircuit::parameter_symbols(void) const {
  return Qasm3Exporter::collect_parameter_symbols(*this);
}

inline std::string QuantumCircuit::to_qasm3(void) {
  return Qasm3Exporter(*this).to_qasm3();
}

} // namespace circuit
} // namespace Qiskit

#endif // __qiskitcpp_circuit_qasm3_exporter_hpp__
