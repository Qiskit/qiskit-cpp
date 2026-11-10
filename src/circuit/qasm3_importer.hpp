/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2026.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.
*/

// OpenQASM 3 importer

#ifndef __qiskitcpp_circuit_qasm3_importer_hpp__
#define __qiskitcpp_circuit_qasm3_importer_hpp__

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "circuit/delay.hpp"
#include "circuit/library/standard_gates/standard_gates.hpp"
#include "circuit/quantumcircuit_def.hpp"

namespace Qiskit {
namespace circuit {

/// @class Qasm3Importer
/// @brief Builds a QuantumCircuit from an OpenQASM 3 program.
/// @details Reads the subset of OpenQASM 3 that ``Qasm3Exporter`` writes, so
///   that a circuit exported with ``QuantumCircuit::to_qasm3`` can be read back.
///   Only the gates qiskit-cpp knows are accepted, which are the ones in
///   ``get_standard_gate_name_mapping`` plus ``measure``, ``reset``,
///   ``barrier`` and ``delay``. Anything else is reported with the line it
///   appears on rather than being skipped, so an unsupported program fails
///   loudly instead of importing as a different circuit.
/// @note There is no QASM3 entry point in the Qiskit C-API (it exposes QPY but
///   not OpenQASM), so this is a parser written here rather than a wrapper over
///   a C-API call. It is deliberately matched to the exporter in the same
///   directory, and the round-trip tests in ``test/test_circuit.cpp`` are what
///   keep the two in agreement.
class Qasm3Importer {
protected:
  /// @brief One token of the program text.
  struct Token {
    enum class Kind {
      End,        ///< End of input.
      Identifier, ///< An identifier or keyword.
      Number,     ///< A numeric literal.
      String,     ///< A quoted string, as in an include.
      Physical,   ///< A physical qubit reference, "$0".
      Symbol,     ///< Punctuation, including the two-character "**".
    };

    Kind kind = Kind::End;
    std::string text;
    double value = 0.0; ///< Set for Kind::Number.
    uint_t index = 0;   ///< Set for Kind::Physical.
    uint_t line = 0;
  };

  std::vector<Token> tokens_;
  std::size_t pos_ = 0;

  // The declarations gathered before the circuit is built. The circuit's
  // constructors take their register sizes up front and there is no way to add
  // a register afterwards, so the declarations have to be known before any
  // instruction can be replayed.
  struct Declarations {
    uint_t num_qubits = 0;
    std::string qreg_name;
    bool has_qubit_declaration = false;
    bool uses_physical_qubits = false;
    // The classical registers, in the order declared, each with its name and
    // width. Their base indices follow from the order, as in the exporter.
    std::vector<std::pair<std::string, uint_t>> cregs;
    std::set<std::string> inputs;
  };

  Declarations decls_;
  // The parameter symbols declared with "input", so that an identifier in an
  // expression can be told apart from a typo.
  std::unordered_map<std::string, Parameter> symbols_;
  // Maps a classical register name to its index in decls_.cregs.
  std::unordered_map<std::string, std::size_t> creg_index_;
  // The base index of each classical register, parallel to decls_.cregs.
  std::vector<uint_t> creg_base_;

  /// @brief Report a problem with the program, naming the line it is on.
  /// @param message what is wrong
  /// @param line the line number to name, 1-based
  /// @throws std::invalid_argument always
  /// @note Marked ``noreturn`` so that a value-returning caller does not need
  ///   an unreachable return after it, and so a compiler does not warn about
  ///   one missing.
  [[noreturn]] static void fail(const std::string &message, const uint_t line) {
    std::stringstream formatted;
    formatted << "QASM3 import failed at line " << line << " : " << message;
    throw std::invalid_argument(formatted.str());
  }

  /// @brief Report a problem at the current token.
  [[noreturn]] void fail_here(const std::string &message) const {
    fail(message, peek().line);
  }

  // --- Tokenizer ------------------------------------------------------------

  static bool is_identifier_start(const unsigned char ch) {
    // Matches the exporter's rule, which accepts non-ASCII bytes so that a
    // parameter named in a non-Latin script survives a round trip.
    return ch == '_' || std::isalpha(ch) || ch >= 0x80;
  }

  static bool is_identifier_char(const unsigned char ch) {
    return is_identifier_start(ch) || std::isdigit(ch);
  }

