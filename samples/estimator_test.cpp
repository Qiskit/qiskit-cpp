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

// Test program for estimator via QRMI Runtime V2

#include <iostream>
#include <cstdint>

#include "circuit/quantumcircuit.hpp"
#include "compiler/transpiler.hpp"
#include "primitives/backend_estimator_v2.hpp"
#include "quantum_info/sparse_observable.hpp"
#include "service/qiskit_runtime_service_qrmi.hpp"

using namespace Qiskit;
using namespace Qiskit::circuit;
using namespace Qiskit::primitives;
using namespace Qiskit::service;
using namespace Qiskit::compiler;
using namespace Qiskit::quantum_info;

using Estimator = BackendEstimatorV2;

int main()
{
    int num_qubits = 2;
    QuantumCircuit circ(num_qubits, 0);

    circ.h(0);
    circ.cx(0, 1);

    auto service = QiskitRuntimeService();
    auto backend = service.backend("ibm_torino");
    auto estimator = Estimator(backend, 0.02);

    auto transpiled_circ = transpile(circ, backend);

    std::vector<std::pair<std::string, std::complex<double>>> terms;
    terms.push_back(std::make_pair(std::string("ZZ"), std::complex<double>(1.0)));
    terms.push_back(std::make_pair(std::string("XX"), std::complex<double>(0.5)));
    auto observable = SparseObservable::from_list(terms, num_qubits);
    observable = observable.apply_layout(transpiled_circ.get_qubit_map());

    auto job = estimator.run({EstimatorPub(transpiled_circ, observable, 0.02)});
    if (job == nullptr) {
        return -1;
    }

    auto result = job->result();
    auto pub_result = result[0];

    std::cout << "===== estimator result for pub[0] =====" << std::endl;
    if (!pub_result.evs().empty()) {
        std::cout << "evs: " << pub_result.evs()[0] << std::endl;
    }
    if (!pub_result.stds().empty()) {
        std::cout << "stds: " << pub_result.stds()[0] << std::endl;
    }
    if (pub_result.metadata().contains("shots")) {
        std::cout << "Shots: " << pub_result.metadata()["shots"] << std::endl;
    }
    if (pub_result.metadata().contains("target_precision")) {
        std::cout << "Target precision: " << pub_result.metadata()["target_precision"] << std::endl;
    }

    return 0;
}
