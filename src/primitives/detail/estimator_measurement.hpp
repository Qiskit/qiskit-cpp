/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2026.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.
*/

// estimator measurement helpers

#ifndef __qiskitcpp_primitives_detail_estimator_measurement_hpp__
#define __qiskitcpp_primitives_detail_estimator_measurement_hpp__

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>
#include <stdexcept>
#include <vector>

#include "circuit/quantumcircuit.hpp"
#include "primitives/containers/bit_array.hpp"
#include "quantum_info/sparse_observable.hpp"

namespace Qiskit {
namespace primitives {
namespace estimator_detail {

constexpr double coefficient_imag_tolerance = 1e-12;

enum class PauliAxis {
    I,
    X,
    Y,
    Z
};

struct PauliFactor {
    uint_t qubit;
    PauliAxis axis;
};

struct PauliTerm {
    uint_t observable_index;
    std::complex<double> coeff;
    std::vector<PauliFactor> factors;
};

struct MeasurementBasis {
    std::vector<PauliAxis> axes;
};

struct MeasurementGroup {
    MeasurementBasis basis;
    std::vector<PauliTerm> terms;
};

struct MeasurementTask {
    uint_t pub_index;
    uint_t group_index;
    uint_t shots;
    circuit::QuantumCircuit measured_circuit;
    MeasurementGroup group;
};

struct PauliStats {
    double mean;
    double variance;
};

inline double resolve_precision(double pub_precision,
                                double run_precision,
                                double default_precision)
{
    if (pub_precision > 0.0)
        return pub_precision;
    if (run_precision > 0.0)
        return run_precision;
    return default_precision;
}

inline uint_t shots_from_precision(double precision)
{
    if (!std::isfinite(precision) || precision <= 0.0) {
        throw std::invalid_argument("BackendEstimatorV2: precision must be greater than 0");
    }
    long double p = static_cast<long double>(precision);
    long double shots = std::ceil(1.0L / (p * p));
    if (!std::isfinite(shots) || shots > static_cast<long double>(std::numeric_limits<uint_t>::max())) {
        throw std::overflow_error("BackendEstimatorV2: precision maps to shots larger than uint_t");
    }
    if (shots < 1.0L)
        shots = 1.0L;
    return static_cast<uint_t>(shots);
}

inline PauliAxis axis_from_qk_bit_term(QkBitTerm term)
{
    switch (term) {
    case QkBitTerm_X:
        return PauliAxis::X;
    case QkBitTerm_Y:
        return PauliAxis::Y;
    case QkBitTerm_Z:
        return PauliAxis::Z;
    default:
        throw std::invalid_argument("Estimator observable: unsupported non-Pauli term");
    }
}

inline std::vector<PauliTerm> extract_pauli_terms(const quantum_info::SparseObservable& observable,
                                                  uint_t observable_index)
{
    uint_t num_terms = observable.num_terms();
    auto coeffs = observable.coeffs();
    auto bit_terms = observable.bit_terms();
    auto indices = observable.indices();
    auto boundaries = observable.boundaries();

    if (coeffs.size() != num_terms) {
        throw std::runtime_error("Estimator observable: coeffs size mismatch");
    }
    if (boundaries.size() != num_terms + 1) {
        throw std::runtime_error("Estimator observable: boundaries size mismatch");
    }
    if (bit_terms.size() != indices.size()) {
        throw std::runtime_error("Estimator observable: bit_terms/indices size mismatch");
    }
    if (boundaries.empty() || boundaries[0] != 0 || boundaries.back() != bit_terms.size()) {
        throw std::runtime_error("Estimator observable: invalid boundaries");
    }
    for (uint_t boundary_index = 1; boundary_index < boundaries.size(); boundary_index++) {
        if (boundaries[boundary_index] < boundaries[boundary_index - 1]
            || boundaries[boundary_index] > bit_terms.size()) {
            throw std::invalid_argument("Estimator observable: invalid boundaries");
        }
    }

    std::vector<PauliTerm> terms;
    for (uint_t term_index = 0; term_index < num_terms; term_index++) {
        if (std::fabs(coeffs[term_index].imag()) > coefficient_imag_tolerance) {
            throw std::invalid_argument("Estimator observable: imaginary coefficient exceeds tolerance");
        }
        PauliTerm term;
        term.observable_index = observable_index;
        term.coeff = coeffs[term_index];
        uint_t begin = boundaries[term_index];
        uint_t end = boundaries[term_index + 1];
        uint_t previous = 0;
        for (uint_t pos = begin; pos < end; pos++) {
            if (pos > begin && indices[pos] <= previous) {
                throw std::runtime_error("Estimator observable: term indices are not strictly increasing");
            }
            if (indices[pos] >= observable.num_qubits()) {
                throw std::runtime_error("Estimator observable: term index exceeds observable width");
            }
            previous = indices[pos];
            PauliFactor factor;
            factor.qubit = indices[pos];
            factor.axis = axis_from_qk_bit_term(bit_terms[pos]);
            term.factors.push_back(factor);
        }
        terms.push_back(term);
    }
    return terms;
}

inline bool axes_compatible(PauliAxis left, PauliAxis right)
{
    return left == PauliAxis::I || right == PauliAxis::I || left == right;
}

inline MeasurementBasis term_basis(const PauliTerm& term, uint_t num_qubits)
{
    MeasurementBasis basis;
    basis.axes.resize(num_qubits, PauliAxis::I);
    for (uint_t i = 0; i < term.factors.size(); i++) {
        basis.axes[term.factors[i].qubit] = term.factors[i].axis;
    }
    return basis;
}

inline bool term_compatible_with_basis(const PauliTerm& term, const MeasurementBasis& basis)
{
    for (uint_t i = 0; i < term.factors.size(); i++) {
        const PauliFactor& factor = term.factors[i];
        if (factor.qubit >= basis.axes.size())
            return false;
        if (!axes_compatible(factor.axis, basis.axes[factor.qubit]))
            return false;
    }
    return true;
}

inline bool terms_compatible(const PauliTerm& left,
                             const PauliTerm& right,
                             uint_t num_qubits)
{
    auto basis = term_basis(left, num_qubits);
    return term_compatible_with_basis(right, basis);
}

inline void merge_term_into_basis(const PauliTerm& term, MeasurementBasis& basis)
{
    for (uint_t i = 0; i < term.factors.size(); i++) {
        const PauliFactor& factor = term.factors[i];
        if (basis.axes[factor.qubit] == PauliAxis::I) {
            basis.axes[factor.qubit] = factor.axis;
        }
    }
}

inline bool is_group_valid(const MeasurementGroup& group)
{
    for (uint_t i = 0; i < group.terms.size(); i++) {
        if (!term_compatible_with_basis(group.terms[i], group.basis))
            return false;
    }
    return true;
}

struct TermOrder {
    uint_t index;
    uint_t degree;
};

inline bool term_order_less(const TermOrder& left, const TermOrder& right)
{
    if (left.degree != right.degree)
        return left.degree > right.degree;
    return left.index < right.index;
}

inline std::vector<MeasurementGroup> group_pauli_terms(const std::vector<PauliTerm>& terms,
                                                       uint_t num_qubits,
                                                       bool abelian_grouping)
{
    std::vector<PauliTerm> non_identity_terms;
    for (uint_t i = 0; i < terms.size(); i++) {
        if (!terms[i].factors.empty()) {
            non_identity_terms.push_back(terms[i]);
        }
    }

    if (!abelian_grouping) {
        std::vector<MeasurementGroup> groups;
        for (uint_t i = 0; i < non_identity_terms.size(); i++) {
            MeasurementGroup group;
            group.basis = term_basis(non_identity_terms[i], num_qubits);
            group.terms.push_back(non_identity_terms[i]);
            groups.push_back(group);
        }
        return groups;
    }

    std::vector<TermOrder> order(non_identity_terms.size());
    for (uint_t i = 0; i < non_identity_terms.size(); i++) {
        order[i].index = i;
        order[i].degree = 0;
    }
    for (uint_t i = 0; i < non_identity_terms.size(); i++) {
        for (uint_t j = i + 1; j < non_identity_terms.size(); j++) {
            if (!terms_compatible(non_identity_terms[i], non_identity_terms[j], num_qubits)) {
                order[i].degree++;
                order[j].degree++;
            }
        }
    }
    std::sort(order.begin(), order.end(), term_order_less);

    std::vector<MeasurementGroup> groups;
    for (uint_t i = 0; i < order.size(); i++) {
        const PauliTerm& term = non_identity_terms[order[i].index];
        bool inserted = false;
        for (uint_t group_index = 0; group_index < groups.size(); group_index++) {
            if (term_compatible_with_basis(term, groups[group_index].basis)) {
                groups[group_index].terms.push_back(term);
                merge_term_into_basis(term, groups[group_index].basis);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            MeasurementGroup group;
            group.basis = term_basis(term, num_qubits);
            group.terms.push_back(term);
            groups.push_back(group);
        }
    }
    return groups;
}

inline circuit::QuantumCircuit make_measurement_circuit(const circuit::QuantumCircuit& circuit,
                                                        const MeasurementBasis& basis)
{
    if (!circuit.get_measure_map().empty()) {
        throw std::invalid_argument("Estimator measurement: input circuit already contains measurements");
    }
    if (basis.axes.size() != circuit.num_qubits()) {
        throw std::invalid_argument("Estimator measurement: basis/circuit width mismatch");
    }

    uint_t num_clbits = std::max(circuit.num_clbits(), circuit.num_qubits());
    circuit::QuantumCircuit measured(circuit.num_qubits(), num_clbits);
    circuit::QuantumCircuit source = circuit.copy();
    measured.compose(source);

    for (uint_t qubit = 0; qubit < basis.axes.size(); qubit++) {
        if (basis.axes[qubit] == PauliAxis::X) {
            measured.h(qubit);
        } else if (basis.axes[qubit] == PauliAxis::Y) {
            measured.sdg(qubit);
            measured.h(qubit);
        }
    }
    for (uint_t qubit = 0; qubit < basis.axes.size(); qubit++) {
        if (basis.axes[qubit] != PauliAxis::I) {
            measured.measure(qubit, qubit);
        }
    }
    return measured;
}

inline double pauli_eigenvalue(const BitVector& bits, const PauliTerm& term)
{
    uint_t parity = 0;
    for (uint_t i = 0; i < term.factors.size(); i++) {
        parity ^= bits[term.factors[i].qubit];
    }
    return parity == 0 ? 1.0 : -1.0;
}

inline PauliStats pauli_statistics(BitArray& bits, const PauliTerm& term)
{
    uint_t shots = bits.num_shots();
    if (shots == 0) {
        throw std::runtime_error("BackendEstimatorJob: empty samples for measurement task");
    }
    double sum = 0.0;
    for (uint_t shot = 0; shot < shots; shot++) {
        sum += pauli_eigenvalue(bits[shot], term);
    }
    PauliStats stats;
    stats.mean = sum / static_cast<double>(shots);
    stats.variance = 1.0 - stats.mean * stats.mean;
    return stats;
}

inline double estimator_std_from_numerator(double numerator, uint_t shots)
{
    if (shots == 0) {
        throw std::runtime_error("BackendEstimatorJob: empty samples for measurement task");
    }
    return numerator / std::sqrt(static_cast<double>(shots));
}

} // namespace estimator_detail
} // namespace primitives
} // namespace Qiskit

#endif //__qiskitcpp_primitives_detail_estimator_measurement_hpp__