  /// @brief Split the program text into tokens.
  /// @param source the program text
  /// @throws std::invalid_argument if the text holds something untokenizable
  void tokenize(const std::string &source) {
    tokens_.clear();
    uint_t line = 1;

    for (std::size_t i = 0; i < source.size();) {
      const char ch = source[i];

      if (ch == '\n') {
        line++;
        i++;
        continue;
      }
      if (std::isspace(static_cast<unsigned char>(ch))) {
        i++;
        continue;
      }

      // Comments. A block comment may span lines, which must still be counted.
      if (ch == '/' && i + 1 < source.size() && source[i + 1] == '/') {
        while (i < source.size() && source[i] != '\n') {
          i++;
        }
        continue;
      }
      if (ch == '/' && i + 1 < source.size() && source[i + 1] == '*') {
        const uint_t start_line = line;
        i += 2;
        bool closed = false;
        while (i < source.size()) {
          if (source[i] == '\n') {
            line++;
          }
          if (source[i] == '*' && i + 1 < source.size() &&
              source[i + 1] == '/') {
            i += 2;
            closed = true;
            break;
          }
          i++;
        }
        if (!closed) {
          fail("a block comment is never closed.", start_line);
        }
        continue;
      }

      Token token;
      token.line = line;

      if (is_identifier_start(static_cast<unsigned char>(ch))) {
        const std::size_t start = i;
        while (i < source.size() &&
               is_identifier_char(static_cast<unsigned char>(source[i]))) {
          i++;
        }
        token.kind = Token::Kind::Identifier;
        token.text = source.substr(start, i - start);
        tokens_.push_back(token);
        continue;
      }

      if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '.') {
        const std::size_t start = i;
        while (i < source.size() &&
               (std::isdigit(static_cast<unsigned char>(source[i])) ||
                source[i] == '.')) {
          i++;
        }
        // An exponent, as in "1e-9". The sign belongs to the exponent rather
        // than being a separate operator token.
        if (i < source.size() && (source[i] == 'e' || source[i] == 'E')) {
          const std::size_t exponent = i;
          i++;
          if (i < source.size() && (source[i] == '+' || source[i] == '-')) {
            i++;
          }
          if (i < source.size() &&
              std::isdigit(static_cast<unsigned char>(source[i]))) {
            while (i < source.size() &&
                   std::isdigit(static_cast<unsigned char>(source[i]))) {
              i++;
            }
          } else {
            // Not an exponent after all: "1east" is a number then a word.
            i = exponent;
          }
        }
        const std::string text = source.substr(start, i - start);
        // Parsed with strtod rather than a stringstream so that the
        // end-of-conversion pointer can be checked: "1.2.3" tokenizes as one
        // run of digits and dots and has to be rejected rather than read as
        // 1.2.
        const char *begin = text.c_str();
        char *end = nullptr;
        const double value = std::strtod(begin, &end);
        if (end != begin + text.size()) {
          fail("\"" + text + "\" is not a number.", line);
        }
        token.kind = Token::Kind::Number;
        token.text = text;
        token.value = value;
        tokens_.push_back(token);
        continue;
      }

      if (ch == '"' || ch == '\'') {
        const char quote = ch;
        const std::size_t start = ++i;
        while (i < source.size() && source[i] != quote) {
          if (source[i] == '\n') {
            fail("a string is never closed.", line);
          }
          i++;
        }
        if (i >= source.size()) {
          fail("a string is never closed.", line);
        }
        token.kind = Token::Kind::String;
        token.text = source.substr(start, i - start);
        i++; // The closing quote.
        tokens_.push_back(token);
        continue;
      }

      if (ch == '$') {
        const std::size_t start = ++i;
        while (i < source.size() &&
               std::isdigit(static_cast<unsigned char>(source[i]))) {
          i++;
        }
        if (i == start) {
          fail("\"$\" must be followed by a qubit number.", line);
        }
        token.kind = Token::Kind::Physical;
        token.text = source.substr(start - 1, i - start + 1);
        token.index = (uint_t)std::strtoull(source.substr(start, i - start).c_str(),
                                            nullptr, 10);
        tokens_.push_back(token);
        continue;
      }

      // Punctuation. "**" is the only two-character operator the exporter
      // writes, so it is matched before the single-character fallback.
      token.kind = Token::Kind::Symbol;
      if (ch == '*' && i + 1 < source.size() && source[i + 1] == '*') {
        token.text = "**";
        i += 2;
      } else {
        token.text = std::string(1, ch);
        i++;
      }
      tokens_.push_back(token);
    }

    Token end;
    end.kind = Token::Kind::End;
    end.line = line;
    tokens_.push_back(end);
  }

  // --- Token cursor ---------------------------------------------------------

  const Token &peek(const std::size_t ahead = 0) const {
    const std::size_t at = pos_ + ahead;
    // The end token is always last, so clamping gives a well-defined result
    // past the end rather than reading out of bounds.
    return tokens_[at < tokens_.size() ? at : tokens_.size() - 1];
  }

  const Token &advance(void) {
    const Token &token = peek();
    if (pos_ < tokens_.size() - 1) {
      pos_++;
    }
    return token;
  }

  bool at_symbol(const std::string &text, const std::size_t ahead = 0) const {
    const Token &token = peek(ahead);
    return token.kind == Token::Kind::Symbol && token.text == text;
  }

  bool at_identifier(const std::string &text,
                     const std::size_t ahead = 0) const {
    const Token &token = peek(ahead);
    return token.kind == Token::Kind::Identifier && token.text == text;
  }

  bool accept_symbol(const std::string &text) {
    if (at_symbol(text)) {
      advance();
      return true;
    }
    return false;
  }

  void expect_symbol(const std::string &text) {
    if (!accept_symbol(text)) {
      fail_here("expected \"" + text + "\" but found \"" + peek().text + "\".");
    }
  }

  void expect_identifier(const std::string &text) {
    if (!at_identifier(text)) {
      fail_here("expected \"" + text + "\" but found \"" + peek().text + "\".");
    }
    advance();
  }

  std::string expect_any_identifier(const std::string &what) {
    if (peek().kind != Token::Kind::Identifier) {
      fail_here("expected " + what + " but found \"" + peek().text + "\".");
    }
    return advance().text;
  }

  /// @brief Read a non-negative integer literal, as in a register width.
  uint_t expect_integer(const std::string &what) {
    const Token &token = peek();
    if (token.kind != Token::Kind::Number) {
      fail_here("expected " + what + " but found \"" + token.text + "\".");
    }
    if (token.value < 0.0 || token.value != std::floor(token.value)) {
      fail_here(what + " must be a non-negative whole number, but is \"" +
                token.text + "\".");
    }
    advance();
    return (uint_t)token.value;
  }

  // --- Expressions ---------------------------------------------------------

