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

#include "circuit/quantumcircuit.hpp"

extern "C" {
    #include "common.h"
}

static int test_circuit_new(void) {
    auto circ = Qiskit::circuit::QuantumCircuit(2, 2);
    return Ok;
}


extern "C" int test_circuit(void) {
    int num_failed = 0;
    num_failed += RUN_TEST(test_circuit_new);

    std::cerr << "=== Number of failed subtests: " << num_failed << std::endl;
    return num_failed;
}
