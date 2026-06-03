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

// OpenQASM 3.0 exporter for QuantumCircuit.
//
// Refactored out of `QuantumCircuit::to_qasm3` (issue #146) so that the exporter
// logic lives outside the (very large) `QuantumCircuit` class definition. The
// exporter supports circuits that contain symbolic `Parameter` expressions, in
// addition to purely numeric ones, by serializing the `QkParam` value of each
// gate parameter with `qk_param_str` (which already returns the symbolic
// expression string for free symbols and their combinations).

#ifndef __qiskitcpp_circuit_qasm3_exporter_hpp__
#define __qiskitcpp_circuit_qasm3_exporter_hpp__

#include <sstream>
#include <string>
#include <vector>

#include "qiskit.h"
#include "utils/types.hpp"
#include "circuit/classicalregister.hpp"

// qiskit C-API circuit data
using rust_circuit = ::QkCircuit;

// Forward declaration (must appear at global/namespace scope, outside any class).
namespace Qiskit { namespace circuit { class QuantumCircuit; } }

// Exporter class declaration.
namespace Qiskit {
namespace circuit {

/// @class Qasm3Exporter
/// @brief Serialize a `QuantumCircuit` as an OpenQASM 3.0 string.
///
/// The exporter is intentionally implemented outside of `QuantumCircuit` so the
/// circuit class definition does not have to know about the textual QASM
/// representation.
///
/// Currently the exporter fully supports all of the standard gates that
/// `QuantumCircuit` knows about, and works for both numeric and symbolic gate
/// parameters (i.e. `Parameter`, including compound expressions built from
/// free symbols). Free symbols are emitted as bare identifiers in the resulting
/// QASM source (e.g. `rx(theta) q[0];`).
class Qasm3Exporter {
public:
    /// @brief Construct an exporter that targets the supplied circuit.
    /// @param circ The circuit to serialize.
    Qasm3Exporter(QuantumCircuit *circ);

    /// @brief Serialize the bound circuit as an OpenQASM 3.0 string.
    /// @return The OpenQASM 3.0 representation of the circuit.
    std::string run(void);

private:
    QuantumCircuit *circ_;

    rust_circuit *rust_circuit_ptr(void) const;

    static std::string param_to_string(const QkParam *param);

    void write_header(std::stringstream &out) const;
    void write_gate_definitions(std::stringstream &out);
    void write_registers(std::stringstream &out) const;
    void write_instructions(std::stringstream &out);
    // Note: using ::uint_t and ::ClassicalRegister since these are defined in
    // the outer namespace scope by quantumcircuit_def.hpp after qiskit.h.
    static std::pair<std::string, uint_t> recover_creg_data(
        const std::vector<ClassicalRegister> &cregs, uint_t index);
};

} // namespace circuit
} // namespace Qiskit

#endif  // __qiskitcpp_circuit_qasm3_exporter_hpp__