  /// @brief Return how many parameters a gate takes.
  /// @param op the instruction, as obtained from the standard gate mapping
  /// @return the number of parameters
  /// @details Asks the C-API rather than the ``Instruction``.
  ///   ``get_standard_gate_name_mapping`` stores its values **by value** as
  ///   ``Instruction``, so ``name_mapping["rx"] = RXGate()`` slices the
  ///   ``RXGate`` down to its base and loses the ``num_params()`` override. The
  ///   base implementation returns ``params_.size()``, and the gate classes
  ///   never populate ``params_``, so every parameterized gate read out of that
  ///   map reports zero parameters. ``qk_gate_num_params`` is the authoritative
  ///   count and is unaffected.
  /// @note ``num_qubits()`` does not have this problem: it is not virtual and
  ///   reads ``num_qubits_``, a plain member each gate's constructor sets, which
  ///   survives the slicing. That is why only the parameter count was wrong.
  static uint_t gate_num_params(const Instruction &op) {
    if (!op.is_standard_gate()) {
      // measure, reset, barrier and delay are handled before this point, and
      // none of them carries a gate parameter.
      return op.num_params();
    }
    return (uint_t)qk_gate_num_params(op.gate_map());
  }

  /// @brief Return how many qubits a gate acts on.
  /// @param op the instruction, as obtained from the standard gate mapping
  /// @return the number of qubits
  /// @details Taken from the C-API for the same reason as the parameter count,
  ///   and because ``num_qubits_`` does not always agree with it: ``MCXGate``
  ///   sets 3 while mapping to ``QkGate_RC3X``, which Qiskit defines as a
  ///   4-qubit gate. Validating against the C-API means a gate call is accepted
  ///   exactly when ``qk_circuit_gate`` will read the right number of qubits
  ///   from the array handed to it, which is what actually matters: a
  ///   mismatched count would have it read past the end.
  static uint_t gate_num_qubits(const Instruction &op) {
    if (!op.is_standard_gate()) {
      return op.num_qubits();
    }
    return (uint_t)qk_gate_num_qubits(op.gate_map());
  }

  /// @brief Return pi to the precision of a double.
  /// @details Not M_PI, which the C++ standard does not define; see the use
  ///   site. The digits are more than a double can hold, so the value rounds
  ///   to the nearest representable one.
  static double pi_value(void) {
    return 3.14159265358979323846264338327950288;
  }

  static bool is_supported_function(const std::string &name) {
    // The functions the exporter can write, which are the ones Qiskit's
    // parameter expressions support. "sqrt" has no qk_param_sqrt and is
    // lowered to a power below. "conj" is the exporter's spelling of
    // qk_param_conjugate.
    return name == "sin" || name == "cos" || name == "tan" || name == "asin" ||
           name == "acos" || name == "atan" || name == "log" ||
           name == "exp" || name == "abs" || name == "sign" ||
           name == "conj" || name == "sqrt" || name == "pow";
  }

  /// @brief Apply a one-argument function to a parameter.
  Parameter apply_function(const std::string &name, const Parameter &argument,
                           const uint_t line) {
    if (name == "sin") {
      return argument.sin();
    }
    if (name == "cos") {
      return argument.cos();
    }
    if (name == "tan") {
      return argument.tan();
    }
    if (name == "asin") {
      return argument.asin();
    }
    if (name == "acos") {
      return argument.acos();
    }
    if (name == "atan") {
      return argument.atan();
    }
    if (name == "log") {
      return argument.log();
    }
    if (name == "exp") {
      return argument.exp();
    }
    if (name == "abs") {
      return argument.abs();
    }
    if (name == "sign") {
      return argument.sign();
    }
    if (name == "conj") {
      return argument.conjugate();
    }
    if (name == "sqrt") {
      return argument.sqrt();
    }
    fail("\"" + name + "\" takes two arguments.", line);
    return Parameter();
  }

  /// @brief Parse a primary expression: a literal, a symbol, a call, or a
  ///   parenthesized expression.
  Parameter parse_primary(void) {
    const Token &token = peek();

    if (token.kind == Token::Kind::Number) {
      advance();
      return Parameter(token.value);
    }

    if (accept_symbol("(")) {
      Parameter inner = parse_expression();
      expect_symbol(")");
      return inner;
    }

    if (accept_symbol("-")) {
      // Unary minus binds tighter than multiplication here, which matches the
      // exporter's "(-0.5)*p0" and "-pi/8" output either way.
      Parameter operand = parse_primary();
      return -operand;
    }
    if (accept_symbol("+")) {
      return parse_primary();
    }

    if (token.kind == Token::Kind::Identifier) {
      const std::string name = token.text;
      const uint_t line = token.line;
      advance();

      if (at_symbol("(")) {
        if (!is_supported_function(name)) {
          fail("\"" + name + "\" is not a function this importer supports.",
               line);
        }
        advance(); // "("
        Parameter first = parse_expression();
        if (accept_symbol(",")) {
          Parameter second = parse_expression();
          expect_symbol(")");
          if (name != "pow") {
            fail("\"" + name + "\" takes one argument, but two were given.",
                 line);
          }
          return first.pow(second);
        }
        expect_symbol(")");
        return apply_function(name, first, line);
      }

      // The constants OpenQASM 3 defines, which the exporter emits as "pi".
      // Spelled out rather than taken from M_PI and M_E, which are not in the
      // C++ standard: they come from POSIX, and MSVC only defines them when
      // _USE_MATH_DEFINES is set before <cmath>.
      if (name == "pi" || name == "\xcf\x80") {
        return Parameter(pi_value());
      }
      if (name == "tau" || name == "\xcf\x84") {
        return Parameter(2.0 * pi_value());
      }
      if (name == "euler" || name == "\xe2\x84\xaf") {
        return Parameter(2.71828182845904523536028747135266250);
      }

      auto found = symbols_.find(name);
      if (found == symbols_.end()) {
        fail("\"" + name +
                 "\" is not a declared parameter. Declare it with \"input "
                 "float[64] " + name + ";\".",
             line);
      }
      return found->second;
    }

    fail_here("expected a value but found \"" + token.text + "\".");
    return Parameter();
  }

