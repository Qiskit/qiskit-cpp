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

// Test QPY export of Qiskit C++

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "circuit/quantumcircuit.hpp"

using namespace Qiskit::circuit;

#if !QISKIT_CPP_HAS_QPY
int main()
{
  std::cerr << "QPY export requires Qiskit 2.6.0 or later." << std::endl;
  return 1;
}
#else
int main()
{
  std::cout << "QPY versions writable from " << (unsigned)QuantumCircuit::qpy_write_min_version()
            << ", readable from " << (unsigned)QuantumCircuit::qpy_read_min_version() << std::endl;

  QuantumRegister qr(2);
  ClassicalRegister cr(2);
  QuantumCircuit circ(qr, cr);

  circ.h(0);
  circ.cx(0, 1);
  circ.measure(qr, cr);

  try {
    // Write a single circuit to a file. Load it back in Python with
    //   from qiskit import qpy
    //   with open("bell.qpy", "rb") as f:
    //       circuits = qpy.load(f)
    circ.export_qpy("bell.qpy");
    std::cout << "wrote bell.qpy" << std::endl;

    // The same payload is available in memory.
    const std::vector<std::uint8_t> payload = circ.to_qpy();
    std::cout << "in-memory payload : " << payload.size() << " bytes" << std::endl;

    // Several circuits can share one payload.
    QuantumCircuit parameterized(1, 0);
    auto theta = Parameter("theta");
    parameterized.rx(theta, 0);

    std::vector<QuantumCircuit> circuits;
    circuits.push_back(circ);
    circuits.push_back(parameterized);
    QuantumCircuit::export_qpy(circuits, "circuits.qpy");
    std::cout << "wrote circuits.qpy with " << circuits.size() << " circuits" << std::endl;

    // A specific format version can be requested, for compatibility with an
    // older reader.
    circ.export_qpy("bell_min.qpy", QuantumCircuit::qpy_write_min_version());
    std::cout << "wrote bell_min.qpy at QPY version "
              << (unsigned)QuantumCircuit::qpy_write_min_version() << std::endl;

    // Reading a QPY file always yields a list, since a payload may hold more
    // than one circuit. This also reads files written by Python Qiskit's
    //   qpy.dump(circuits, f)
    std::vector<QuantumCircuit> imported = QuantumCircuit::import_qpy("circuits.qpy");
    std::cout << "read circuits.qpy : " << imported.size() << " circuits" << std::endl;
    for (std::size_t i = 0; i < imported.size(); i++) {
      std::cout << "  circuit " << i << " : " << imported[i].num_qubits() << " qubits, "
                << imported[i].num_clbits() << " clbits, "
                << imported[i].num_parameters() << " parameters" << std::endl;
    }

    // The same payload can be read straight from memory.
    const std::vector<QuantumCircuit> round_tripped = QuantumCircuit::from_qpy(payload);
    if (round_tripped.size() == 1 && round_tripped[0] == circ) {
      std::cout << "round trip through QPY preserved the circuit" << std::endl;
    } else {
      std::cerr << "round trip through QPY changed the circuit" << std::endl;
      return 1;
    }

    imported[0].draw();
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    return 1;
  }

  return 0;
}
#endif // QISKIT_CPP_HAS_QPY
