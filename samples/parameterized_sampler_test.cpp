/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2024. 2026.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.
*/

// Test sampler of parameterized circuit of Qiskit C++

#define _USE_MATH_DEFINES
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <cmath>

#include "circuit/quantumcircuit.hpp"
#include "primitives/backend_sampler_v2.hpp"
#ifdef QRMI_ROOT
#include "service/qiskit_runtime_service_qrmi.hpp"
#else
#include "service/qiskit_runtime_service_c.hpp"
#endif

#include "compiler/transpiler.hpp"

using namespace Qiskit;
using namespace Qiskit::circuit;
using namespace Qiskit::providers;
using namespace Qiskit::primitives;
using namespace Qiskit::service;
using namespace Qiskit::compiler;

using Sampler = BackendSamplerV2;

int main(int argc, char** argv)
{
    std::string backend_name = "ibm_torino";
    if (argc > 1) {
        backend_name = argv[1];
    }

    QuantumRegister qr(2);
    ClassicalRegister cr(2, std::string("meas"));
    QuantumCircuit circ(qr, cr);

    auto a = Parameter("a");
    auto b = Parameter("b");

    circ.h(0);
    circ.cx(0, 1);
    circ.rx(a, 0);
    circ.ry(b, 1);

    circ.measure(qr, cr);

    circ.draw();

    auto service = QiskitRuntimeService();
    auto backend = service.backend(backend_name);
    auto sampler = Sampler(backend, 100);

    auto transpiled_circ = transpile(circ, backend);

    transpiled_circ.draw();

    // Each inner list binds every parameter of the circuit once, so the pub
    // runs the circuit once per list. Values are positional, in the order
    // circ.parameter_symbols() reports, which is alphabetical: a then b.
    std::vector<std::vector<double>> params({{0.5, 0.6}, {0.3, 0.9}});
    auto job = sampler.run({SamplerPub(transpiled_circ, params)});
    if (job == nullptr)
        return -1;
    auto result = job->result();

    auto pub_result = result[0];

    // Index the result to reach one parameter value set at a time, as in
    // Qiskit Python.
    std::cout << " ===== " << pub_result.size() << " parameter value set(s) ====="
              << std::endl;
    for (uint_t set = 0; set < pub_result.size(); set++)
    {
        auto bits = pub_result[set].data("meas").get_bitstrings();
        std::cout << " ----- samples for parameter set " << set << " -----" << std::endl;
        for (auto b : bits)
        {
            std::cout << b << ", ";
        }
        std::cout << std::endl;

        std::cout << " counts : ";
        for (auto c : pub_result[set].data("meas").get_counts())
        {
            std::cout << c.first << "=" << c.second << " ";
        }
        std::cout << std::endl;
    }

    // Counts unioned over every value set, matching Qiskit's get_counts() with
    // no location argument.
    std::cout << " ===== counts over all parameter sets =====" << std::endl;
    for (auto c : pub_result.get_counts("meas"))
    {
        std::cout << c.first << " : " << c.second << std::endl;
    }

    return 0;
}