  /// @brief Parse a power expression, which is right-associative.
  Parameter parse_power(void) {
    Parameter base = parse_primary();
    if (accept_symbol("**")) {
      Parameter exponent = parse_power();
      return base.pow(exponent);
    }
    return base;
  }

  /// @brief Parse a product, left-associative over "*" and "/".
  Parameter parse_product(void) {
    Parameter result = parse_power();
    for (;;) {
      if (accept_symbol("*")) {
        Parameter rhs = parse_power();
        result = result * rhs;
      } else if (accept_symbol("/")) {
        Parameter rhs = parse_power();
        result = result / rhs;
      } else {
        return result;
      }
    }
  }

  /// @brief Parse a sum, left-associative over "+" and "-".
  Parameter parse_expression(void) {
    Parameter result = parse_product();
    for (;;) {
      if (accept_symbol("+")) {
        Parameter rhs = parse_product();
        result = result + rhs;
      } else if (accept_symbol("-")) {
        Parameter rhs = parse_product();
        result = result - rhs;
      } else {
        return result;
      }
    }
  }

  // --- Declarations ---------------------------------------------------------

  /// @brief Skip a bracketed width, as in the "[64]" of "float[64]".
  void skip_optional_width(void) {
    if (accept_symbol("[")) {
      expect_integer("a width");
      expect_symbol("]");
    }
  }

  /// @brief Skip the body of a gate definition by matching braces.
  /// @details The gate is already known to qiskit-cpp, so its definition adds
  ///   nothing: the exporter writes these blocks only because a consumer that
  ///   is not qiskit-cpp needs them. Matching braces rather than parsing the
  ///   body keeps the importer from having to understand the gate-body
  ///   sublanguage.
  void skip_braced_body(void) {
    const uint_t start_line = peek().line;
    expect_symbol("{");
    uint_t depth = 1;
    while (depth > 0) {
      if (peek().kind == Token::Kind::End) {
        fail("a gate definition opened here is never closed.", start_line);
      }
      if (at_symbol("{")) {
        depth++;
      } else if (at_symbol("}")) {
        depth--;
      }
      advance();
    }
  }

  /// @brief Return the gate definitions the exporter writes.
  /// @details Only these are accepted: each names a gate that is in the
  ///   standard mapping anyway, so skipping the body loses nothing. A
  ///   definition of some other gate would have to be inlined to be honoured,
  ///   which this importer does not do, so it is rejected rather than silently
  ///   dropped.
  static const std::set<std::string> &exporter_gate_definitions(void) {
    static const std::set<std::string> names = {
        "r",    "sxdg",  "ryy",   "xx_plus_yy", "xx_minus_yy", "dcx",
        "ecr",  "iswap", "cs",    "csdg",       "csx",         "ccz",
        "rxx",  "rzx",   "rzz",   "rccx",       "mcx",         "c3sx",
        "rcccx", "cu1",  "cu3"};
    return names;
  }

  /// @brief Walk the whole program once, collecting what the circuit needs to
  ///   be constructed.
  /// @details Declarations have to be known before the circuit exists, because
  ///   ``QuantumCircuit``'s constructors take the register sizes and there is
  ///   no way to add a register to a built circuit. The qubit count also has to
  ///   account for physical references, which carry no declaration at all: a
  ///   transpiled circuit is exported as bare "$n" with no "qubit[n]" line, so
  ///   the width is the highest index seen plus one.
  void collect_declarations(void) {
    const std::size_t saved = pos_;
    uint_t highest_physical = 0;
    bool saw_physical = false;

    for (;;) {
      const Token &token = peek();
      if (token.kind == Token::Kind::End) {
        break;
      }

      if (token.kind == Token::Kind::Physical) {
        saw_physical = true;
        if (token.index > highest_physical) {
          highest_physical = token.index;
        }
        advance();
        continue;
      }

      if (token.kind != Token::Kind::Identifier) {
        advance();
        continue;
      }

      if (token.text == "qubit") {
        const uint_t line = token.line;
        advance();
        uint_t width = 1; // "qubit q;" declares a single qubit.
        if (accept_symbol("[")) {
          width = expect_integer("a qubit count");
          expect_symbol("]");
        }
        const std::string name = expect_any_identifier("a register name");
        if (decls_.has_qubit_declaration) {
          fail("a second quantum register \"" + name +
                   "\" was declared. The exporter merges every quantum "
                   "register into one, so only a single declaration can be "
                   "imported.",
               line);
        }
        decls_.has_qubit_declaration = true;
        decls_.num_qubits = width;
        decls_.qreg_name = name;
        continue;
      }

      if (token.text == "bit" || token.text == "creg") {
        advance();
        uint_t width = 1;
        if (accept_symbol("[")) {
          width = expect_integer("a bit count");
          expect_symbol("]");
        }
        const std::string name = expect_any_identifier("a register name");
        // A repeated name would make a reference ambiguous.
        for (std::size_t i = 0; i < decls_.cregs.size(); i++) {
          if (decls_.cregs[i].first == name) {
            fail("the classical register \"" + name +
                     "\" is declared more than once.",
                 token.line);
          }
        }
        decls_.cregs.push_back(std::make_pair(name, width));
        continue;
      }

      if (token.text == "input" || token.text == "output") {
        advance();
        // The type, as in "float[64]" or "angle[32]".
        expect_any_identifier("a parameter type");
        skip_optional_width();
        const std::string name = expect_any_identifier("a parameter name");
        decls_.inputs.insert(name);
        continue;
      }

      advance();
    }

    if (saw_physical) {
      decls_.uses_physical_qubits = true;
      // A transpiled circuit is exported with no "qubit[n]" line at all, so
      // the width can only be inferred from the references. This under-counts
      // when the circuit's highest qubits carry no operation: a 127-qubit
      // backend circuit touching only $0 and $1 imports as 2 qubits. The
      // operations and their relative order are unaffected.
      const uint_t needed = highest_physical + 1;
      if (!decls_.has_qubit_declaration) {
        decls_.num_qubits = needed;
      } else if (needed > decls_.num_qubits) {
        // A program mixing "q[n]" and "$n" would need both; widening keeps
        // every reference in range rather than failing late on one statement.
        decls_.num_qubits = needed;
      }
    }

    pos_ = saved;
  }

