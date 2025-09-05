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

#include "service/qiskit_runtime_service.hpp"
#include "compiler/transpiler.hpp"

using namespace Qiskit::circuit;
using namespace Qiskit::providers;
using namespace Qiskit::primitives;
using namespace Qiskit::compiler;

int main()
{
  auto service = Qiskit::service::QiskitRuntimeService();
  auto backend = service.backend("ibm_torino");

  QuantumRegister qr(10);
  ClassicalRegister cr(10);
  QuantumCircuit circ(qr, cr);

  circ.h(0);
  for (int i=0;i<9;i++) {
    circ.cx(i, i+1);
  }
  circ.rzz(0.1, 0, 1);
  for (int i=0;i<10;i++) {
    circ.measure(i,i);
  }

  std::cout << "input circuit" << std::endl;
  circ.print();

  auto transpiled = transpile(circ, backend);

  std::cout << "transpiled circuit" << std::endl;
  transpiled.print();

  return 0;
}

