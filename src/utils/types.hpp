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

#ifndef __qiskitcpp_utils_types_hpp__
#define __qiskitcpp_utils_types_hpp__

#include <complex>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace qiskitcpp {

// Numeric Types
using int_t = int_fast64_t;
using uint_t = uint_fast64_t;
using int32_t = std::int32_t;
using uint32_t = std::uint32_t;
using complex_t = std::complex<double>;
using complexf_t = std::complex<float>;
using cvector_t = std::vector<complex_t>;
using cvectorf_t = std::vector<complexf_t>;
using rvector_t = std::vector<double>;
using reg_t = std::vector<uint_t>;

} // namespace qiskitcpp
#endif

