/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2025.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.
*/

// Test program for SQC

#define _USE_MATH_DEFINES
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <cmath>

#include "circuit/quantumcircuit.hpp"
#include "primitives/backend_sampler_v2.hpp"
//#include "service/qiskit_runtime_service.hpp"
#include "compiler/transpiler.hpp"

using namespace Qiskit;
using namespace Qiskit::circuit;
using namespace Qiskit::providers;
using namespace Qiskit::primitives;
//using namespace Qiskit::service;
using namespace Qiskit::compiler;

using Sampler = BackendSamplerV2;

int main()
{
    int num_qubits = 10;
    auto qreg = QuantumRegister(num_qubits);
    auto creg = ClassicalRegister(num_qubits, std::string("meas"));
    auto ctest = ClassicalRegister(num_qubits, std::string("test"));
    QuantumCircuit circ(std::vector<QuantumRegister>({qreg,}), std::vector<ClassicalRegister>({creg, ctest}));

    // test measure all
    circ.measure(qreg, ctest);

    // GHZ circuit
    circ.h(0);
    for (int i = 0; i < num_qubits - 1; i++)
    {
        circ.cx(i, i + 1);
    }
    circ.measure(qreg, creg);

    //auto service = QiskitRuntimeService();
    //auto backend = service.backend("ibm_torino");
    //auto sampler = Sampler(backend, 100);

    //auto transpiled_circ = transpile(circ, backend);

    //auto job = sampler.run({SamplerPub(transpiled_circ)});
    //if (job == nullptr)
    //    return -1;
    //auto result = job->result();

    //auto pub_result = result[0];
    //auto meas_bits = pub_result.data("meas");
    //auto bits = meas_bits.get_bitstrings();
    //std::cout << " ===== samples for pub[0] =====" << std::endl;
    //for (auto b : bits)
    //{
    //    std::cout << b << ", ";
    //}
    //std::cout << std::endl;

    //std::cout << " --- test bits ---" << std::endl;
    //auto test_bits = pub_result.data("test");
    //bits = test_bits.get_bitstrings();
    //for (auto b : bits)
    //{
    //    std::cout << b << ", ";
    //}
    //std::cout << std::endl;

    //std::cout << " ===== counts for pub[0] =====" << std::endl;
    //auto count = meas_bits.get_counts();
    //for (auto c : count)
    //{
    //    std::cout << c.first << " : " << c.second << std::endl;
    //}


    //auto bitcounts = test_bits.bitcount();
    //reg_t zero_index;
    //zero_index.reserve(bitcounts.size());
    //for (uint_t i = 0; i < bitcounts.size(); i++) {
    //    if (bitcounts[i] == 0) {
    //        zero_index.push_back(i);
    //    }
    //}

    //std::cout << " ===== counts for pub[0] whose test bit are 0 =====" << std::endl;
    //count = meas_bits.get_counts(zero_index);
    //for (auto c : count)
    //{
    //    std::cout << c.first << " : " << c.second << std::endl;
    //}

    return 0;
}
