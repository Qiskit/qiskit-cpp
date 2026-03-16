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

#include "test_circuit.hpp"
#include "test_parameter.hpp"
#include "test_transpiler.hpp"

extern "C" int test_main(void) {
    int num_failed = 0;

    num_failed += RUN_TEST(test_circuit);
    num_failed += RUN_TEST(test_parameter);
    num_failed += RUN_TEST(test_transpiler);

    return num_failed;
}
