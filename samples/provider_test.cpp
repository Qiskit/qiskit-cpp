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

// Test program for sampler

#define _USE_MATH_DEFINES
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <cmath>

#include "circuit/quantumcircuit.hpp"
#include "primitives/backend_sampler_v2.hpp"

#ifdef QRMI_ROOT
#include "service/qiskit_runtime_service_qrmi.hpp"
#elif defined(SQC_ROOT)
#include "service/qiskit_runtime_service_sqc.hpp"
#else
#include "service/qiskit_runtime_service_c.hpp"
#endif
#include "compiler/transpiler.hpp"

using namespace Qiskit::circuit;
using namespace Qiskit::providers;
using namespace Qiskit::primitives;
using namespace Qiskit::compiler;

int main(int argc, char** argv)
{
  auto service = Qiskit::service::QiskitRuntimeService();
  auto backends = service.backends();

  std::cout << " Available backends :" << std::endl;
  for (size_t i = 0; i < backends.size(); i++) {
    std::cout << backends[i] << std::endl;
  }

  auto backend = service.least_busy();
  return 0;
}

