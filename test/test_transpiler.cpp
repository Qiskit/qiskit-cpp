// This code is part of Qiskit.
//
// (C) Copyright IBM 2025.
//
// This code is licensed under the Apache License, Version 2.0. You may
// obtain a copy of this license in the LICENSE.txt file in the root directory
// of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.

#include <iostream>
#include <cstdint>

#define _USE_MATH_DEFINES
#include <math.h>

#include "circuit/quantumcircuit.hpp"
#include "transpiler/target.hpp"
#include "transpiler/preset_passmanagers/generate_preset_pass_manager.hpp"

using namespace Qiskit;
using namespace Qiskit::circuit;
using namespace Qiskit::transpiler;

extern "C" {
    #include "common.h"
}

static int test_translate_h(void)
{
    QuantumRegister qr(1);
    ClassicalRegister cr(1);
    QuantumCircuit circ(qr, cr);

    circ.h(0);

    auto target = Target({"cz", "id", "rx", "rz", "sx", "x"}, {});
    auto pass = StagedPassManager({"init", "layout", "routing", "translation"}, target, 1.0, 1);
    auto transpiled = pass.run(circ);

    QuantumCircuit circ_ref(1, 1);
    circ_ref.rz(M_PI / 2.0, 0);
    circ_ref.sx(0);
    circ_ref.rz(M_PI / 2.0, 0);

    if (transpiled != circ_ref) {
        return EqualityError;
    }
    return Ok;
}

static int test_translate_cx(void)
{
    QuantumRegister qr(2);
    ClassicalRegister cr(2);
    QuantumCircuit circ(qr, cr);

    circ.cx(0, 1);

    auto target = Target({"cz", "id", "rx", "rz", "sx", "x"}, {{0, 1}, {1, 0}});
    auto pass = StagedPassManager({"init", "layout", "routing", "translation"}, target, 1.0, 1);
    auto transpiled = pass.run(circ);

    QuantumCircuit circ_ref(2, 2);
    circ_ref.rz(M_PI / 2.0, 1);
    circ_ref.sx(1);
    circ_ref.rz(M_PI / 2.0, 1);
    circ_ref.cz(0, 1);
    circ_ref.rz(M_PI / 2.0, 1);
    circ_ref.sx(1);
    circ_ref.rz(M_PI / 2.0, 1);

    if (transpiled != circ_ref) {
        return EqualityError;
    }
    return Ok;
}

static int test_ghz_routing(void)
{
    QuantumRegister qr(3);
    ClassicalRegister cr(3);
    QuantumCircuit circ(qr, cr);

    circ.h(0);
    for (int i = 1; i < 3; i++) {
        circ.cx(0, i);
    }
    circ.measure(qr, cr);

    auto target = Target({"h", "cx"}, {{0, 1}, {1, 0}, {1, 2}, {2, 1}});
    auto pass = StagedPassManager({"init", "layout", "routing"}, target);
    auto transpiled = pass.run(circ);

    QuantumCircuit circ_ref(3, 3);
    circ_ref.h(1);
    circ_ref.cx(1, 0);
    circ_ref.cx(1, 2);
    circ_ref.measure(1, 0);
    circ_ref.measure(0, 1);
    circ_ref.measure(2, 2);

    if (transpiled != circ_ref) {
        return EqualityError;
    }
    return Ok;
}


extern "C" int test_circuit(void) {
    int num_failed = 0;
    num_failed += RUN_TEST(test_translate_h);
    num_failed += RUN_TEST(test_translate_cx);
    num_failed += RUN_TEST(test_ghz_routing);

    std::cerr << "=== Number of failed subtests: " << num_failed << std::endl;
    return num_failed;
}