  // --- References ----------------------------------------------------------

  /// @brief Parse one qubit reference, either "q[n]", "$n", or a bare register.
  /// @return the qubit index within the circuit
  uint_t parse_qubit(void) {
    const Token &token = peek();

    if (token.kind == Token::Kind::Physical) {
      advance();
      if (token.index >= decls_.num_qubits) {
        fail("physical qubit " + std::to_string(token.index) +
                 " is outside the circuit's " +
                 std::to_string(decls_.num_qubits) + " qubits.",
             token.line);
      }
      return token.index;
    }

    const uint_t line = token.line;
    const std::string name = expect_any_identifier("a qubit reference");
    if (!decls_.has_qubit_declaration || name != decls_.qreg_name) {
      fail("\"" + name + "\" is not a declared quantum register.", line);
    }
    if (!accept_symbol("[")) {
      // "h q;" broadcasts over the register in OpenQASM 3. Only a single-qubit
      // register is unambiguous as one qubit, so anything wider is rejected
      // rather than guessed at.
      if (decls_.num_qubits == 1) {
        return 0;
      }
      fail("\"" + name +
               "\" is used without an index. Broadcasting a gate over a whole "
               "register is not supported; index the qubit explicitly.",
           line);
    }
    const uint_t index = expect_integer("a qubit index");
    expect_symbol("]");
    if (index >= decls_.num_qubits) {
      fail("qubit index " + std::to_string(index) + " is outside \"" + name +
               "\", which has " + std::to_string(decls_.num_qubits) +
               " qubits.",
           line);
    }
    return index;
  }

  /// @brief Parse one clbit reference, "c[n]" or a bare single-bit register.
  /// @return the clbit index within the circuit
  uint_t parse_clbit(void) {
    const uint_t line = peek().line;
    const std::string name = expect_any_identifier("a classical bit reference");
    auto found = creg_index_.find(name);
    if (found == creg_index_.end()) {
      fail("\"" + name + "\" is not a declared classical register.", line);
    }
    const std::size_t which = found->second;
    const uint_t width = decls_.cregs[which].second;

    if (!accept_symbol("[")) {
      if (width == 1) {
        return creg_base_[which];
      }
      fail("\"" + name +
               "\" is used without an index. Index the bit explicitly.",
           line);
    }
    const uint_t index = expect_integer("a bit index");
    expect_symbol("]");
    if (index >= width) {
      fail("bit index " + std::to_string(index) + " is outside \"" + name +
               "\", which has " + std::to_string(width) + " bits.",
           line);
    }
    return creg_base_[which] + index;
  }

  /// @brief Parse a comma-separated list of qubit references up to the ";".
  reg_t parse_qubit_list(void) {
    reg_t qubits;
    if (at_symbol(";")) {
      return qubits;
    }
    for (;;) {
      qubits.push_back(parse_qubit());
      if (!accept_symbol(",")) {
        return qubits;
      }
    }
  }

  // --- Statements ----------------------------------------------------------

  /// @brief Parse the "delay[<duration><unit>] <qubits>;" statement.
  void parse_delay(QuantumCircuit &circuit) {
    const uint_t line = peek().line;
    expect_identifier("delay");
    expect_symbol("[");

    const Token &duration_token = peek();
    if (duration_token.kind != Token::Kind::Number) {
      fail_here("expected a delay duration but found \"" +
                duration_token.text + "\".");
    }
    advance();
    const double duration = duration_token.value;

    // The unit is written up against the number, so it tokenizes separately.
    DelayUnit unit = DelayUnit::dt;
    if (peek().kind == Token::Kind::Identifier) {
      const std::string name = peek().text;
      if (!delay_unit_from_name(name, unit)) {
        fail("\"" + name +
                 "\" is not a delay unit. Use one of dt, s, ms, us, ns or ps.",
             peek().line);
      }
      advance();
    } else {
      fail_here("a delay duration must be followed by a unit, one of dt, s, "
                "ms, us, ns or ps.");
    }
    expect_symbol("]");

    const reg_t qubits = parse_qubit_list();
    expect_symbol(";");
    if (qubits.empty()) {
      fail("a delay must name at least one qubit.", line);
    }
    if (!circuit.delay(duration, qubits, unit)) {
      // delay() reports the reason itself and returns false rather than
      // throwing, so the line number is added here.
      fail("the delay could not be added to the circuit.", line);
    }
  }

  /// @brief Parse "<creg>[<n>] = measure <qubit>;".
  void parse_measure_assignment(QuantumCircuit &circuit) {
    const uint_t clbit = parse_clbit();
    expect_symbol("=");
    expect_identifier("measure");
    const uint_t qubit = parse_qubit();
    expect_symbol(";");
    circuit.measure(qubit, clbit);
  }

