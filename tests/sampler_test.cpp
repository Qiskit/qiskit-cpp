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
using namespace Qiskit::service;
using namespace Qiskit::compiler;

using Sampler = BackendSamplerV2;

int main()
{
    int num_qubits = 10;
    QuantumCircuit circ(num_qubits, num_qubits);

    // GHZ circuit
    circ.h(0);
    for (int i = 0; i < num_qubits - 1; i++)
    {
        circ.cx(i, i + 1);
    }
    for (int i = 0; i < num_qubits; i++)
    {
        circ.measure(i, i);
    }

    // set $HONE/.qiskit/qiskit-ibm.json
    // by using Qiskit IBM Runtime
    // see https://github.com/Qiskit/qiskit-ibm-runtime?tab=readme-ov-file#save-your-account-on-disk
    //
    // or set 2 environment variables before executing
    // QISKIT_IBM_TOKEN = "your API key"
    // QISKIT_IBM_INSTANCE = "your CRN"
    auto service = QiskitRuntimeService();
    auto backend = service.backend("ibm_fez");
    auto sampler = Sampler(backend, 100);

    auto transpiled_circ = transpile(circ, backend);

    auto job = sampler.run({SamplerPub(transpiled_circ)});
    if (job == nullptr)
        return -1;
    auto result = job->result();

    auto pub_result = result[0];
    auto hex = pub_result.data().get_hexstring();
    std::cout << " ===== samples for pub[0] =====" << std::endl;
    for (auto h : hex)
    {
        std::cout << h << ", ";
    }
    std::cout << std::endl;

    std::cout << " ===== counts for pub[0] =====" << std::endl;
    auto count = pub_result.data().get_counts();
    for (auto c : count)
    {
        std::cout << c.first << " : " << c.second << std::endl;
    }

    return 0;
}
