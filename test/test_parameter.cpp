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
#include <cmath>

#include "circuit/parameter.hpp"
using namespace Qiskit;
using namespace Qiskit::circuit;

extern "C" {
    #include "common.h"
}

/**
 * Test creating a new free symbol and check the name.
 */
int test_parameter_new(void) {
    auto p = Parameter("a");

    if (p.as_str() != "a") {
        std::cerr << "The parameter " << p.as_str() << " is not a" << std::endl;
        return EqualityError;
    }
    return Ok;
}

/**
 * Test creating a new symbol from values.
 */
int test_parameter_new_values(void) {
    auto real = Parameter(0.1);
    if (real.as_str() != "0.1") {
        std::cerr << "The parameter value " << real.as_str() << " is not 0.1" << std::endl;
        return EqualityError;
    }
    return Ok;
}

/**
 * Test casting to real values.
 */
int test_parameter_to_real(void) {
    auto x = Parameter("x");
    auto cmplx = Parameter(std::complex<double>({1.0, 2.0}));
    auto val = Parameter(10.0);

    double x_out = x.as_real();
    double cmplx_out = cmplx.as_real();
    double val_out = val.as_real();

    if (!std::isnan(x_out) || std::isnan(cmplx_out) || std::isnan(val_out)) {
        std::cerr << "Unexpected success/failure in as_real." << std::endl;
        return EqualityError;
    }

    if (std::abs(val_out - 10.0) > 1e-10) {
        std::cerr << "Unexpected extracted value in as_real." << std::endl;
        return EqualityError;
    }

    // qk_param_as_real extracts the real part of a complex value
    if (std::abs(cmplx_out - 1.0) > 1e-10) {
        std::cerr << "Unexpected extracted value in as_real." << std::endl;
        return EqualityError;
    }

    return Ok;
}

/**
 * Test calling all binary operations and verify their string representation.
 */