  /// @brief Parse a gate call, a bare measure, a reset, or a barrier.
  void parse_operation(QuantumCircuit &circuit, bool &seen_operation) {
    const Token &token = peek();
    const uint_t line = token.line;
    const std::string name = token.text;

    if (name == "measure") {
      // "measure q[0];" with no target, or "measure q[0] -> c[0];".
      advance();
      const uint_t qubit = parse_qubit();
      if (accept_symbol("-")) {
        expect_symbol(">");
        const uint_t clbit = parse_clbit();
        expect_symbol(";");
        seen_operation = true;
        circuit.measure(qubit, clbit);
        return;
      }
      expect_symbol(";");
      fail("a measurement must store its result, as in \"c[0] = measure "
           "q[0];\". qiskit-cpp has no measurement without a target bit.",
           line);
    }

    if (name == "reset") {
      advance();
      const reg_t qubits = parse_qubit_list();
      expect_symbol(";");
      if (qubits.empty()) {
        fail("a reset must name at least one qubit.", line);
      }
      seen_operation = true;
      for (uint_t i = 0; i < qubits.size(); i++) {
        circuit.reset(qubits[i]);
      }
      return;
    }

    if (name == "barrier") {
      advance();
      reg_t qubits = parse_qubit_list();
      expect_symbol(";");
      if (qubits.empty()) {
        // A bare "barrier;" covers the whole circuit in OpenQASM 3.
        qubits.resize(decls_.num_qubits);
        for (uint_t i = 0; i < decls_.num_qubits; i++) {
          qubits[i] = i;
        }
      }
      seen_operation = true;
      circuit.barrier(qubits);
      return;
    }

    if (name == "delay") {
      parse_delay(circuit);
      seen_operation = true;
      return;
    }

    // Control flow and the other statement keywords are reported for what they
    // are rather than as an unknown gate, which is what the lookup below would
    // otherwise call them.
    if (name == "if" || name == "else" || name == "for" || name == "while" ||
        name == "switch" || name == "case" || name == "box" ||
        name == "def" || name == "defcal" || name == "cal" ||
        name == "return" || name == "break" || name == "continue" ||
        name == "end") {
      fail("\"" + name +
               "\" is not supported. qiskit-cpp cannot build control flow "
               "through the Qiskit C-API, which exposes only accessors for it "
               "and no constructors, so a control-flow statement cannot be "
               "imported.",
           line);
    }
    if (name == "ctrl" || name == "negctrl" || name == "inv" ||
        name == "pow") {
      fail("the gate modifier \"" + name +
               "\" is not supported; only a plain gate call can be imported.",
           line);
    }
    if (name == "let" || name == "const" || name == "int" ||
        name == "uint" || name == "float" || name == "angle" ||
        name == "complex" || name == "bool" || name == "array" ||
        name == "duration" || name == "stretch") {
      fail("\"" + name +
               "\" declares a classical variable, which qiskit-cpp has no way "
               "to represent.",
           line);
    }

    // A gate call. The exporter renames two gates on the way out, so those are
    // inverted here before the name is looked up.
    advance();
    std::string gate_name = name;
    if (gate_name == "U") {
      gate_name = "u";
    } else if (gate_name == "gphase") {
      gate_name = "global_phase";
    } else if (gate_name == "CX") {
      // OpenQASM 3's built-in spelling of cx.
      gate_name = "cx";
    }

    // Built per call rather than held in a static, as the exporter does. An
    // Instruction owns Parameter objects, which hold QkParam handles; a
    // function-local static would free them at program exit, after main has
    // returned, which is not a defined point to be calling into Qiskit.
    const std::unordered_map<std::string, Instruction> name_map =
        get_standard_gate_name_mapping();
    auto found = name_map.find(gate_name);
    if (found == name_map.end()) {
      fail("\"" + name +
               "\" is not a gate qiskit-cpp can import. Only the gates in "
               "get_standard_gate_name_mapping are supported.",
           line);
    }

    std::vector<Parameter> params;
    if (accept_symbol("(")) {
      if (!at_symbol(")")) {
        for (;;) {
          params.push_back(parse_expression());
          if (!accept_symbol(",")) {
            break;
          }
        }
      }
      expect_symbol(")");
    }

    const reg_t qubits = parse_qubit_list();
    expect_symbol(";");

    Instruction op = found->second;
    const uint_t expected_params = gate_num_params(op);
    if (params.size() != expected_params) {
      std::stringstream message;
      message << "\"" << name << "\" takes " << expected_params
              << " parameter(s), but " << params.size() << " were given.";
      fail(message.str(), line);
    }

    // gphase before any operation is the circuit's global phase property
    // rather than an instruction. The exporter writes the property directly
    // after the declarations and before the first operation, so the position
    // is what distinguishes the two.
    if (gate_name == "global_phase" && !seen_operation && qubits.empty()) {
      if (params.empty()) {
        // Only reachable if the C-API reports that gphase takes no parameter,
        // in which case the count check above would have accepted "gphase();".
        fail("\"" + name + "\" needs a phase, as in \"gphase(0.5);\".", line);
      }
      circuit.global_phase(params[0]);
      return;
    }

    const uint_t expected_qubits = gate_num_qubits(op);
    if (qubits.size() != expected_qubits) {
      std::stringstream message;
      message << "\"" << name << "\" acts on " << expected_qubits
              << " qubit(s), but " << qubits.size() << " were given.";
      fail(message.str(), line);
    }

    // A qubit used twice in one gate call is not a valid circuit.
    for (uint_t i = 0; i < qubits.size(); i++) {
      for (uint_t j = i + 1; j < qubits.size(); j++) {
        if (qubits[i] == qubits[j]) {
          fail("\"" + name + "\" names qubit " + std::to_string(qubits[i]) +
                   " more than once.",
               line);
        }
      }
    }

    if (!params.empty()) {
      op.set_params(params);
    }
    // append() compares qubits.size() against op.num_qubits() and silently
    // does nothing when they differ, so the instruction has to carry the count
    // the C-API reported rather than whatever the gate's constructor set. These
    // disagree for mcx, where the constructor says 3 and QkGate_RC3X is a
    // 4-qubit gate; without this the gate would be dropped instead of added.
    if (op.num_qubits() != expected_qubits) {
      op.set_num_qubits(expected_qubits);
    }
    seen_operation = true;
    circuit.append(op, qubits);
  }

