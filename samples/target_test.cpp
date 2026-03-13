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

// Test program for local target

#define _USE_MATH_DEFINES
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <cmath>

#include "circuit/quantumcircuit.hpp"
#include "transpiler/target.hpp"
#include "transpiler/passmanager.hpp"
#include "transpiler/preset_passmanagers/generate_preset_pass_manager.hpp"

using namespace Qiskit::circuit;
using namespace Qiskit::transpiler;

int main()
{
    QuantumRegister qr(4);
    ClassicalRegister cr(4);
    QuantumCircuit circ(qr, cr);

    circ.h(0);
    for (int i=1;i<4;i++) {
        circ.cx(0, i);
    }
    for (int i=0;i<4;i++) {
        circ.measure(i,i);
    }

    std::cout << "input circuit" << std::endl;
    circ.print();

    auto target = Target({"cz", "id", "rx", "rz", "rzz", "sx", "x"}, {{0, 1}, {1, 0}, {1, 3}, {3, 1}, {0, 2}, {2, 0}, {2, 3}, {3, 2}});
    auto pass = StagedPassManager({"init", "layout", "routing", "translation"}, target);
    auto transpiled = pass.run(circ);

    std::cout << "transpiled circuit" << std::endl;
    transpiled.print();

    return 0;
}

