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

// Test program for estimator

#include <iostream>
#include <cstdint>

#include "circuit/quantumcircuit.hpp"
#include "primitives/backend_estimator_v2.hpp"
#ifdef QRMI_ROOT
#include "service/qiskit_runtime_service_qrmi.hpp"
#else
#include "service/qiskit_runtime_service_c.hpp"
#endif

#include "compiler/transpiler.hpp"

using namespace Qiskit;
using namespace Qiskit::circuit;
using namespace Qiskit::primitives;
using namespace Qiskit::service;
using namespace Qiskit::compiler;

using Estimator = BackendEstimatorV2;

int main()
{
    int num_qubits = 2;
    auto qreg = QuantumRegister(num_qubits);
    auto creg = ClassicalRegister(num_qubits, std::string("meas"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qreg}),
                        std::vector<ClassicalRegister>({creg}));

    circ.h(0);
    circ.cx(0, 1);
    circ.measure(qreg, creg);

    auto service = QiskitRuntimeService();
    auto backend = service.backend("ibm_torino");
    auto estimator = Estimator(backend, 1000);

    auto transpiled_circ = transpile(circ, backend);

    std::vector<std::pair<std::string, double>> observables = {
        {"ZZ", 1.0},
        {"XI", 0.5},
    };

    auto job = estimator.run({EstimatorPub(transpiled_circ, observables, 1000)});
    if (job == nullptr) {
        return -1;
    }

    auto result = job->result();
    auto pub_result = result[0];
    std::cout << "EV = " << pub_result.ev()
              << " +/- " << pub_result.stddev() << std::endl;

    return 0;
}