  /// @brief Parse the program header, which both versions of the exporter
  ///   always write.
  void parse_header(void) {
    if (!at_identifier("OPENQASM")) {
      fail_here("the program must start with \"OPENQASM 3.0;\".");
    }
    advance();
    const Token &version = peek();
    if (version.kind != Token::Kind::Number) {
      fail_here("expected an OpenQASM version but found \"" + version.text +
                "\".");
    }
    advance();
    // Only version 3 is accepted: an OpenQASM 2 program uses "qreg"/"creg" and
    // different gate names, so reading it as 3 would misinterpret it.
    if (version.value < 3.0 || version.value >= 4.0) {
      fail("OpenQASM version " + version.text +
               " is not supported; this importer reads OpenQASM 3.",
           version.line);
    }
    expect_symbol(";");
  }

  /// @brief Build the circuit by replaying the program's statements.
  /// @param circuit the circuit to populate, built in the caller's own object
  /// @details Takes the circuit by reference rather than returning one. The
  ///   registers hold bits pointing back at the register that owns them, and
  ///   neither ``Register``'s copy constructor nor ``QuantumCircuit``'s
  ///   re-points them, so a circuit copied out of this function would hold bits
  ///   addressing a destroyed register. Under C++11 a return by value is not
  ///   guaranteed to elide the copy, so the circuit is assembled where it will
  ///   live.
  void parse_program(QuantumCircuit &circuit) {
    parse_header();
    collect_declarations();

    // Lay the classical registers out in declaration order, which is the order
    // the exporter writes them and so the order their base indices follow.
    uint_t base = 0;
    creg_base_.clear();
    creg_index_.clear();
    for (std::size_t i = 0; i < decls_.cregs.size(); i++) {
      creg_base_.push_back(base);
      creg_index_[decls_.cregs[i].first] = i;
      base += decls_.cregs[i].second;
    }

    for (auto it = decls_.inputs.begin(); it != decls_.inputs.end(); ++it) {
      symbols_[*it] = Parameter(*it);
    }

    build_circuit(circuit);

    bool seen_operation = false;
    for (;;) {
      const Token &token = peek();
      if (token.kind == Token::Kind::End) {
        break;
      }

      if (token.kind == Token::Kind::Identifier) {
        const std::string &keyword = token.text;

        if (keyword == "include") {
          advance();
          if (peek().kind != Token::Kind::String) {
            fail_here("expected a quoted file name after \"include\".");
          }
          advance();
          expect_symbol(";");
          continue;
        }

        if (keyword == "OPENQASM") {
          fail("\"OPENQASM\" may only appear once, at the start of the "
               "program.",
               token.line);
        }

        // Declarations were read in the first pass; skip over them here.
        if (keyword == "qubit") {
          advance();
          skip_optional_width();
          expect_any_identifier("a register name");
          expect_symbol(";");
          continue;
        }
        if (keyword == "bit" || keyword == "creg") {
          advance();
          skip_optional_width();
          expect_any_identifier("a register name");
          expect_symbol(";");
          continue;
        }
        if (keyword == "input" || keyword == "output") {
          advance();
          expect_any_identifier("a parameter type");
          skip_optional_width();
          expect_any_identifier("a parameter name");
          expect_symbol(";");
          continue;
        }

        if (keyword == "gate") {
          const uint_t line = token.line;
          advance();
          const std::string gate_name =
              expect_any_identifier("a gate name");
          if (exporter_gate_definitions().count(gate_name) == 0) {
            fail("\"" + gate_name +
                     "\" is defined here, but this importer cannot inline a "
                     "gate definition. Only the definitions the qiskit-cpp "
                     "exporter writes for its own gates are recognised.",
                 line);
          }
          // The signature, then the body.
          if (accept_symbol("(")) {
            while (!at_symbol(")")) {
              if (peek().kind == Token::Kind::End) {
                fail("the parameter list of \"" + gate_name +
                         "\" is never closed.",
                     line);
              }
              advance();
            }
            expect_symbol(")");
          }
          while (!at_symbol("{")) {
            if (peek().kind == Token::Kind::End) {
              fail("the definition of \"" + gate_name +
                       "\" has no body.",
                   line);
            }
            advance();
          }
          skip_braced_body();
          continue;
        }

        // "c[0] = measure q[0];" starts with a classical register name.
        if (creg_index_.count(keyword) != 0 &&
            (at_symbol("=", 1) ||
             (at_symbol("[", 1) && statement_is_assignment()))) {
          parse_measure_assignment(circuit);
          seen_operation = true;
          continue;
        }

        parse_operation(circuit, seen_operation);
        continue;
      }

      if (token.kind == Token::Kind::Physical) {
        fail("a statement cannot start with a qubit reference.", token.line);
      }

      fail("\"" + token.text + "\" does not start a statement this importer "
                               "understands.",
           token.line);
    }
  }