int test_parameter_binary_ops(void) {
    auto a = Parameter("a");
    auto b = Parameter("b");
    auto ret = Parameter();
    int result = Ok;

    // add
    ret = a + b;
    if (ret.as_str() != "a + b") {
        std::cerr << "qk_param_add " << ret.as_str() << " is not a + b" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    // sub
    ret = a - b;
    if (ret.as_str() != "a - b") {
        std::cerr << "qk_param_sub " << ret.as_str() << " is not a - b" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    // mul
    ret = a * b;
    if (ret.as_str() != "a*b") {
        std::cerr << "qk_param_mul " << ret.as_str() << " is not a*b" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    // div
    ret = a / b;
    if (ret.as_str() != "a/b") {
        std::cerr << "qk_param_div " << ret.as_str() << " is not a/b" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    // pow
    ret = a.pow(b);
    if (ret.as_str() != "a**b") {
        std::cerr << "qk_param_pow " << ret.as_str() << " is not a**b" << std::endl;
        result = EqualityError;
        goto cleanup;
    }
cleanup:
    return result;
}

/**
 * Test calling all unary operations and verify their string representation.
 */
int test_parameter_unary_ops(void) {
    auto a = Parameter("a");
    auto b = Parameter("b");
    auto c = a + b;
    auto ret = Parameter();
    int result = Ok;

    // sin
    ret = c.sin();
    if (ret.as_str() != "sin(a + b)") {
        std::cerr << "qk_param_sin " << ret.as_str() << " is not sin(a + b)" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    // cos
    ret = c.cos();
    if (ret.as_str() != "cos(a + b)") {
        std::cerr << "qk_param_cos " << ret.as_str() << " is not cos(a + b)" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    // tan
    ret = c.tan();
    if (ret.as_str() != "tan(a + b)") {
        std::cerr << "qk_param_tan " << ret.as_str() << " is not tan(a + b)" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    // asin
    ret = c.asin();
    if (ret.as_str() != "asin(a + b)") {
        std::cerr << "qk_param_asin " << ret.as_str() << " is not asin(a + b)" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    // acos
    ret = c.acos();
    if (ret.as_str() != "acos(a + b)") {
        std::cerr << "qk_param_acos " << ret.as_str() << " is not acos(a + b)" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    // atan
    ret = c.atan();
    if (ret.as_str() != "atan(a + b)") {
        std::cerr << "qk_param_atan " << ret.as_str() << " is not atan(a + b)" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    // log
    ret = c.log();
    if (ret.as_str() != "log(a + b)") {
        std::cerr << "qk_param_log " << ret.as_str() << " is not log(a + b)" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    // exp
    ret = c.exp();
    if (ret.as_str() != "exp(a + b)") {
        std::cerr << "qk_param_exp " << ret.as_str() << " is not exp(a + b)" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    // abs
    ret = c.abs();
    if (ret.as_str() != "abs(a + b)") {
        std::cerr << "qk_param_abs " << ret.as_str() << " is not abs(a + b)" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    // sign
    ret = c.sign();
    if (ret.as_str() != "sign(a + b)") {
        std::cerr << "qk_param_sign " << ret.as_str() << " is not sign(a + b)" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    // neg
    ret = -c;
    if (ret.as_str() != "-a - b") {
        std::cerr << "qk_param_neg " << ret.as_str() << " is not -a - b" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    // conj
    ret = c.conjugate();
    if (ret.as_str() != "conj(a) + conj(b)") {
        std::cerr << "qk_param_conj " << ret.as_str() << " is not conj(a) + conj(b)" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

cleanup:
    return result;
}

/**
 * Test operations with free parameters and fixed values.
 */
int test_parameter_with_value(void) {
    auto a = Parameter("a");
    auto v = Parameter(2.5);
    auto ret = Parameter();
    int result = Ok;

    // add
    ret = a + v;
    if (ret.as_str() != "2.5 + a") {
        std::cerr << "qk_param_add " << ret.as_str() << " is not 2.5 + a" << std::endl;
        result = EqualityError;
    }
    return result;
}

/**
 * Test equality.
 */
int test_parameter_equal(void) {
    Parameter x = Parameter("x");
    Parameter x_imposter = Parameter("x");
    Parameter y = Parameter("y");
    Parameter z = Parameter("z");
    Parameter val = Parameter(0.2);
    Parameter sum1 = Parameter();
    Parameter sum1_clone = Parameter();
    Parameter sum2 = Parameter();
    Parameter mul = Parameter();

    int result = Ok;

    sum1 = x + y;
    sum1_clone = x + y;
    sum2 = x + z;
    mul = x * val;

    if (x != x) {
        std::cerr << "Symbol not equal to itself" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    if (x == x_imposter) {
        std::cerr << "Symbol equal a new instance with the same name" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    if (x == mul) {
        std::cerr << "Symbol equals but they differ by a coefficient" << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    if (sum1 != sum1_clone) {
        std::cerr << "Expression not equal to the same expression." << std::endl;
        result = EqualityError;
        goto cleanup;
    }

    if (sum1 == sum2) {
        std::cerr << "Expression equal to a different sum." << std::endl;
        result = EqualityError;
        goto cleanup;
    }
cleanup:
    return result;
}

/**
 * Test copy.
 */
int test_parameter_copy(void) {
    Parameter x = Parameter("x");
    Parameter y = Parameter("y");
    Parameter sum = Parameter();

    int result = Ok;

    sum = x + y;
    Parameter copy = sum;
    if (sum != copy) {
        std::cerr << "Copy not equal to original";
        result = EqualityError;
    }

    return result;
}

extern "C" int test_parameter(void) {
    int num_failed = 0;
    num_failed += RUN_TEST(test_parameter_new);
    num_failed += RUN_TEST(test_parameter_new_values);
    num_failed += RUN_TEST(test_parameter_to_real);
    num_failed += RUN_TEST(test_parameter_equal);
    num_failed += RUN_TEST(test_parameter_copy);
    num_failed += RUN_TEST(test_parameter_binary_ops);
    num_failed += RUN_TEST(test_parameter_unary_ops);
    num_failed += RUN_TEST(test_parameter_with_value);

    std::cerr << "=== Number of failed subtests: " << num_failed << std::endl;

    return num_failed;
}

