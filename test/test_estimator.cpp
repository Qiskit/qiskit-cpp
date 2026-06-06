// This code is part of Qiskit.
//
// (C) Copyright IBM 2026.
//
// This code is licensed under the Apache License, Version 2.0. You may
// obtain a copy of this license in the LICENSE.txt file in the root directory
// of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.

#include <cmath>
#include <complex>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "common.hpp"

#include "circuit/quantumcircuit.hpp"
#include "primitives/backend_estimator_v2.hpp"
#include "primitives/containers/estimator_result.hpp"
#include "primitives/detail/estimator_measurement.hpp"
#include "quantum_info/sparse_observable.hpp"
using namespace Qiskit;
using namespace Qiskit::circuit;
using namespace Qiskit::primitives;
using namespace Qiskit::primitives::estimator_detail;
using namespace Qiskit::quantum_info;

static SparseObservable observable_from_label(const char* text)
{
    std::string label(text);
    return SparseObservable::from_label(label);
}
template <class Exception, class Function>
static bool raises(Function function)
{
    try {
        function();
    } catch (const Exception&) {
        return true;
    }
    return false;
}
static int test_estimator_pub_and_result_containers(void)
{
    QuantumCircuit circuit(1, 0);
    auto z = observable_from_label("Z");
    EstimatorPub pub(circuit, z, 0.25);
    EstimatorPub assigned;
    assigned = pub;
    if (assigned.circuit().num_qubits() != 1 || assigned.observables().size() != 1
        || assigned.precision() != 0.25) {
        std::cerr << "Unexpected estimator pub contents." << std::endl;
        return EqualityError;
    }
    EstimatorPubResult pub_result(pub, {0.5}, {0.125}, 16, 0.25);
    EstimatorResult result;
    result.push_back(pub_result);
    const EstimatorResult& const_result = result;
    if (const_result.size() != 1 || const_result[0].evs()[0] != 0.5
        || const_result[0].stds()[0] != 0.125 || const_result[0].shots() != 16
        || const_result[0].target_precision() != 0.25) {
        std::cerr << "Unexpected estimator result contents." << std::endl;
        return EqualityError;
    }
    auto zz = observable_from_label("ZZ");
    if (!raises<std::invalid_argument>(
            [&] { EstimatorPub(circuit, z, std::numeric_limits<double>::quiet_NaN()); })
        || !raises<std::invalid_argument>([&] { EstimatorPub(circuit, zz); })) {
        std::cerr << "Estimator pub validation failed." << std::endl;
        return EqualityError;
    }
    return Ok;
}
static int test_precision_and_observable_extraction(void)
{
    if (shots_from_precision(0.015625) != 4096 || shots_from_precision(0.1) != 100
        || std::abs(resolve_precision(0.2, 0.1, 0.015625) - 0.2) > 1e-10
        || std::abs(resolve_precision(0.0, 0.1, 0.015625) - 0.1) > 1e-10) {
        std::cerr << "Unexpected precision handling." << std::endl;
        return EqualityError;
    }
    if (!raises<std::invalid_argument>([] { shots_from_precision(0.0); })) {
        std::cerr << "Zero precision was accepted." << std::endl;
        return EqualityError;
    }
    auto zi = observable_from_label("ZI");
    auto terms = extract_pauli_terms(zi, 0);
    if (terms.size() != 1 || terms[0].factors.size() != 1
        || terms[0].factors[0].axis != PauliAxis::Z || terms[0].factors[0].qubit != 1) {
        std::cerr << "Unexpected ZI observable extraction." << std::endl;
        return EqualityError;
    }
    std::vector<std::complex<double>> coeffs{2.0, -0.5};
    std::vector<QkBitTerm> bit_terms{QkBitTerm_Z};
    reg_t indices{0};
    std::vector<size_t> boundaries{0, 1, 1};
    auto scaled_z_plus_identity = SparseObservable(1, coeffs, bit_terms, indices, boundaries);
    auto extracted = extract_pauli_terms(scaled_z_plus_identity, 0);
    if (extracted.size() != 2 || extracted[0].factors.size() != 1
        || !extracted[1].factors.empty() || extracted[1].coeff.real() != -0.5) {
        std::cerr << "Unexpected multi-term observable extraction." << std::endl;
        return EqualityError;
    }
    auto projector = observable_from_label("+");
    if (!raises<std::invalid_argument>([&] { extract_pauli_terms(projector, 0); })) {
        std::cerr << "Projector observable was accepted." << std::endl;
        return EqualityError;
    }
    return Ok;
}
static int test_measurement_planning(void)
{
    auto zi = observable_from_label("ZI");
    auto compatible_terms = extract_pauli_terms(zi, 0);
    compatible_terms.push_back(PauliTerm{0, 1.0, std::vector<PauliFactor>{PauliFactor{0, PauliAxis::X}}});
    auto compatible_groups = group_pauli_terms(compatible_terms, 2, true);
    if (compatible_groups.size() != 1 || compatible_groups[0].terms.size() != 2
        || !is_group_valid(compatible_groups[0])
        || compatible_groups[0].basis.axes[0] != PauliAxis::X
        || compatible_groups[0].basis.axes[1] != PauliAxis::Z) {
        std::cerr << "Unexpected compatible Pauli grouping." << std::endl;
        return EqualityError;
    }
    std::vector<PauliTerm> split_terms{
        PauliTerm{0, 1.0, std::vector<PauliFactor>{PauliFactor{0, PauliAxis::X}}},
        PauliTerm{0, 1.0, std::vector<PauliFactor>{PauliFactor{0, PauliAxis::Z}}}};
    auto split_groups = group_pauli_terms(split_terms, 1, true);
    if (split_groups.size() != 2 || !is_group_valid(split_groups[0])
        || !is_group_valid(split_groups[1])) {
        std::cerr << "Incompatible Pauli terms were grouped together." << std::endl;
        return EqualityError;
    }
    QuantumCircuit circuit(1, 2);
    circuit.x(0);
    MeasurementBasis y_basis;
    y_basis.axes.push_back(PauliAxis::Y);
    auto measured = make_measurement_circuit(circuit, y_basis);
    if (measured[0].instruction().name() != "x" || measured[1].instruction().name() != "sdg"
        || measured[2].instruction().name() != "h" || measured[3].instruction().name() != "measure"
        || measured[3].clbits()[0] != 0) {
        std::cerr << "Unexpected Y-basis measurement circuit." << std::endl;
        return EqualityError;
    }
    QuantumCircuit premeasured(1, 1);
    premeasured.measure(0, 0);
    if (!raises<std::invalid_argument>([&] { make_measurement_circuit(premeasured, y_basis); })) {
        std::cerr << "Pre-measured circuit was accepted." << std::endl;
        return EqualityError;
    }
    return Ok;
}
static int test_pauli_statistics(void)
{
    BitArray samples;
    samples.allocate(4, 2);
    std::vector<reg_t> bits{reg_t{0, 0}, reg_t{0, 1}, reg_t{0, 1}, reg_t{0, 0}};
    for (uint_t i = 0; i < bits.size(); i++) {
        BitVector shot;
        shot.from_vector(bits[i]);
        samples[i] = shot;
    }
    PauliTerm zi_term{0, 1.0, std::vector<PauliFactor>{PauliFactor{1, PauliAxis::Z}}};
    auto stats = pauli_statistics(samples, zi_term);
    if (std::abs(stats.mean) > 1e-10 || std::abs(stats.variance - 1.0) > 1e-10
        || std::abs(estimator_std_from_numerator(2.0, 4) - 1.0) > 1e-10) {
        std::cerr << "Unexpected Pauli statistics." << std::endl;
        return EqualityError;
    }
    if (!raises<std::runtime_error>([] { estimator_std_from_numerator(1.0, 0); })) {
        std::cerr << "Zero-shot standard error was accepted." << std::endl;
        return EqualityError;
    }
    return Ok;
}
int test_estimator(int argc, char** argv)
{
    int num_failed = 0;
    num_failed += RUN_TEST(test_estimator_pub_and_result_containers);
    num_failed += RUN_TEST(test_precision_and_observable_extraction);
    num_failed += RUN_TEST(test_measurement_planning);
    num_failed += RUN_TEST(test_pauli_statistics);
    std::cerr << "=== Number of failed subtests: " << num_failed << std::endl;
    return num_failed;
}