  /// @brief Decide whether the statement at the cursor is an assignment.
  /// @details Distinguishes "c[0] = measure q[0];" from a gate call on a
  ///   register that happens to share a name, by looking ahead for the "="
  ///   before the ";".
  bool statement_is_assignment(void) const {
    for (std::size_t ahead = 1; ahead < tokens_.size(); ahead++) {
      if (peek(ahead).kind == Token::Kind::End || at_symbol(";", ahead)) {
        return false;
      }
      if (at_symbol("=", ahead)) {
        return true;
      }
    }
    return false;
  }

  /// @brief Construct the circuit the declarations describe, in place.
  /// @param circuit the circuit to build into
  /// @details ``Register``'s copy constructor and assignment copy ``bits_``
  ///   without re-pointing each bit at its new owner, and
  ///   ``Bit::global_index()`` follows that pointer. The registers handed to
  ///   the ``QuantumCircuit`` constructor are locals that die when this
  ///   function returns, so every register stored in the circuit is re-sized
  ///   afterwards: ``resize()`` runs ``allocate_bits()``, which re-points every
  ///   bit at the stored register. This is the same fix ``QpyImporter`` applies
  ///   for the same reason, and it is why the circuit is built into the
  ///   caller's object rather than returned.
  void build_circuit(QuantumCircuit &circuit) {
    // The exporter writes a single merged quantum register, so one is built
    // here. Its name is kept so that a re-export names it the same way.
    const std::string qreg_name =
        decls_.qreg_name.empty() ? std::string("q") : decls_.qreg_name;

    circuit.num_qubits_ = decls_.num_qubits;
    circuit.num_clbits_ = 0;
    circuit.global_phase_ = Parameter(0.0);
    circuit.measure_map_.clear();
    // Left empty deliberately: a "$n" reference carries no transpiler layout,
    // so there is nothing to recover. from_qpy does the same.
    circuit.qubit_map_.clear();

    circuit.rust_circuit_ = std::shared_ptr<rust_circuit>(qk_circuit_new(0, 0),
                                                          qk_circuit_free);
    if (!circuit.rust_circuit_) {
      throw std::runtime_error(
          "QASM3 import failed : could not create the circuit.");
    }

    // Built in place and sized through resize() so that every bit points at
    // the register stored in the circuit. Reserved first so the push_backs
    // cannot reallocate and leave those pointers behind.
    circuit.qregs_.clear();
    circuit.qregs_.reserve(1);
    circuit.qregs_.push_back(QuantumRegister(decls_.num_qubits, qreg_name));
    QuantumRegister &stored_qreg = circuit.qregs_.back();
    stored_qreg.resize(decls_.num_qubits);
    stored_qreg.set_base_index(0);
    qk_circuit_add_quantum_register(circuit.rust_circuit_.get(),
                                    stored_qreg.get_register().get());

    circuit.cregs_.clear();
    if (decls_.cregs.empty()) {
      // QuantumCircuit always holds a classical register; an empty one keeps
      // the shape consistent with the (num_qubits, 0) constructor.
      circuit.cregs_.reserve(1);
      circuit.cregs_.push_back(ClassicalRegister(0, std::string("c")));
      ClassicalRegister &stored = circuit.cregs_.back();
      stored.resize(0);
      stored.set_base_index(0);
      qk_circuit_add_classical_register(circuit.rust_circuit_.get(),
                                        stored.get_register().get());
      return;
    }

    circuit.cregs_.reserve(decls_.cregs.size());
    for (std::size_t i = 0; i < decls_.cregs.size(); i++) {
      circuit.cregs_.push_back(
          ClassicalRegister(decls_.cregs[i].second, decls_.cregs[i].first));
      ClassicalRegister &stored = circuit.cregs_.back();
      stored.resize(decls_.cregs[i].second);
      stored.set_base_index(creg_base_[i]);
      qk_circuit_add_classical_register(circuit.rust_circuit_.get(),
                                        stored.get_register().get());
      circuit.num_clbits_ += decls_.cregs[i].second;
    }
  }

public:
  /// @brief Build a QuantumCircuit from an OpenQASM 3 program.
  /// @param circuit the circuit to build into
  /// @param source the program text
  /// @throws std::invalid_argument if the program holds anything this importer
  ///   does not support, naming the line it is on
  /// @details Populates the caller's circuit rather than returning one; see
  ///   ``parse_program`` for why.
  static void load_string(QuantumCircuit &circuit, const std::string &source) {
    Qasm3Importer importer;
    importer.tokenize(source);
    importer.parse_program(circuit);
  }

  /// @brief Build a QuantumCircuit from an OpenQASM 3 file.
  /// @param circuit the circuit to build into
  /// @param filename the path to read
  /// @throws std::runtime_error if the file cannot be read
  /// @throws std::invalid_argument if the program holds anything this importer
  ///   does not support
  static void load_file(QuantumCircuit &circuit, const std::string &filename) {
    std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
    if (!file) {
      throw std::runtime_error("QASM3 import failed : could not open \"" +
                               filename + "\".");
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    if (file.bad()) {
      throw std::runtime_error("QASM3 import failed : could not read \"" +
                               filename + "\".");
    }
    load_string(circuit, buffer.str());
  }
};

inline QuantumCircuit QuantumCircuit::from_qasm3(const std::string &qasm3) {
  QuantumCircuit circuit;
  Qasm3Importer::load_string(circuit, qasm3);
  return circuit;
}

inline QuantumCircuit QuantumCircuit::import_qasm3(const std::string &filename) {
  QuantumCircuit circuit;
  Qasm3Importer::load_file(circuit, filename);
  return circuit;
}

} // namespace circuit
} // namespace Qiskit

#endif // __qiskitcpp_circuit_qasm3_importer_hpp__
