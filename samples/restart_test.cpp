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

// Test program for sampler restart

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

using namespace Qiskit;
using namespace Qiskit::circuit;
using namespace Qiskit::providers;
using namespace Qiskit::primitives;
using namespace Qiskit::service;
using namespace Qiskit::compiler;

using Sampler = BackendSamplerV2;

int main(int argc, char** argv)
{
    std::string checkpoint = "checkpoint.qcp";
    if (argc > 1) {
        checkpoint = argv[1];
    }

    auto service = QiskitRuntimeService();
    std::shared_ptr<BackendSamplerJob> job = std::dynamic_pointer_cast<BackendSamplerJob>(service.restart(checkpoint));
    if (job == nullptr)
        return -1;
    std::cout << "job restarted from checkpoint " << checkpoint << std::endl;

    job->pubs()[0].circuit().draw();

    auto result = job->result();

    auto pub_result = result[0];
    auto meas_bits = pub_result.data("meas");
    auto bits = meas_bits.get_bitstrings();
    std::cout << " ===== samples for pub[0] =====" << std::endl;
    for (auto b : bits)
    {
        std::cout << b << ", ";
    }
    std::cout << std::endl;

    std::cout << " ===== counts for pub[0] =====" << std::endl;
    auto count = meas_bits.get_counts();
    for (auto c : count)
    {
        std::cout << c.first << " : " << c.second << std::endl;
    }

    return 0;
}