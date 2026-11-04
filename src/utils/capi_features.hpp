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

// Feature detection for the Qiskit C-API
//
// Qiskit C++ supports a range of Qiskit versions, so functionality that is only
// available from a given Qiskit release has to be compiled conditionally. Each
// macro below is defined to 1 when the C-API provides the feature and to 0 when
// it does not, so they are always safe to test with ``#if``. Every macro can be
// pre-defined by the application to override the detection.

#ifndef __qiskitcpp_utils_capi_features_hpp__
#define __qiskitcpp_utils_capi_features_hpp__

#include "qiskit.h"

// QPY serialization (``qk_qpy_*``), added in Qiskit 2.6.0.
//
// The comparison uses the "dev" release level so that pre-releases of the
// Qiskit version that introduced the feature are also accepted.
#ifndef QISKIT_CPP_HAS_QPY
#if defined(QISKIT_VERSION_HEX) && defined(QISKIT_GET_VERSION_HEX) &&          \
    defined(QISKIT_RELEASE_LEVEL_DEV)
#if QISKIT_VERSION_HEX >=                                                      \
    QISKIT_GET_VERSION_HEX(2, 6, 0, QISKIT_RELEASE_LEVEL_DEV, 0)
#define QISKIT_CPP_HAS_QPY 1
#else
#define QISKIT_CPP_HAS_QPY 0
#endif
#else
// The version macros are expected in every supported Qiskit release. If they
// are missing we cannot tell what the C-API provides, so assume the worst.
#define QISKIT_CPP_HAS_QPY 0
#endif
#endif // QISKIT_CPP_HAS_QPY

#endif //__qiskitcpp_utils_capi_features_hpp__
