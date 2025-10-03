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

#ifndef __qiskitcpp_utils_hpp__
#define __qiskitcpp_utils_hpp__

#include "utils/types.hpp"

#if defined(__linux__)
#include <unistd.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/types.h>
#elif defined(_MSC_VER)
#include <windows.h>
#endif

namespace Qiskit {


#ifdef _MSC_VER
#define POPCNT __popcnt64
#define INTRINSIC_PARITY 1
inline bool _intrinsic_parity(uint_t x) { return (POPCNT(x) & 1U); }
inline uint_t _instrinsic_weight(uint_t x) { return (POPCNT(x)); }
#endif
#ifdef __GNUC__
#define INTRINSIC_PARITY 1
inline bool _intrinsic_parity(uint_t x) {
    return (__builtin_popcountll(x) & 1U);
}
inline uint_t _instrinsic_weight(uint_t x) { return (__builtin_popcountll(x)); }
#endif
#ifdef _CLANG_
#if __has__builtin(__builtin_popcount)
#define INTRINSIC_PARITY 1
inline bool _intrinsic_parity(uint_t x) {
    return (__builtin_popcountll(x) & 1U);
}
inline uint_t _instrinsic_weight(uint_t x) { return (__builtin_popcountll(x)); }
#endif
#endif

uint_t _naive_weight(uint_t x) {
    auto count = x;
    count = (count & 0x5555555555555555) + ((count >> 1) & 0x5555555555555555);
    count = (count & 0x3333333333333333) + ((count >> 2) & 0x3333333333333333);
    count = (count & 0x0f0f0f0f0f0f0f0f) + ((count >> 4) & 0x0f0f0f0f0f0f0f0f);
    count = (count & 0x00ff00ff00ff00ff) + ((count >> 8) & 0x00ff00ff00ff00ff);
    count = (count & 0x0000ffff0000ffff) + ((count >> 16) & 0x0000ffff0000ffff);
    count = (count & 0x00000000ffffffff) + ((count >> 32) & 0x00000000ffffffff);
    return count;
}

inline bool _naive_parity(uint_t x) {
    return (_naive_weight(x) & 1U);
}


#ifdef INTRINSIC_PARITY
bool (*hamming_parity)(uint_t) = &_intrinsic_parity;
uint_t (*popcount)(uint_t) = &_instrinsic_weight;
#else
bool (*hamming_parity)(uint_t) = &_naive_parity;
uint_t (*popcount)(uint_t) = &_naive_weight;
#endif


} // namespace Qiskit

#endif

