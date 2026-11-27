// This code is part of Qiskit.
//
// (C) Copyright IBM 2024.
//
// This code is licensed under the Apache License, Version 2.0. You may
// obtain a copy of this license in the LICENSE.txt file in the root directory
// of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.

#include "common.hpp"
#include <exception>
#include <iostream>
#include <cstdint>

// A function to run a test function of a given name. This function will also
// post-process the returned `TestResult` to product a minimal info message for
// the developer running the test suite.
int run(const char *name, int (*test_function)(void), bool test_raise_exception) {
    // TODO: we could consider to change the return value of our test functions
    // to be a struct containing the integer return value and a custom error
    // message which could then be included below.
    int result;

    // An exception thrown by a test function would otherwise travel out of here
    // and out of the driver's main, where an uncaught exception terminates the
    // process. One subtest would then take every later subtest in the whole
    // driver with it, and the report would stop at whatever ran last rather
    // than naming the test at fault. Caught here so the subtest is reported as
    // failed, with the reason, and the remaining subtests still run.
    try {
        result = test_function();
    } catch (const std::exception &e) {
        std::cerr << "--- " << name << ": FAILED with an uncaught exception: "
            << e.what() << std::endl;
        return test_raise_exception ? 0 : 1;
    } catch (...) {
        std::cerr << "--- " << name
            << ": FAILED with an uncaught exception that does not derive from "
               "std::exception" << std::endl;
        return test_raise_exception ? 0 : 1;
    }

    int did_fail = 1;
    if (result == Ok) {
        did_fail = 0;
        std::cerr << "--- " << name << ": OK" << std::endl;
    } else if (result == EqualityError) {
        std::cerr << "--- " << name << ": FAILED with an EqualityError" << std::endl;
    } else if (result == RuntimeError) {
        std::cerr << "--- " << name << ": FAILED with a RuntimeError" << std::endl;
    } else if (result == NullptrError) {
        std::cerr << "--- " << name << ": FAILED with an unexpected null pointer"
            << std::endl;
    } else {
        std::cerr << "--- " << name << ": FAILED with unknown error" << std::endl;
    }
    return did_fail;
}
