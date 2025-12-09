/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2017, 2025.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.
*/

// Qiskit Runtime Service wrapper for backward compatibility
// use one of "service/qiskit_runtime_service_qrmi.hpp" or "service/qiskit_runtime_service_c.hpp" directly

#ifdef QRMI_ROOT
#include "service/qiskit_runtime_service_qrmi.hpp"
#else   // otherwise use Qiskit IBM runtime C-API
#include "service/qiskit_runtime_service_c.hpp"
#endif

