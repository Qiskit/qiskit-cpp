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

// Test quantm circuit of Qiskit C++

#include <iostream>
#include <cstdint>

#include "circuit/quantumcircuit.hpp"
#include "circuit/library/quantum_volume.hpp"

using namespace Qiskit::circuit;

int main()
{
  QuantumRegister qr(4);
  ClassicalRegister cr(4);
  QuantumCircuit circ(qr, cr);

  double theta_A = 3.1;
  double theta_B = 2.5;
  double phi = -0.05;
  double lambda = 1.3;

  circ.h(0);
  circ.x(0);

  circ.reset(qr);

  circ.x(1);
  circ.cx(0,1);
  circ.rz(theta_A, 0);
  circ.ry(theta_B + theta_A, 1);

  circ.p(0.2, 1);

  circ.rz(theta_B*2.0, 1);

  circ.u(theta_A - theta_B, phi, lambda, 0);

  circ.measure(0, 0);

  circ.sx(0);
  circ.x(1);

  circ.xx_plus_yy(0.5, 0.0, 0, 1);

  QuantumCircuit sub_circ(2, 2);
  sub_circ.cz(0, 1);
  sub_circ.rxx(-3.1, 1, 0);
  sub_circ.cz(0, 1);
  sub_circ.barrier({0, 1});
  sub_circ.measure(0, 0);

  circ.compose(sub_circ, {3, 1}, {3, 1});

  circ.barrier(0);
  circ.measure(qr, cr);

  circ.print();
  std::cout << circ.to_qasm3();

  return 0;
}

