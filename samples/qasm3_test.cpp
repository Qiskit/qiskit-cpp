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

// Test quantm circuit of Qiskit C++

#include <iostream>
#include <cstdint>

#include "circuit/quantumcircuit.hpp"
#include "circuit/library/quantum_volume.hpp"

using namespace Qiskit::circuit;

int main()
{
    int num_qubits = 10;
    auto qreg = QuantumRegister(num_qubits);
    auto creg = ClassicalRegister(num_qubits, std::string("meas"));
    auto ctest = ClassicalRegister(num_qubits, std::string("test"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qreg,}), std::vector<ClassicalRegister>({creg, ctest}));

    Parameter theta("t");
    Parameter a = theta + 0.5;
    Parameter b = theta - 0.5;

    // GHZ circuit
    circ.h(0);
    for (int i = 0; i < num_qubits - 1; i++)
    {
        circ.cx(i, i + 1);
    }
    circ.xx_plus_yy(a, b, 0, 1);
    circ.measure(qreg, creg);

    std::cout << " input circuit : " << std::endl;
    circ.draw();

    auto qasm3 = circ.to_qasm3();

    std::cout << qasm3 << std::endl;


    auto circ_out = QuantumCircuit::from_qasm3(qasm3);

    std::cout << " from QASM3 circuit : " << std::endl;
    circ_out.draw();

    return 0;
}

