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
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "common.hpp"

#include "quantum_info/sparse_observable.hpp"

using namespace Qiskit;
using namespace Qiskit::quantum_info;

static int test_observable_add_inplace(void) {
    // Adding in place must give the same result as the out-of-place operator,
    // which is what it replaced.
    auto a = SparseObservable::identity(3);
    auto b = SparseObservable::identity(3);

    auto expected = a + b;

    a += b;
    if (a.num_terms() != expected.num_terms()) {
        std::cerr << "  observable_add_inplace test : " << a.num_terms()
            << " terms, expected " << expected.num_terms() << std::endl;
        return EqualityError;
    }
    if (a.to_string() != expected.to_string()) {
        std::cerr << "  observable_add_inplace test : \"" << a.to_string()
            << "\", expected \"" << expected.to_string() << "\"" << std::endl;
        return EqualityError;
    }
    // Adding appends terms rather than combining them, so two identities make
    // two terms.
    if (a.num_terms() != 2) {
        std::cerr << "  observable_add_inplace test : " << a.num_terms()
            << " terms, expected 2" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_add_inplace_does_not_touch_rhs(void) {
    // The right-hand side must come out unchanged: only the left is mutated.
    auto a = SparseObservable::identity(2);
    auto b = SparseObservable::from_label(std::string("XZ"));

    const std::string before = b.to_string();
    const uint_t before_terms = b.num_terms();

    a += b;

    if (b.to_string() != before || b.num_terms() != before_terms) {
        std::cerr << "  observable_add_inplace_does_not_touch_rhs test : the right-hand "
            << "side changed from \"" << before << "\" to \"" << b.to_string() << "\""
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_add_inplace_self(void) {
    // Doubling an observable by adding it to itself. The in-place C-API call
    // takes its left side mutably and its right immutably, so it cannot be
    // handed the same observable twice; this must still work.
    auto a = SparseObservable::from_label(std::string("XY"));

    auto expected = a + a;

    a += a;
    if (a.num_terms() != 2) {
        std::cerr << "  observable_add_inplace_self test : " << a.num_terms()
            << " terms, expected 2" << std::endl;
        return EqualityError;
    }
    if (a.to_string() != expected.to_string()) {
        std::cerr << "  observable_add_inplace_self test : \"" << a.to_string()
            << "\", expected \"" << expected.to_string() << "\"" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_add_inplace_accumulate(void) {
    // Repeated accumulation, which is what the in-place form is for.
    auto total = SparseObservable::zero(2);
    for (uint_t i = 0; i < 5; i++) {
        auto term = SparseObservable::identity(2);
        total += term;
    }
    // zero() contributes no terms, so only the five identities remain.
    if (total.num_terms() != 5) {
        std::cerr << "  observable_add_inplace_accumulate test : " << total.num_terms()
            << " terms, expected 5" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_add_inplace_unset(void) {
    // A default-constructed observable holds no C-API observable, which the
    // in-place call would reject, so those cases are handled before the call.
    SparseObservable unset;
    auto identity = SparseObservable::identity(2);

    // Adding to an unset observable adopts a copy of the right-hand side.
    SparseObservable target;
    target += identity;
    if (target.num_terms() != identity.num_terms() || target.num_qubits() != 2) {
        std::cerr << "  observable_add_inplace_unset test : adding to an unset observable "
            << "did not adopt the right-hand side" << std::endl;
        return EqualityError;
    }
    // ... and it must be a copy, not a share: mutating it must not affect the
    // original.
    target += identity;
    if (identity.num_terms() != 1) {
        std::cerr << "  observable_add_inplace_unset test : the right-hand side was "
            << "shared rather than copied" << std::endl;
        return EqualityError;
    }

    // Adding an unset observable leaves the left side alone.
    auto keep = SparseObservable::identity(2);
    keep += unset;
    if (keep.num_terms() != 1 || keep.num_qubits() != 2) {
        std::cerr << "  observable_add_inplace_unset test : adding an unset observable "
            << "changed the left-hand side" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_multiply_inplace(void) {
    // Scaling in place must match the out-of-place operator.
    auto a = SparseObservable::from_label(std::string("XY"));
    auto b = a;

    std::complex<double> factor(2.0, 0.0);
    auto expected = a * factor;

    b *= factor;
    if (b.to_string() != expected.to_string()) {
        std::cerr << "  observable_multiply_inplace test : \"" << b.to_string()
            << "\", expected \"" << expected.to_string() << "\"" << std::endl;
        return EqualityError;
    }

    // The coefficient must actually have been applied.
    const auto coeffs = b.coeffs();
    if (coeffs.size() != 1 || coeffs[0] != std::complex<double>(2.0, 0.0)) {
        std::cerr << "  observable_multiply_inplace test : the coefficient was not scaled"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_multiply_inplace_complex(void) {
    // A complex factor must scale both parts, and scaling twice must compose.
    auto a = SparseObservable::identity(1);

    std::complex<double> i(0.0, 1.0);
    a *= i;
    const auto once = a.coeffs();
    if (once.size() != 1 || once[0] != std::complex<double>(0.0, 1.0)) {
        std::cerr << "  observable_multiply_inplace_complex test : one scaling gave "
            << once[0].real() << "+" << once[0].imag() << "i, expected 0+1i" << std::endl;
        return EqualityError;
    }

    a *= i;
    const auto twice = a.coeffs();
    if (twice.size() != 1 || twice[0] != std::complex<double>(-1.0, 0.0)) {
        std::cerr << "  observable_multiply_inplace_complex test : two scalings gave "
            << twice[0].real() << "+" << twice[0].imag() << "i, expected -1+0i" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_multiply_inplace_unset(void) {
    // Scaling an observable that holds nothing has nothing to do, and must not
    // reach the C-API with a null pointer.
    SparseObservable unset;
    std::complex<double> factor(2.0, 0.0);
    unset *= factor;

    if (unset.num_terms() != 0 || unset.num_qubits() != 0) {
        std::cerr << "  observable_multiply_inplace_unset test : scaling an unset "
            << "observable changed it" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_inplace_accepts_const(void) {
    // Both in-place operators take their right-hand side by const reference, so
    // a const observable and a literal coefficient are usable.
    const auto addend = SparseObservable::identity(2);
    auto target = SparseObservable::identity(2);
    target += addend;
    if (target.num_terms() != 2) {
        std::cerr << "  observable_inplace_accepts_const test : adding a const observable "
            << "gave " << target.num_terms() << " terms, expected 2" << std::endl;
        return EqualityError;
    }

    const std::complex<double> factor(3.0, 0.0);
    target *= factor;
    const auto coeffs = target.coeffs();
    if (coeffs.size() != 2 || coeffs[0] != std::complex<double>(3.0, 0.0)) {
        std::cerr << "  observable_inplace_accepts_const test : scaling by a const "
            << "coefficient did not apply" << std::endl;
        return EqualityError;
    }
    return Ok;
}

// ===========================================================================
// Term, operator[] and from_terms
// ===========================================================================

static int test_term_accessors(void) {
    std::vector<QkBitTerm> bits;
    bits.push_back(QkBitTerm_Z);
    bits.push_back(QkBitTerm_X);
    reg_t indices;
    indices.push_back(0);
    indices.push_back(2);

    Term term(3, std::complex<double>(1.5, -0.5), bits, indices);

    if (term.num_qubits() != 3 || term.len() != 2) {
        std::cerr << "  term_accessors test : " << term.num_qubits() << " qubits, "
            << term.len() << " operators, expected 3 and 2" << std::endl;
        return EqualityError;
    }
    if (term.coeff() != std::complex<double>(1.5, -0.5)) {
        std::cerr << "  term_accessors test : the coefficient did not survive" << std::endl;
        return EqualityError;
    }
    if (term.bit_terms() != bits || term.indices() != indices) {
        std::cerr << "  term_accessors test : the operators or indices did not survive"
            << std::endl;
        return EqualityError;
    }
    // bit_labels follows the index order, so Z on qubit 0 comes first.
    if (term.bit_labels() != "ZX") {
        std::cerr << "  term_accessors test : labels are \"" << term.bit_labels()
            << "\", expected \"ZX\"" << std::endl;
        return EqualityError;
    }
    // A dense label puts qubit 0 rightmost, so X on qubit 2 is leftmost.
    if (term.to_label() != "X_Z") {
        std::cerr << "  term_accessors test : dense label is \"" << term.to_label()
            << "\", expected \"X_Z\"" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_term_rejects_bad_input(void) {
    std::vector<QkBitTerm> bits;
    bits.push_back(QkBitTerm_X);

    // Two operators but one index.
    std::vector<QkBitTerm> two;
    two.push_back(QkBitTerm_X);
    two.push_back(QkBitTerm_Y);
    reg_t one;
    one.push_back(0);

    bool threw = false;
    try {
        Term bad(2, std::complex<double>(1.0, 0.0), two, one);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  term_rejects_bad_input test : mismatched lengths were accepted"
            << std::endl;
        return EqualityError;
    }

    // An index at or past num_qubits.
    reg_t out_of_range;
    out_of_range.push_back(2);
    threw = false;
    try {
        Term bad(2, std::complex<double>(1.0, 0.0), bits, out_of_range);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  term_rejects_bad_input test : an out-of-range index was accepted"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_term_identity(void) {
    // A term with no operators is the identity, scaled by its coefficient.
    Term term(4, std::complex<double>(2.0, 0.0), std::vector<QkBitTerm>(), reg_t());

    if (term.len() != 0 || !term.bit_labels().empty()) {
        std::cerr << "  term_identity test : an empty term reported operators" << std::endl;
        return EqualityError;
    }
    if (term.to_label() != "____") {
        std::cerr << "  term_identity test : dense label is \"" << term.to_label()
            << "\", expected \"____\"" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_index_term(void) {
    // Indexing must give back the term that went in.
    auto obs = SparseObservable::from_label(std::string("XYZ"));
    if (obs.num_terms() != 1) {
        std::cerr << "  observable_index_term test : " << obs.num_terms()
            << " terms, expected 1" << std::endl;
        return EqualityError;
    }

    Term term = obs[0];
    if (term.num_qubits() != 3 || term.len() != 3) {
        std::cerr << "  observable_index_term test : " << term.num_qubits() << " qubits, "
            << term.len() << " operators, expected 3 and 3" << std::endl;
        return EqualityError;
    }
    if (term.coeff() != std::complex<double>(1.0, 0.0)) {
        std::cerr << "  observable_index_term test : the coefficient is wrong" << std::endl;
        return EqualityError;
    }
    // from_label reads the rightmost character as qubit 0, so "XYZ" is
    // Z on qubit 0, Y on 1, X on 2 -- and indices come out sorted.
    const reg_t expected_indices({0, 1, 2});
    if (term.indices() != expected_indices) {
        std::cerr << "  observable_index_term test : indices are not sorted 0,1,2"
            << std::endl;
        return EqualityError;
    }
    if (term.bit_labels() != "ZYX") {
        std::cerr << "  observable_index_term test : labels are \"" << term.bit_labels()
            << "\", expected \"ZYX\"" << std::endl;
        return EqualityError;
    }
    // The dense label must round-trip back to what was parsed.
    if (term.to_label() != "XYZ") {
        std::cerr << "  observable_index_term test : dense label is \"" << term.to_label()
            << "\", expected \"XYZ\"" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_index_out_of_range(void) {
    auto obs = SparseObservable::identity(2);

    // The C-API's own bounds check uses > where it needs >=, so index ==
    // num_terms would read past the end of its arrays. The wrapper must catch
    // it first.
    bool threw = false;
    try {
        obs[obs.num_terms()];
    } catch (const std::out_of_range &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  observable_index_out_of_range test : index == num_terms was "
            << "accessible" << std::endl;
        return EqualityError;
    }

    threw = false;
    try {
        obs[100];
    } catch (const std::out_of_range &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  observable_index_out_of_range test : a far index was accessible"
            << std::endl;
        return EqualityError;
    }

    // An observable holding nothing has no terms to reach.
    SparseObservable unset;
    threw = false;
    try {
        unset[0];
    } catch (const std::out_of_range &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  observable_index_out_of_range test : an unset observable was "
            << "indexable" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_index_multi_term(void) {
    // Each term must come back separately, with the right coefficient.
    std::vector<std::pair<std::string, std::complex<double>>> list;
    list.push_back(std::make_pair(std::string("XI"), std::complex<double>(1.0, 0.0)));
    list.push_back(std::make_pair(std::string("IZ"), std::complex<double>(-2.0, 0.0)));
    auto obs = SparseObservable::from_list(list);

    if (obs.num_terms() != 2) {
        std::cerr << "  observable_index_multi_term test : " << obs.num_terms()
            << " terms, expected 2" << std::endl;
        return EqualityError;
    }

    Term first = obs[0];
    Term second = obs[1];
    if (first.coeff() != std::complex<double>(1.0, 0.0)
        || second.coeff() != std::complex<double>(-2.0, 0.0)) {
        std::cerr << "  observable_index_multi_term test : the coefficients are wrong"
            << std::endl;
        return EqualityError;
    }
    // "XI" is X on qubit 1; "IZ" is Z on qubit 0.
    if (first.len() != 1 || first.indices()[0] != 1 || first.bit_labels() != "X") {
        std::cerr << "  observable_index_multi_term test : term 0 is \""
            << first.to_string() << "\", expected X on qubit 1" << std::endl;
        return EqualityError;
    }
    if (second.len() != 1 || second.indices()[0] != 0 || second.bit_labels() != "Z") {
        std::cerr << "  observable_index_multi_term test : term 1 is \""
            << second.to_string() << "\", expected Z on qubit 0" << std::endl;
        return EqualityError;
    }

    // terms() must agree with indexing one at a time.
    const auto all = obs.terms();
    if (all.size() != 2 || all[0] != first || all[1] != second) {
        std::cerr << "  observable_index_multi_term test : terms() disagrees with "
            << "operator[]" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_from_terms(void) {
    // Building from terms, then reading them back, must round-trip.
    std::vector<QkBitTerm> first_bits;
    first_bits.push_back(QkBitTerm_X);
    reg_t first_indices;
    first_indices.push_back(1);

    std::vector<QkBitTerm> second_bits;
    second_bits.push_back(QkBitTerm_Z);
    reg_t second_indices;
    second_indices.push_back(0);

    std::vector<Term> terms;
    terms.push_back(Term(2, std::complex<double>(1.0, 0.0), first_bits, first_indices));
    terms.push_back(Term(2, std::complex<double>(0.0, 1.0), second_bits, second_indices));

    auto obs = SparseObservable::from_terms(terms);
    if (obs.num_qubits() != 2 || obs.num_terms() != 2) {
        std::cerr << "  observable_from_terms test : " << obs.num_qubits() << " qubits, "
            << obs.num_terms() << " terms, expected 2 and 2" << std::endl;
        return EqualityError;
    }
    const auto read_back = obs.terms();
    if (read_back.size() != 2 || read_back[0] != terms[0] || read_back[1] != terms[1]) {
        std::cerr << "  observable_from_terms test : the terms did not round-trip"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_from_terms_empty(void) {
    // An empty list needs the qubit count given explicitly, as in Qiskit.
    auto obs = SparseObservable::from_terms(std::vector<Term>(), 5);
    if (obs.num_qubits() != 5 || obs.num_terms() != 0) {
        std::cerr << "  observable_from_terms_empty test : " << obs.num_qubits()
            << " qubits, " << obs.num_terms() << " terms, expected 5 and 0" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_from_terms_rejects_mismatch(void) {
    // All terms must agree on the qubit count.
    std::vector<QkBitTerm> bits;
    bits.push_back(QkBitTerm_X);
    reg_t indices;
    indices.push_back(0);

    std::vector<Term> terms;
    terms.push_back(Term(2, std::complex<double>(1.0, 0.0), bits, indices));
    terms.push_back(Term(3, std::complex<double>(1.0, 0.0), bits, indices));

    bool threw = false;
    try {
        SparseObservable::from_terms(terms);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  observable_from_terms_rejects_mismatch test : terms on different "
            << "qubit counts were accepted" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_from_terms_matches_from_label(void) {
    // Assembling a term by hand must give the same observable as parsing the
    // equivalent dense label.
    auto expected = SparseObservable::from_label(std::string("ZX"));

    std::vector<QkBitTerm> bits;
    bits.push_back(QkBitTerm_X);
    bits.push_back(QkBitTerm_Z);
    reg_t indices;
    indices.push_back(0);
    indices.push_back(1);

    std::vector<Term> terms;
    terms.push_back(Term(2, std::complex<double>(1.0, 0.0), bits, indices));
    auto built = SparseObservable::from_terms(terms);

    if (built.to_string() != expected.to_string()) {
        std::cerr << "  observable_from_terms_matches_from_label test : \""
            << built.to_string() << "\", expected \"" << expected.to_string() << "\""
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_term_to_observable(void) {
    std::vector<QkBitTerm> bits;
    bits.push_back(QkBitTerm_Y);
    reg_t indices;
    indices.push_back(1);

    Term term(3, std::complex<double>(2.0, 0.0), bits, indices);
    auto obs = term.to_observable();

    if (obs.num_qubits() != 3 || obs.num_terms() != 1) {
        std::cerr << "  term_to_observable test : " << obs.num_qubits() << " qubits, "
            << obs.num_terms() << " terms, expected 3 and 1" << std::endl;
        return EqualityError;
    }
    if (obs[0] != term) {
        std::cerr << "  term_to_observable test : the term did not round-trip" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_add_term(void) {
    // Appending terms one at a time, which is what from_terms uses.
    auto obs = SparseObservable::zero(2);

    std::vector<QkBitTerm> bits;
    bits.push_back(QkBitTerm_X);
    reg_t indices;
    indices.push_back(0);
    Term term(2, std::complex<double>(1.0, 0.0), bits, indices);

    if (obs.add_term(term) != QkExitCode_Success) {
        std::cerr << "  observable_add_term test : adding a matching term failed"
            << std::endl;
        return RuntimeError;
    }
    if (obs.num_terms() != 1 || obs[0] != term) {
        std::cerr << "  observable_add_term test : the term was not appended" << std::endl;
        return EqualityError;
    }

    // A term on a different number of qubits is reported, not accepted.
    Term wrong(3, std::complex<double>(1.0, 0.0), bits, indices);
    if (obs.add_term(wrong) == QkExitCode_Success) {
        std::cerr << "  observable_add_term test : a term on 3 qubits was added to a "
            << "2-qubit observable" << std::endl;
        return EqualityError;
    }
    if (obs.num_terms() != 1) {
        std::cerr << "  observable_add_term test : a rejected term changed the observable"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_bit_term_labels(void) {
    // Every single-qubit term must report the label Qiskit uses for it.
    const QkBitTerm terms[] = {QkBitTerm_X, QkBitTerm_Y, QkBitTerm_Z,
                               QkBitTerm_Plus, QkBitTerm_Minus,
                               QkBitTerm_Left, QkBitTerm_Right,
                               QkBitTerm_Zero, QkBitTerm_One};
    const char expected[] = {'X', 'Y', 'Z', '+', '-', 'l', 'r', '0', '1'};

    for (uint_t i = 0; i < 9; i++) {
        const char label = bit_term_label(terms[i]);
        if (label != expected[i]) {
            std::cerr << "  bit_term_labels test : term " << i << " labelled '" << label
                << "', expected '" << expected[i] << "'" << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

// ===========================================================================
// compose with a qubit map
// ===========================================================================

static int test_compose_map_identity_order(void) {
    // Mapping each qubit to itself must match the plain compose, which
    // requires equal qubit counts.
    auto a = SparseObservable::from_label(std::string("XZ"));
    auto b = SparseObservable::from_label(std::string("ZX"));

    auto expected = a.compose(b);

    reg_t qargs;
    qargs.push_back(0);
    qargs.push_back(1);
    auto mapped = a.compose(b, qargs);

    if (mapped.num_qubits() != 2) {
        std::cerr << "  compose_map_identity_order test : " << mapped.num_qubits()
            << " qubits, expected 2" << std::endl;
        return EqualityError;
    }
    if (mapped.to_string() != expected.to_string()) {
        std::cerr << "  compose_map_identity_order test : \"" << mapped.to_string()
            << "\", expected \"" << expected.to_string() << "\"" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_compose_map_smaller_operand(void) {
    // The point of the qubit map: compose a small observable into a larger one.
    auto big = SparseObservable::identity(5);
    auto small = SparseObservable::from_label(std::string("Z"));

    // The single qubit of `small` acts on qubit 3 of `big`.
    reg_t qargs;
    qargs.push_back(3);
    auto composed = big.compose(small, qargs);

    if (composed.num_qubits() != 5) {
        std::cerr << "  compose_map_smaller_operand test : " << composed.num_qubits()
            << " qubits, expected 5" << std::endl;
        return EqualityError;
    }
    if (composed.num_terms() != 1) {
        std::cerr << "  compose_map_smaller_operand test : " << composed.num_terms()
            << " terms, expected 1" << std::endl;
        return EqualityError;
    }

    // Composing with the identity leaves just the mapped Z, on qubit 3.
    Term term = composed[0];
    if (term.len() != 1 || term.indices()[0] != 3 || term.bit_labels() != "Z") {
        std::cerr << "  compose_map_smaller_operand test : term is \"" << term.to_string()
            << "\", expected Z on qubit 3" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_compose_map_permutes(void) {
    // A map may reorder qubits, not just place them.
    auto big = SparseObservable::identity(3);
    auto small = SparseObservable::from_label(std::string("ZX"));

    // from_label("ZX") is X on qubit 0 and Z on qubit 1 of `small`. Send
    // small's qubit 0 to big's qubit 2, and small's qubit 1 to big's qubit 0.
    reg_t qargs;
    qargs.push_back(2);
    qargs.push_back(0);
    auto composed = big.compose(small, qargs);

    if (composed.num_terms() != 1) {
        std::cerr << "  compose_map_permutes test : " << composed.num_terms()
            << " terms, expected 1" << std::endl;
        return EqualityError;
    }

    Term term = composed[0];
    // Indices come back sorted, so Z on qubit 0 precedes X on qubit 2.
    const reg_t expected_indices({0, 2});
    if (term.indices() != expected_indices || term.bit_labels() != "ZX") {
        std::cerr << "  compose_map_permutes test : term is \"" << term.to_string()
            << "\", expected Z on qubit 0 and X on qubit 2" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_compose_map_rejects_larger_operand(void) {
    // The C-API panics when the operand is the larger observable, so this has
    // to be caught first.
    auto small = SparseObservable::identity(2);
    auto big = SparseObservable::identity(4);

    reg_t qargs;
    for (uint_t i = 0; i < 4; i++) {
        qargs.push_back(i);
    }

    bool threw = false;
    try {
        small.compose(big, qargs);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  compose_map_rejects_larger_operand test : composing a 4-qubit "
            << "observable into a 2-qubit one was accepted" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_compose_map_rejects_wrong_length(void) {
    // The C-API reads exactly one entry per qubit of the operand, so a short
    // list would read past the end of the array.
    auto big = SparseObservable::identity(4);
    auto small = SparseObservable::from_label(std::string("ZX"));

    // Too few: the operand has 2 qubits.
    reg_t too_few;
    too_few.push_back(0);
    bool threw = false;
    try {
        big.compose(small, too_few);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  compose_map_rejects_wrong_length test : a short qubit map was "
            << "accepted" << std::endl;
        return EqualityError;
    }

    // Too many.
    reg_t too_many;
    too_many.push_back(0);
    too_many.push_back(1);
    too_many.push_back(2);
    threw = false;
    try {
        big.compose(small, too_many);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  compose_map_rejects_wrong_length test : a long qubit map was "
            << "accepted" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_compose_map_rejects_out_of_range(void) {
    // A mapped index at or past the base observable's qubit count panics in
    // the C-API.
    auto big = SparseObservable::identity(3);
    auto small = SparseObservable::from_label(std::string("Z"));

    reg_t qargs;
    qargs.push_back(3);

    bool threw = false;
    try {
        big.compose(small, qargs);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  compose_map_rejects_out_of_range test : mapping to qubit 3 of a "
            << "3-qubit observable was accepted" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_compose_map_rejects_duplicate(void) {
    // Two qubits mapped to the same index also panics in the C-API.
    auto big = SparseObservable::identity(4);
    auto small = SparseObservable::from_label(std::string("ZX"));

    reg_t qargs;
    qargs.push_back(1);
    qargs.push_back(1);

    bool threw = false;
    try {
        big.compose(small, qargs);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  compose_map_rejects_duplicate test : mapping two qubits to the "
            << "same index was accepted" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_compose_map_rejects_unset(void) {
    // An observable holding nothing would reach the C-API as a null pointer.
    SparseObservable unset;
    auto obs = SparseObservable::identity(2);
    reg_t qargs;
    qargs.push_back(0);
    qargs.push_back(1);

    bool threw = false;
    try {
        unset.compose(obs, qargs);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  compose_map_rejects_unset test : composing from an unset "
            << "observable was accepted" << std::endl;
        return EqualityError;
    }

    threw = false;
    try {
        obs.compose(unset, reg_t());
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  compose_map_rejects_unset test : composing with an unset "
            << "observable was accepted" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_compose_map_accepts_const(void) {
    // The overload takes its operand by const reference and is itself const.
    const auto big = SparseObservable::identity(3);
    const auto small = SparseObservable::from_label(std::string("Z"));
    const reg_t qargs(1, 1);

    auto composed = big.compose(small, qargs);
    if (composed.num_qubits() != 3 || composed.num_terms() != 1) {
        std::cerr << "  compose_map_accepts_const test : " << composed.num_qubits()
            << " qubits, " << composed.num_terms() << " terms, expected 3 and 1"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

// ===========================================================================
// simplify
// ===========================================================================

static int test_simplify_sums_like_terms(void) {
    // Adding appends terms rather than combining them, so two identities make
    // two terms; simplify sums them into one.
    auto a = SparseObservable::identity(2);
    auto b = SparseObservable::identity(2);
    a += b;

    if (a.num_terms() != 2) {
        std::cerr << "  simplify_sums_like_terms test : " << a.num_terms()
            << " terms before simplifying, expected 2" << std::endl;
        return EqualityError;
    }

    auto simplified = a.simplify();
    if (simplified.num_terms() != 1) {
        std::cerr << "  simplify_sums_like_terms test : " << simplified.num_terms()
            << " terms after simplifying, expected 1" << std::endl;
        return EqualityError;
    }
    // The two unit coefficients must have summed to 2.
    const auto coeffs = simplified.coeffs();
    if (coeffs.size() != 1 || coeffs[0] != std::complex<double>(2.0, 0.0)) {
        std::cerr << "  simplify_sums_like_terms test : the coefficient did not sum to 2"
            << std::endl;
        return EqualityError;
    }
    if (simplified.num_qubits() != 2) {
        std::cerr << "  simplify_sums_like_terms test : " << simplified.num_qubits()
            << " qubits, expected 2" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_simplify_drops_cancelling_terms(void) {
    // A term and its negation cancel, leaving nothing.
    auto a = SparseObservable::from_label(std::string("XZ"));
    auto b = SparseObservable::from_label(std::string("XZ"));
    std::complex<double> minus_one(-1.0, 0.0);
    b *= minus_one;
    a += b;

    auto simplified = a.simplify();
    if (simplified.num_terms() != 0) {
        std::cerr << "  simplify_drops_cancelling_terms test : " << simplified.num_terms()
            << " terms, expected 0" << std::endl;
        return EqualityError;
    }
    // The qubit count survives even when every term is dropped.
    if (simplified.num_qubits() != 2) {
        std::cerr << "  simplify_drops_cancelling_terms test : " << simplified.num_qubits()
            << " qubits, expected 2" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_simplify_tolerance(void) {
    // A coefficient within the tolerance of zero is dropped, one above it is
    // kept.
    std::vector<std::pair<std::string, std::complex<double>>> list;
    list.push_back(std::make_pair(std::string("ZZ"), std::complex<double>(1e-10, 0.0)));
    auto tiny = SparseObservable::from_list(list);

    // The default tolerance of 1e-8 is larger than the coefficient.
    if (tiny.simplify().num_terms() != 0) {
        std::cerr << "  simplify_tolerance test : a 1e-10 coefficient survived the "
            << "default tolerance" << std::endl;
        return EqualityError;
    }
    // A tighter tolerance keeps it.
    if (tiny.simplify(1e-12).num_terms() != 1) {
        std::cerr << "  simplify_tolerance test : a 1e-10 coefficient was dropped at a "
            << "1e-12 tolerance" << std::endl;
        return EqualityError;
    }
    // A zero tolerance keeps anything non-zero.
    if (tiny.simplify(0.0).num_terms() != 1) {
        std::cerr << "  simplify_tolerance test : a 1e-10 coefficient was dropped at a "
            << "zero tolerance" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_simplify_is_idempotent(void) {
    // Simplifying an already simplified observable changes nothing.
    auto a = SparseObservable::identity(3);
    auto b = SparseObservable::identity(3);
    a += b;

    auto once = a.simplify();
    auto twice = once.simplify();

    if (once.num_terms() != twice.num_terms()
        || once.to_string() != twice.to_string()) {
        std::cerr << "  simplify_is_idempotent test : \"" << twice.to_string()
            << "\" differs from \"" << once.to_string() << "\"" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_simplify_does_not_touch_source(void) {
    // simplify returns a new observable and leaves this one alone.
    auto a = SparseObservable::identity(2);
    auto b = SparseObservable::identity(2);
    a += b;

    const uint_t before = a.num_terms();
    const std::string before_string = a.to_string();

    a.simplify();

    if (a.num_terms() != before || a.to_string() != before_string) {
        std::cerr << "  simplify_does_not_touch_source test : the source changed from \""
            << before_string << "\" to \"" << a.to_string() << "\"" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_simplify_makes_equal_comparable(void) {
    // The main reason to simplify: operator== compares term by term, so
    // observables built in a different order compare unequal until both are
    // put into canonical form.
    std::vector<std::pair<std::string, std::complex<double>>> forward;
    forward.push_back(std::make_pair(std::string("XI"), std::complex<double>(1.0, 0.0)));
    forward.push_back(std::make_pair(std::string("IZ"), std::complex<double>(2.0, 0.0)));

    std::vector<std::pair<std::string, std::complex<double>>> reversed;
    reversed.push_back(std::make_pair(std::string("IZ"), std::complex<double>(2.0, 0.0)));
    reversed.push_back(std::make_pair(std::string("XI"), std::complex<double>(1.0, 0.0)));

    auto a = SparseObservable::from_list(forward);
    auto b = SparseObservable::from_list(reversed);

    auto sa = a.simplify();
    auto sb = b.simplify();

    if (sa.num_terms() != 2 || sb.num_terms() != 2) {
        std::cerr << "  simplify_makes_equal_comparable test : " << sa.num_terms()
            << " and " << sb.num_terms() << " terms, expected 2 each" << std::endl;
        return EqualityError;
    }
    if (!(sa == sb)) {
        std::cerr << "  simplify_makes_equal_comparable test : \"" << sa.to_string()
            << "\" and \"" << sb.to_string()
            << "\" compare unequal after simplifying" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_simplify_rejects_bad_tolerance(void) {
    auto obs = SparseObservable::identity(2);

    // A negative tolerance would act as its own magnitude.
    bool threw = false;
    try {
        obs.simplify(-1.0);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  simplify_rejects_bad_tolerance test : a negative tolerance was "
            << "accepted" << std::endl;
        return EqualityError;
    }

    // A NaN tolerance would silently drop every term, since any comparison
    // against NaN is false.
    threw = false;
    try {
        obs.simplify(std::nan(""));
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  simplify_rejects_bad_tolerance test : a NaN tolerance was accepted"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_simplify_rejects_unset(void) {
    // An observable holding nothing would reach the C-API as a null pointer.
    SparseObservable unset;

    bool threw = false;
    try {
        unset.simplify();
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  simplify_rejects_unset test : an unset observable was simplified"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_simplify_accepts_const(void) {
    // simplify is const, so it works on a const observable.
    const auto obs = SparseObservable::identity(2);
    auto simplified = obs.simplify();

    if (simplified.num_terms() != 1 || simplified.num_qubits() != 2) {
        std::cerr << "  simplify_accepts_const test : " << simplified.num_terms()
            << " terms, " << simplified.num_qubits() << " qubits, expected 1 and 2"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

// ===========================================================================
// apply_layout
// ===========================================================================

static int test_apply_layout_permutes(void) {
    // A layout of the same width relabels qubits without widening.
    auto obs = SparseObservable::from_label(std::string("ZX"));
    // from_label gives X on qubit 0 and Z on qubit 1. Swap them.
    reg_t layout;
    layout.push_back(1);
    layout.push_back(0);

    auto mapped = obs.apply_layout(layout);
    if (mapped.num_qubits() != 2 || mapped.num_terms() != 1) {
        std::cerr << "  apply_layout_permutes test : " << mapped.num_qubits()
            << " qubits, " << mapped.num_terms() << " terms, expected 2 and 1" << std::endl;
        return EqualityError;
    }

    Term term = mapped[0];
    // X moves to qubit 1, Z to qubit 0, and indices come back sorted.
    const reg_t expected({0, 1});
    if (term.indices() != expected || term.bit_labels() != "ZX") {
        std::cerr << "  apply_layout_permutes test : term is \"" << term.to_string()
            << "\", expected Z on qubit 0 and X on qubit 1" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_apply_layout_widens(void) {
    // The usual transpiler case: a small observable moved onto the physical
    // qubits of a wider device.
    auto obs = SparseObservable::from_label(std::string("ZX"));

    // A layout sized to the device, as get_qubit_map() returns. Virtual qubit
    // 0 goes to physical 7, virtual 1 to physical 3; the rest are ancillas.
    // Every entry must be a distinct index below the output width, because the
    // C-API validates the whole layout and not only the entries it uses.
    reg_t layout;
    layout.push_back(7);
    layout.push_back(3);
    const uint_t ancillas[] = {0, 1, 2, 4, 5, 6, 8, 9};
    for (uint_t i = 0; i < 8; i++) {
        layout.push_back(ancillas[i]);
    }

    auto mapped = obs.apply_layout(layout);
    if (mapped.num_qubits() != layout.size()) {
        std::cerr << "  apply_layout_widens test : " << mapped.num_qubits()
            << " qubits, expected " << layout.size() << std::endl;
        return EqualityError;
    }

    Term term = mapped[0];
    // X was on qubit 0 so lands on 7; Z was on qubit 1 so lands on 3.
    const reg_t expected({3, 7});
    if (term.indices() != expected || term.bit_labels() != "ZX") {
        std::cerr << "  apply_layout_widens test : term is \"" << term.to_string()
            << "\", expected Z on qubit 3 and X on qubit 7" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_apply_layout_explicit_width(void) {
    // The output width can be given explicitly, which is the only way to widen
    // when the layout is sized to the observable rather than the device.
    auto obs = SparseObservable::from_label(std::string("X"));

    reg_t layout;
    layout.push_back(4);

    // Without a width the result would be 1 qubit wide, which cannot hold
    // index 4, so this also checks the width is really used.
    auto mapped = obs.apply_layout(layout, 10);
    if (mapped.num_qubits() != 10) {
        std::cerr << "  apply_layout_explicit_width test : " << mapped.num_qubits()
            << " qubits, expected 10" << std::endl;
        return EqualityError;
    }
    Term term = mapped[0];
    if (term.len() != 1 || term.indices()[0] != 4 || term.bit_labels() != "X") {
        std::cerr << "  apply_layout_explicit_width test : term is \"" << term.to_string()
            << "\", expected X on qubit 4" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_apply_layout_does_not_touch_source(void) {
    auto obs = SparseObservable::from_label(std::string("ZX"));
    const uint_t before_qubits = obs.num_qubits();
    const std::string before = obs.to_string();

    reg_t layout;
    layout.push_back(3);
    layout.push_back(1);
    obs.apply_layout(layout, 5);

    if (obs.num_qubits() != before_qubits || obs.to_string() != before) {
        std::cerr << "  apply_layout_does_not_touch_source test : the source changed from \""
            << before << "\" to \"" << obs.to_string() << "\"" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_apply_layout_rejects_short_layout(void) {
    // The C-API reads num_qubits() entries regardless of how many are given,
    // so a short layout would read past the end of the array.
    auto obs = SparseObservable::from_label(std::string("ZZZ"));

    reg_t layout;
    layout.push_back(0);
    layout.push_back(1);

    bool threw = false;
    try {
        obs.apply_layout(layout);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  apply_layout_rejects_short_layout test : a 2-entry layout was "
            << "accepted for a 3-qubit observable" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_apply_layout_rejects_narrowing(void) {
    // An observable cannot be narrowed.
    auto obs = SparseObservable::from_label(std::string("ZZZ"));

    reg_t layout;
    layout.push_back(0);
    layout.push_back(1);
    layout.push_back(2);

    bool threw = false;
    try {
        obs.apply_layout(layout, 2);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  apply_layout_rejects_narrowing test : narrowing a 3-qubit "
            << "observable to 2 qubits was accepted" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_apply_layout_rejects_out_of_range(void) {
    auto obs = SparseObservable::from_label(std::string("ZX"));

    // Index 2 is out of range for a 2-qubit result.
    reg_t layout;
    layout.push_back(0);
    layout.push_back(2);

    bool threw = false;
    try {
        obs.apply_layout(layout);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  apply_layout_rejects_out_of_range test : mapping to qubit 2 of a "
            << "2-qubit result was accepted" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_apply_layout_rejects_duplicate(void) {
    auto obs = SparseObservable::from_label(std::string("ZX"));

    reg_t layout;
    layout.push_back(1);
    layout.push_back(1);

    bool threw = false;
    try {
        obs.apply_layout(layout, 4);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  apply_layout_rejects_duplicate test : mapping two qubits to the "
            << "same index was accepted" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_apply_layout_rejects_unset(void) {
    SparseObservable unset;
    reg_t layout;
    layout.push_back(0);

    bool threw = false;
    try {
        unset.apply_layout(layout);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  apply_layout_rejects_unset test : an unset observable was "
            << "relabelled" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_apply_layout_accepts_const(void) {
    const auto obs = SparseObservable::from_label(std::string("X"));
    const reg_t layout(1, 2);

    auto mapped = obs.apply_layout(layout, 5);
    if (mapped.num_qubits() != 5 || mapped.num_terms() != 1) {
        std::cerr << "  apply_layout_accepts_const test : " << mapped.num_qubits()
            << " qubits, " << mapped.num_terms() << " terms, expected 5 and 1" << std::endl;
        return EqualityError;
    }
    return Ok;
}

// ===========================================================================
// Construction from the C-API types
// ===========================================================================

static int test_observable_from_obs_copies(void) {
    // The caller keeps ownership, so the wrapper must hold its own copy. This
    // is the usage from issue #162: build with the C-API, wrap, then free.
    QkObs *raw = qk_obs_identity(3);
    if (raw == nullptr) {
        std::cerr << "  observable_from_obs_copies test : could not create a C-API "
            << "observable" << std::endl;
        return RuntimeError;
    }

    auto wrapped = SparseObservable::from_obs(raw);
    qk_obs_free(raw);

    // Still usable after the original was freed, which only holds if it copied.
    if (wrapped.num_qubits() != 3 || wrapped.num_terms() != 1) {
        std::cerr << "  observable_from_obs_copies test : " << wrapped.num_qubits()
            << " qubits, " << wrapped.num_terms() << " terms, expected 3 and 1"
            << std::endl;
        return EqualityError;
    }
    if (wrapped.to_string().empty()) {
        std::cerr << "  observable_from_obs_copies test : the copy could not be rendered"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_constructor_from_obs(void) {
    // The constructor is the same operation as from_obs.
    QkObs *raw = qk_obs_identity(2);
    SparseObservable wrapped(raw);
    qk_obs_free(raw);

    if (wrapped.num_qubits() != 2 || wrapped.num_terms() != 1) {
        std::cerr << "  observable_constructor_from_obs test : " << wrapped.num_qubits()
            << " qubits, " << wrapped.num_terms() << " terms, expected 2 and 1"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_from_obs_null(void) {
    // A null handle gives an observable holding nothing, rather than a crash.
    auto wrapped = SparseObservable::from_obs(nullptr);
    if (wrapped.num_qubits() != 0 || wrapped.num_terms() != 0) {
        std::cerr << "  observable_from_obs_null test : a null handle did not give an "
            << "empty observable" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_from_owned_obs(void) {
    // Ownership passes to the wrapper, so the caller must not free it. If the
    // wrapper did not take it, this would leak; if it took it twice, the
    // destructor would double-free.
    QkObs *raw = qk_obs_identity(4);
    {
        auto wrapped = SparseObservable::from_owned_obs(raw);
        if (wrapped.num_qubits() != 4 || wrapped.num_terms() != 1) {
            std::cerr << "  observable_from_owned_obs test : " << wrapped.num_qubits()
                << " qubits, " << wrapped.num_terms() << " terms, expected 4 and 1"
                << std::endl;
            return EqualityError;
        }
    }
    // wrapped is gone and has freed raw; nothing more to do here.

    // A null handle is accepted and leaves the observable holding nothing.
    auto empty = SparseObservable::from_owned_obs(nullptr);
    if (empty.num_terms() != 0) {
        std::cerr << "  observable_from_owned_obs test : a null handle did not give an "
            << "empty observable" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_from_obs_term_round_trip(void) {
    // Build a term with the C-API, wrap it, and check it survives.
    const QkBitTerm bits[] = {QkBitTerm_Z, QkBitTerm_X};
    std::uint32_t indices[] = {0, 2};

    QkObsTerm cterm;
    cterm.coeff.re = 1.5;
    cterm.coeff.im = -0.5;
    cterm.len = 2;
    cterm.bit_terms = const_cast<QkBitTerm *>(bits);
    cterm.indices = indices;
    cterm.num_qubits = 3;

    Term term(cterm);
    if (term.num_qubits() != 3 || term.len() != 2) {
        std::cerr << "  observable_from_obs_term_round_trip test : " << term.num_qubits()
            << " qubits, " << term.len() << " operators, expected 3 and 2" << std::endl;
        return EqualityError;
    }
    if (term.coeff() != std::complex<double>(1.5, -0.5)) {
        std::cerr << "  observable_from_obs_term_round_trip test : the coefficient did "
            << "not survive" << std::endl;
        return EqualityError;
    }
    const reg_t expected({0, 2});
    if (term.indices() != expected || term.bit_labels() != "ZX") {
        std::cerr << "  observable_from_obs_term_round_trip test : term is \""
            << term.to_string() << "\", expected Z on qubit 0 and X on qubit 2"
            << std::endl;
        return EqualityError;
    }

    // The term must own its data: changing the source arrays must not affect it.
    indices[0] = 1;
    if (term.indices()[0] != 0) {
        std::cerr << "  observable_from_obs_term_round_trip test : the term aliased the "
            << "source index array" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_from_obs_term_identity(void) {
    // A term with no operators may carry null arrays.
    QkObsTerm cterm;
    cterm.coeff.re = 2.0;
    cterm.coeff.im = 0.0;
    cterm.len = 0;
    cterm.bit_terms = nullptr;
    cterm.indices = nullptr;
    cterm.num_qubits = 5;

    Term term(cterm);
    if (term.num_qubits() != 5 || term.len() != 0) {
        std::cerr << "  observable_from_obs_term_identity test : " << term.num_qubits()
            << " qubits, " << term.len() << " operators, expected 5 and 0" << std::endl;
        return EqualityError;
    }
    if (term.coeff() != std::complex<double>(2.0, 0.0)) {
        std::cerr << "  observable_from_obs_term_identity test : the coefficient did not "
            << "survive" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_from_obs_term_rejects_bad(void) {
    // Operators with a null array, which the C-API only allows at length zero.
    QkObsTerm cterm;
    cterm.coeff.re = 1.0;
    cterm.coeff.im = 0.0;
    cterm.len = 2;
    cterm.bit_terms = nullptr;
    cterm.indices = nullptr;
    cterm.num_qubits = 2;

    bool threw = false;
    try {
        Term bad(cterm);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  observable_from_obs_term_rejects_bad test : a null array with a "
            << "non-zero length was accepted" << std::endl;
        return EqualityError;
    }

    // An index at or past the term's qubit count.
    const QkBitTerm bits[] = {QkBitTerm_X};
    std::uint32_t out_of_range[] = {2};
    cterm.len = 1;
    cterm.bit_terms = const_cast<QkBitTerm *>(bits);
    cterm.indices = out_of_range;
    cterm.num_qubits = 2;

    threw = false;
    try {
        Term bad(cterm);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    if (!threw) {
        std::cerr << "  observable_from_obs_term_rejects_bad test : an out-of-range index "
            << "was accepted" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_observable_term_to_obsterm(void) {
    // The reverse direction: a Term handed back to the C-API must survive a
    // round trip through an observable.
    std::vector<QkBitTerm> bits;
    bits.push_back(QkBitTerm_Y);
    reg_t indices;
    indices.push_back(1);
    Term original(3, std::complex<double>(0.0, 2.0), bits, indices);

    std::vector<QkBitTerm> scratch_bits;
    std::vector<std::uint32_t> scratch_indices;
    QkObsTerm cterm;
    original.to_obsterm(cterm, scratch_bits, scratch_indices);

    if (cterm.num_qubits != 3 || cterm.len != 1
        || cterm.coeff.re != 0.0 || cterm.coeff.im != 2.0
        || cterm.bit_terms[0] != QkBitTerm_Y || cterm.indices[0] != 1) {
        std::cerr << "  observable_term_to_obsterm test : the C-API term does not match "
            << "the source" << std::endl;
        return EqualityError;
    }

    // Reading it back through Term must give the original.
    Term round_tripped(cterm);
    if (round_tripped != original) {
        std::cerr << "  observable_term_to_obsterm test : the term did not round-trip"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

#if defined(_WIN32)
int test_observable(int argc, char** const argv) {
#else
int test_observable(int argc, char** argv) {
#endif
    int num_failed = 0;
    num_failed += RUN_TEST(test_observable_add_inplace);
    num_failed += RUN_TEST(test_observable_add_inplace_does_not_touch_rhs);
    num_failed += RUN_TEST(test_observable_add_inplace_self);
    num_failed += RUN_TEST(test_observable_add_inplace_accumulate);
    num_failed += RUN_TEST(test_observable_add_inplace_unset);
    num_failed += RUN_TEST(test_observable_multiply_inplace);
    num_failed += RUN_TEST(test_observable_multiply_inplace_complex);
    num_failed += RUN_TEST(test_observable_multiply_inplace_unset);
    num_failed += RUN_TEST(test_observable_inplace_accepts_const);
    num_failed += RUN_TEST(test_bit_term_labels);
    num_failed += RUN_TEST(test_term_accessors);
    num_failed += RUN_TEST(test_term_rejects_bad_input);
    num_failed += RUN_TEST(test_term_identity);
    num_failed += RUN_TEST(test_observable_index_term);
    num_failed += RUN_TEST(test_observable_index_out_of_range);
    num_failed += RUN_TEST(test_observable_index_multi_term);
    num_failed += RUN_TEST(test_observable_from_terms);
    num_failed += RUN_TEST(test_observable_from_terms_empty);
    num_failed += RUN_TEST(test_observable_from_terms_rejects_mismatch);
    num_failed += RUN_TEST(test_observable_from_terms_matches_from_label);
    num_failed += RUN_TEST(test_term_to_observable);
    num_failed += RUN_TEST(test_observable_add_term);
    num_failed += RUN_TEST(test_compose_map_identity_order);
    num_failed += RUN_TEST(test_compose_map_smaller_operand);
    num_failed += RUN_TEST(test_compose_map_permutes);
    num_failed += RUN_TEST(test_compose_map_rejects_larger_operand);
    num_failed += RUN_TEST(test_compose_map_rejects_wrong_length);
    num_failed += RUN_TEST(test_compose_map_rejects_out_of_range);
    num_failed += RUN_TEST(test_compose_map_rejects_duplicate);
    num_failed += RUN_TEST(test_compose_map_rejects_unset);
    num_failed += RUN_TEST(test_compose_map_accepts_const);
    num_failed += RUN_TEST(test_simplify_sums_like_terms);
    num_failed += RUN_TEST(test_simplify_drops_cancelling_terms);
    num_failed += RUN_TEST(test_simplify_tolerance);
    num_failed += RUN_TEST(test_simplify_is_idempotent);
    num_failed += RUN_TEST(test_simplify_does_not_touch_source);
    num_failed += RUN_TEST(test_simplify_makes_equal_comparable);
    num_failed += RUN_TEST(test_simplify_rejects_bad_tolerance);
    num_failed += RUN_TEST(test_simplify_rejects_unset);
    num_failed += RUN_TEST(test_simplify_accepts_const);
    num_failed += RUN_TEST(test_apply_layout_permutes);
    num_failed += RUN_TEST(test_apply_layout_widens);
    num_failed += RUN_TEST(test_apply_layout_explicit_width);
    num_failed += RUN_TEST(test_apply_layout_does_not_touch_source);
    num_failed += RUN_TEST(test_apply_layout_rejects_short_layout);
    num_failed += RUN_TEST(test_apply_layout_rejects_narrowing);
    num_failed += RUN_TEST(test_apply_layout_rejects_out_of_range);
    num_failed += RUN_TEST(test_apply_layout_rejects_duplicate);
    num_failed += RUN_TEST(test_apply_layout_rejects_unset);
    num_failed += RUN_TEST(test_apply_layout_accepts_const);
    num_failed += RUN_TEST(test_observable_from_obs_copies);
    num_failed += RUN_TEST(test_observable_constructor_from_obs);
    num_failed += RUN_TEST(test_observable_from_obs_null);
    num_failed += RUN_TEST(test_observable_from_owned_obs);
    num_failed += RUN_TEST(test_observable_from_obs_term_round_trip);
    num_failed += RUN_TEST(test_observable_from_obs_term_identity);
    num_failed += RUN_TEST(test_observable_from_obs_term_rejects_bad);
    num_failed += RUN_TEST(test_observable_term_to_obsterm);

    std::cerr << "=== Number of failed subtests: " << num_failed << std::endl;
    return num_failed;
}
