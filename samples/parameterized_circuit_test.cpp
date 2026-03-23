/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2024. 2026.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.
*/

// Test parameterized circuit of Qiskit C++

#include <iostream>
#include <cstdint>

#include "circuit/quantumcircuit.hpp"

using namespace Qiskit::circuit;

int main()
{
    QuantumRegister qr(4);
    ClassicalRegister cr(4);
    QuantumCircuit circ(qr, cr);

    Parameter theta("t");
    Parameter a = theta + 0.5;

    circ.h(0);
    circ.rx(a, 1);

    circ.measure(qr, cr);

    circ.print();

    circ.draw();

    return 0;
}

