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

using Sampler = BackendSamplerV2;

int main(int argc, char** argv)
{
  auto service = Qiskit::service::QiskitRuntimeService();
  auto backends = service.backends();

  std::string filename = "checkpoint.qcp";
  if (argc > 1)
    filename = argv[1];

  auto backend = service.least_busy();

  int num_qubits = 10;
  auto qreg = QuantumRegister(num_qubits);
  auto creg = ClassicalRegister(num_qubits, std::string("meas"));
  QuantumCircuit circ(std::vector<QuantumRegister>({qreg,}), std::vector<ClassicalRegister>({creg}));

  // GHZ circuit
  circ.h(0);
  for (int i = 0; i < num_qubits - 1; i++)
  {
      circ.cx(i, i + 1);
  }
  circ.measure(qreg, creg);

  auto sampler = Sampler(backend, 100);

  auto transpiled_circ = transpile(circ, backend);

  auto job = sampler.run({SamplerPub(transpiled_circ)});
  if (job == nullptr)
      return -1;

  job->checkpoint(filename);

  return 0;
}

