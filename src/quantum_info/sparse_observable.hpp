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

// class for sparse observable

#ifndef __qiskitcpp_quantum_info_sparse_observable_hpp__
#define __qiskitcpp_quantum_info_sparse_observable_hpp__

#include <cmath>
#include <complex>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "qiskit.h"
#include "utils/types.hpp"

namespace Qiskit
{
namespace quantum_info
{

class SparseObservable;

/// @brief Return the label of a single-qubit term, e.g. 'X' or '+'.
/// @param bit_term the single-qubit term
/// @return the one-character label
inline char bit_term_label(const QkBitTerm bit_term)
{
    return (char)qk_bitterm_label(bit_term);
}

/// @class Term
/// @brief A single term of a SparseObservable.
/// @details Mirrors ``SparseObservable.Term`` in Qiskit Python: a coefficient
///   together with the non-identity single-qubit operators of one term and the
///   qubits they act on. A term is normally obtained by indexing a
///   ``SparseObservable`` rather than built directly, though the constructor is
///   available for assembling an observable with
///   ``SparseObservable::from_terms``.
/// @note Owns its data. The C-API hands back pointers into the observable's own
///   buffers, so a Term copies out of them: writing through those pointers can
///   leave the observable in an incoherent state.
class Term
{
protected:
    uint_t num_qubits_ = 0;
    std::complex<double> coeff_ = std::complex<double>(0.0, 0.0);
    std::vector<QkBitTerm> bit_terms_;
    reg_t indices_;

public:
    /// @brief Create an empty term on no qubits.
    Term(void) {}

    /// @brief Create a new term.
    /// @param num_qubits the number of qubits the term is defined on
    /// @param coeff the coefficient of the term
    /// @param bit_terms the non-identity single-qubit operators
    /// @param indices the qubit each operator acts on, in sorted order
    /// @throws std::invalid_argument if the operator and index lists differ in
    ///   length, or if an index is not below ``num_qubits``
    /// @note The indices must be sorted and free of duplicates, as Qiskit
    ///   requires. That is not checked here: ``SparseObservable::from_terms``
    ///   reports it when the term is added.
    Term(uint_t num_qubits, const std::complex<double> &coeff,
         const std::vector<QkBitTerm> &bit_terms, const reg_t &indices)
    {
        if (bit_terms.size() != indices.size()) {
            std::stringstream message;
            message << "Term holds " << bit_terms.size() << " operators but "
                    << indices.size() << " indices.";
            throw std::invalid_argument(message.str());
        }
        for (uint_t i = 0; i < indices.size(); i++) {
            if (indices[i] >= num_qubits) {
                std::stringstream message;
                message << "Term index " << indices[i]
                        << " is out of range for a term on " << num_qubits
                        << " qubits.";
                throw std::invalid_argument(message.str());
            }
        }

        num_qubits_ = num_qubits;
        coeff_ = coeff;
        bit_terms_ = bit_terms;
        indices_ = indices;
    }

    /// @brief Return the number of qubits this term is defined on.
    uint_t num_qubits(void) const { return num_qubits_; }

    /// @brief Return the coefficient of this term.
    const std::complex<double> &coeff(void) const { return coeff_; }

    /// @brief Return the non-identity single-qubit operators of this term.
    /// @return the operators, in the same order as ``indices()``
    const std::vector<QkBitTerm> &bit_terms(void) const { return bit_terms_; }

    /// @brief Return the qubits this term's operators act on.
    /// @return the indices, in sorted order
    const reg_t &indices(void) const { return indices_; }

    /// @brief Return the number of non-identity operators in this term.
    uint_t len(void) const { return bit_terms_.size(); }

    /// @brief Return the labels of this term's operators.
    /// @return one character per operator, in the order of ``indices()``
    /// @details Matches ``Term.bit_labels()`` in Qiskit Python: the i-th
    ///   character is the operator acting on qubit ``indices()[i]``.
    std::string bit_labels(void) const
    {
        std::string ret;
        ret.reserve(bit_terms_.size());
        for (uint_t i = 0; i < bit_terms_.size(); i++) {
            ret += bit_term_label(bit_terms_[i]);
        }
        return ret;
    }

    /// @brief Return this term as a dense label over all of its qubits.
    /// @return a label of ``num_qubits()`` characters, identities written as
    ///   '_', with qubit 0 rightmost
    /// @details The ordering matches ``SparseObservable::from_label``, which
    ///   reads the rightmost character as qubit 0.
    std::string to_label(void) const
    {
        std::string ret(num_qubits_, '_');
        for (uint_t i = 0; i < bit_terms_.size(); i++) {
            // Qubit 0 is the last character, as in Qiskit's dense labels.
            ret[num_qubits_ - 1 - indices_[i]] = bit_term_label(bit_terms_[i]);
        }
        return ret;
    }

    /// @brief Return this term as a readable string.
    /// @return a rendering such as ``(1+0i) X_2 Z_0``
    std::string to_string(void) const
    {
        std::stringstream ret;
        ret << "(" << coeff_.real();
        if (coeff_.imag() >= 0.0) {
            ret << "+";
        }
        ret << coeff_.imag() << "i)";

        if (bit_terms_.empty()) {
            // An empty term is the identity, scaled by the coefficient.
            ret << " I";
            return ret.str();
        }
        for (uint_t i = 0; i < bit_terms_.size(); i++) {
            ret << " " << bit_term_label(bit_terms_[i]) << "_" << indices_[i];
        }
        return ret.str();
    }

    /// @brief Compare two terms for equality.
    /// @param rhs the term to compare with
    /// @return true if the qubit count, coefficient and operators all match
    bool operator==(const Term &rhs) const
    {
        return num_qubits_ == rhs.num_qubits_ && coeff_ == rhs.coeff_
            && bit_terms_ == rhs.bit_terms_ && indices_ == rhs.indices_;
    }

    /// @brief Compare two terms for inequality.
    /// @param rhs the term to compare with
    /// @return true if the terms differ
    bool operator!=(const Term &rhs) const
    {
        return !(*this == rhs);
    }

    /// @brief Return this term as an observable in its own right.
    /// @return an observable holding only this term
    /// @note Defined after SparseObservable, which it constructs.
    SparseObservable to_observable(void) const;
};

class SparseObservable
{
protected:
    QkObs *obs_;

public:
    SparseObservable(void)
    {
        obs_ = nullptr;
    }
    SparseObservable(uint_t num_qubits, std::vector<std::complex<double>> &coeffs, std::vector<QkBitTerm> &bits, reg_t &indicies, std::vector<size_t> &boundaries)
    {
        std::vector<std::uint32_t> idx32(indicies.size());
        for (int i = 0; i < indicies.size(); i++) {
            idx32[i] = (std::uint32_t)indicies[i];
        }
        obs_ = qk_obs_new((std::uint32_t)num_qubits, coeffs.size(), bits.size(), (QkComplex64 *)coeffs.data(), bits.data(), idx32.data(), boundaries.data());
    }

    SparseObservable(const SparseObservable &other)
    {
        obs_ = other.obs_ ? qk_obs_copy(other.obs_) : nullptr;
    }

    SparseObservable &operator=(const SparseObservable &other)
    {
        if (this != &other)
        {
            if (obs_)
            {
                qk_obs_free(obs_);
            }
            obs_ = other.obs_ ? qk_obs_copy(other.obs_) : nullptr;
        }
        return *this;
    }

    SparseObservable(SparseObservable &&other)
    {
        obs_ = other.obs_;
        other.obs_ = nullptr;
    }

    SparseObservable &operator=(SparseObservable &&other)
    {
        if (this != &other)
        {
            if (obs_)
            {
                qk_obs_free(obs_);
            }
            obs_ = other.obs_;
            other.obs_ = nullptr;
        }
        return *this;
    }

    ~SparseObservable()
    {
        if (obs_)
        {
            qk_obs_free(obs_);
        }
    }

    static SparseObservable zero(uint_t num_qubits)
    {
        SparseObservable ret;
        ret.obs_ = qk_obs_zero(num_qubits);
        return ret;
    }

    static SparseObservable identity(uint_t num_qubits)
    {
        SparseObservable ret;
        ret.obs_ = qk_obs_identity(num_qubits);
        return ret;
    }

    static SparseObservable from_label(const std::string &label)
    {
        reg_t indices;
        std::vector<size_t> boundaries(2);
        uint_t num_qubits = label.length();
        std::vector<QkBitTerm> terms;
        std::vector<std::complex<double>> coeff(1);
        coeff[0] = 1.0;

        for (int i = 0; i < num_qubits; i++)
        {
            switch (label[num_qubits - 1 - i])
            {
            case 'X':
                terms.push_back(QkBitTerm_X);
                break;
            case 'Y':
                terms.push_back(QkBitTerm_Y);
                break;
            case 'Z':
                terms.push_back(QkBitTerm_Z);
                break;
            case '+':
                terms.push_back(QkBitTerm_Plus);
                break;
            case '-':
                terms.push_back(QkBitTerm_Minus);
                break;
            case 'l':
                terms.push_back(QkBitTerm_Left);
                break;
            case 'r':
                terms.push_back(QkBitTerm_Right);
                break;
            case '0':
                terms.push_back(QkBitTerm_Zero);
                break;
            case '1':
                terms.push_back(QkBitTerm_One);
                break;
            default:
                break;
            }
            if (indices.size() < terms.size())
                indices.push_back(i);
        }
        boundaries[0] = 0;
        boundaries[1] = terms.size();
        return SparseObservable(num_qubits, coeff, terms, indices, boundaries);
    }

    static SparseObservable from_list(const std::vector<std::pair<std::string, std::complex<double>>> &list, uint_t num_qubits_in = 0)
    {
        reg_t indices;
        std::vector<size_t> boundaries;
        uint_t num_qubits = num_qubits_in;
        std::vector<QkBitTerm> terms;
        std::vector<std::complex<double>> coeffs;
        boundaries.push_back(0);

        for (int i = 0; i < list.size(); i++)
        {
            if (num_qubits == 0)
            {
                num_qubits = list[i].first.length();
            }
            else if (num_qubits != list[i].first.length())
            {
                std::cerr << " SparseObservable : Error label with length " << list[i].first.length() << "cannot be added to a " << num_qubits << "-qubits operator" << std::endl;
                abort();
            }
            for (int j = 0; j < num_qubits; j++)
            {
                switch (list[i].first[num_qubits - 1 - j])
                {
                case 'X':
                    terms.push_back(QkBitTerm_X);
                    break;
                case 'Y':
                    terms.push_back(QkBitTerm_Y);
                    break;
                case 'Z':
                    terms.push_back(QkBitTerm_Z);
                    break;
                case '+':
                    terms.push_back(QkBitTerm_Plus);
                    break;
                case '-':
                    terms.push_back(QkBitTerm_Minus);
                    break;
                case 'l':
                    terms.push_back(QkBitTerm_Left);
                    break;
                case 'r':
                    terms.push_back(QkBitTerm_Right);
                    break;
                case '0':
                    terms.push_back(QkBitTerm_Zero);
                    break;
                case '1':
                    terms.push_back(QkBitTerm_One);
                    break;
                default:
                    break;
                }
                if (indices.size() < terms.size())
                    indices.push_back(j);
            }
            boundaries.push_back(terms.size());
            coeffs.push_back(list[i].second);
        }
        return SparseObservable(num_qubits, coeffs, terms, indices, boundaries);
    }

    /// @brief Build an observable from a list of terms.
    /// @param terms the terms to sum, which must all act on the same number of
    ///   qubits
    /// @param num_qubits the number of qubits the observable is defined on,
    ///   used only when ``terms`` is empty
    /// @return the observable
    /// @throws std::invalid_argument if the terms disagree on the qubit count,
    ///   or if a term holds unsorted or duplicated indices
    /// @details Matches ``SparseObservable.from_terms`` in Qiskit Python. The
    ///   terms are appended in order through ``qk_obs_add_term``, so the result
    ///   holds one term per entry without combining them; call
    ///   ``canonicalize`` to collapse duplicates.
    static SparseObservable from_terms(const std::vector<Term> &terms,
                                       uint_t num_qubits = 0)
    {
        if (!terms.empty()) {
            num_qubits = terms[0].num_qubits();
            for (uint_t i = 1; i < terms.size(); i++) {
                if (terms[i].num_qubits() != num_qubits) {
                    std::stringstream message;
                    message << "Term " << i << " is defined on "
                            << terms[i].num_qubits() << " qubits but term 0 is defined on "
                            << num_qubits << ".";
                    throw std::invalid_argument(message.str());
                }
            }
        }

        SparseObservable ret;
        ret.obs_ = qk_obs_zero((std::uint32_t)num_qubits);
        if (ret.obs_ == nullptr) {
            throw std::runtime_error("SparseObservable::from_terms could not create an observable.");
        }

        for (uint_t i = 0; i < terms.size(); i++) {
            const QkExitCode code = ret.add_term(terms[i]);
            if (code != QkExitCode_Success) {
                std::stringstream message;
                message << "SparseObservable::from_terms could not add term " << i
                        << " (" << code << ").";
                throw std::invalid_argument(message.str());
            }
        }
        return ret;
    }

    /// @brief Append a term to this observable.
    /// @param term the term to append, which must act on the same number of
    ///   qubits as this observable
    /// @return QkExitCode_Success, or the code the C-API reported
    /// @details Appends through ``qk_obs_add_term`` without combining it with
    ///   any existing term. ``QkExitCode_MismatchedQubits`` means the qubit
    ///   counts differ and ``QkExitCode_DuplicateIndexError`` that the term
    ///   repeats a qubit index.
    QkExitCode add_term(const Term &term)
    {
        if (!obs_) {
            // An unset observable takes its qubit count from the first term.
            obs_ = qk_obs_zero((std::uint32_t)term.num_qubits());
            if (obs_ == nullptr) {
                return QkExitCode_CInputError;
            }
        }

        // The C-API reads through these pointers but does not keep them, so
        // local copies are enough. They must be non-null only when the term
        // holds operators.
        std::vector<QkBitTerm> bit_terms = term.bit_terms();
        std::vector<std::uint32_t> indices(term.indices().size());
        for (uint_t i = 0; i < term.indices().size(); i++) {
            indices[i] = (std::uint32_t)term.indices()[i];
        }

        QkObsTerm cterm;
        // Set member-wise rather than by reinterpreting the std::complex, so
        // this does not depend on the two types sharing a layout.
        cterm.coeff.re = term.coeff().real();
        cterm.coeff.im = term.coeff().imag();
        cterm.len = bit_terms.size();
        cterm.bit_terms = bit_terms.empty() ? nullptr : bit_terms.data();
        cterm.indices = indices.empty() ? nullptr : indices.data();
        cterm.num_qubits = (std::uint32_t)term.num_qubits();

        return qk_obs_add_term(obs_, &cterm);
    }

    /// @brief Return one term of this observable.
    /// @param index the index of the term
    /// @return the term, which owns its own copy of the data
    /// @throws std::out_of_range if the index is past the end
    /// @details Matches indexing a ``SparseObservable`` in Qiskit Python. The
    ///   index is checked here rather than left to the C-API, whose own bounds
    ///   check uses ``>`` where it needs ``>=`` and so reads past the end of
    ///   its arrays for ``index == num_terms()``.
    Term operator[](const uint_t index) const
    {
        const uint_t count = num_terms();
        if (index >= count) {
            std::stringstream message;
            message << "SparseObservable holds " << count
                    << " terms, cannot access term " << index;
            throw std::out_of_range(message.str());
        }

        QkObsTerm cterm;
        // qk_obs_term takes a mutable observable because the term it returns
        // points into the observable's own buffers. Term copies them out, so
        // nothing is mutated and the const-ness of this method holds.
        const QkExitCode code = qk_obs_term(obs_, (std::uint64_t)index, &cterm);
        if (code != QkExitCode_Success) {
            std::stringstream message;
            message << "SparseObservable could not read term " << index
                    << " (" << code << ")";
            throw std::out_of_range(message.str());
        }

        std::vector<QkBitTerm> bit_terms(cterm.len);
        reg_t indices(cterm.len);
        for (uint_t i = 0; i < cterm.len; i++) {
            bit_terms[i] = cterm.bit_terms[i];
            indices[i] = cterm.indices[i];
        }

        return Term((uint_t)cterm.num_qubits,
                    std::complex<double>(cterm.coeff.re, cterm.coeff.im),
                    bit_terms, indices);
    }

    /// @brief Return every term of this observable.
    /// @return the terms, in order
    std::vector<Term> terms(void) const
    {
        const uint_t count = num_terms();
        std::vector<Term> ret;
        ret.reserve(count);
        for (uint_t i = 0; i < count; i++) {
            ret.push_back((*this)[i]);
        }
        return ret;
    }

    uint_t num_qubits(void) const
    {
        if (obs_)
        {
            return qk_obs_num_qubits(obs_);
        }
        return 0;
    }
    uint_t num_terms(void) const
    {
        if (obs_)
        {
            return qk_obs_num_terms(obs_);
        }
        return 0;
    }
    std::vector<QkBitTerm> bit_terms(void) const
    {
        if (obs_)
        {
            std::vector<QkBitTerm> ret(qk_obs_len(obs_));
            auto terms = qk_obs_bit_terms(obs_);
            for (int i = 0; i < ret.size(); i++)
            {
                ret[i] = terms[i];
            }
            return ret;
        }
        return std::vector<QkBitTerm>();
    }
    std::vector<std::complex<double>> coeffs(void) const
    {
        std::vector<std::complex<double>> ret(num_terms());
        if (obs_)
        {
            auto cf = qk_obs_coeffs(obs_);
            for (int i = 0; i < ret.size(); i++)
            {
                ret[i] = {cf[i].re, cf[i].im};
            }
        }
        return ret;
    }
    reg_t indices(void) const
    {
        if (obs_)
        {
            reg_t ret(qk_obs_len(obs_));
            auto idx = qk_obs_indices(obs_);
            for (int i = 0; i < ret.size(); i++)
            {
                ret[i] = idx[i];
            }
            return ret;
        }
        return reg_t();
    }
    reg_t boundaries(void) const
    {
        if (obs_)
        {
            reg_t ret(num_terms() + 1);
            auto idx = qk_obs_boundaries(obs_);
            for (int i = 0; i < ret.size(); i++)
            {
                ret[i] = idx[i];
            }
            return ret;
        }
        return reg_t();
    }

    SparseObservable operator+(SparseObservable &rhs)
    {
        SparseObservable ret;
        ret.obs_ = qk_obs_add(obs_, rhs.obs_);
        return ret;
    }

    /// @brief Add an observable to this one, in place.
    /// @param rhs the observable to add, which must act on the same number of
    ///   qubits as this one
    /// @return a reference to this observable
    /// @details Adds through ``qk_obs_add_inplace``, which mutates this
    ///   observable rather than building a new one and swapping it in.
    /// @note Adding observables with different qubit counts aborts inside
    ///   Qiskit, as it did before through ``qk_obs_add``.
    SparseObservable &operator+=(const SparseObservable &rhs)
    {
        // The C-API requires both pointers to be non-null, so an unset
        // observable is handled here: adding to nothing copies the right-hand
        // side, and adding nothing leaves this one alone.
        if (!rhs.obs_) {
            return *this;
        }
        if (!obs_) {
            obs_ = qk_obs_copy(rhs.obs_);
            return *this;
        }
        if (obs_ == rhs.obs_) {
            // Doubling itself. The in-place call cannot be given the same
            // observable twice: it takes the left side mutably and the right
            // side immutably, so aliasing them is undefined. Add a copy.
            QkObs *doubled = qk_obs_copy(obs_);
            qk_obs_add_inplace(obs_, doubled);
            qk_obs_free(doubled);
            return *this;
        }
        qk_obs_add_inplace(obs_, rhs.obs_);
        return *this;
    }

    SparseObservable operator*(std::complex<double> &rhs)
    {
        SparseObservable ret;
        ret.obs_ = qk_obs_multiply(obs_, (QkComplex64 *)&rhs);
        return ret;
    }

    /// @brief Scale this observable by a complex coefficient, in place.
    /// @param rhs the coefficient to scale by
    /// @return a reference to this observable
    /// @details Scales through ``qk_obs_multiply_inplace``, which mutates this
    ///   observable rather than building a new one and swapping it in.
    SparseObservable &operator*=(const std::complex<double> &rhs)
    {
        // The C-API requires a non-null observable; scaling an unset one has
        // nothing to do.
        if (!obs_) {
            return *this;
        }
        qk_obs_multiply_inplace(obs_, (const QkComplex64 *)&rhs);
        return *this;
    }

    bool operator==(SparseObservable &rhs)
    {
        return qk_obs_equal(obs_, rhs.obs_);
    }

    SparseObservable compose(SparseObservable &other)
    {
        SparseObservable ret;
        ret.obs_ = qk_obs_compose(obs_, other.obs_);
        return ret;
    }

    /// @brief Compose this observable with another acting on a subset of its
    ///   qubits.
    /// @param other the observable to compose with, which must act on no more
    ///   qubits than this one
    /// @param qargs which qubit of this observable each qubit of ``other``
    ///   maps to: qubit ``i`` of ``other`` acts on qubit ``qargs[i]`` here.
    ///   Must hold exactly ``other.num_qubits()`` entries.
    /// @return ``this.compose(other)``, which equals ``other @ this`` as matrix
    ///   multiplication
    /// @throws std::invalid_argument if either observable holds nothing, if
    ///   ``other`` acts on more qubits than this one, if ``qargs`` is the wrong
    ///   length, or if it maps a qubit out of range or two qubits to the same
    ///   index
    /// @details Unlike ``compose``, which requires both observables to act on
    ///   the same qubits, this composes observables of different sizes, so a
    ///   two-qubit observable can be composed into a hundred-qubit one.
    /// @note Every precondition is checked here because the C-API aborts the
    ///   process rather than reporting an error: ``qk_obs_compose_map`` panics
    ///   if ``other`` is the larger observable, if a mapped index is out of
    ///   bounds, or if two qubits map to the same index.
    SparseObservable compose(const SparseObservable &other, const reg_t &qargs) const
    {
        if (!obs_ || !other.obs_) {
            throw std::invalid_argument(
                "SparseObservable::compose cannot compose an observable that holds nothing.");
        }

        const uint_t num_qubits_here = num_qubits();
        const uint_t num_qubits_other = other.num_qubits();

        if (num_qubits_other > num_qubits_here) {
            std::stringstream message;
            message << "SparseObservable::compose cannot compose an observable on "
                    << num_qubits_other << " qubits into one on " << num_qubits_here
                    << ".";
            throw std::invalid_argument(message.str());
        }

        // The C-API reads exactly one entry per qubit of the other observable,
        // so a short list would read past the end of the array.
        if (qargs.size() != num_qubits_other) {
            std::stringstream message;
            message << "SparseObservable::compose needs " << num_qubits_other
                    << " qubit arguments for an observable on that many qubits, got "
                    << qargs.size() << ".";
            throw std::invalid_argument(message.str());
        }

        std::vector<std::uint32_t> qargs32(qargs.size());
        std::vector<bool> seen(num_qubits_here, false);
        for (uint_t i = 0; i < qargs.size(); i++) {
            if (qargs[i] >= num_qubits_here) {
                std::stringstream message;
                message << "SparseObservable::compose maps qubit " << i << " to "
                        << qargs[i] << ", which is out of range for an observable on "
                        << num_qubits_here << " qubits.";
                throw std::invalid_argument(message.str());
            }
            if (seen[qargs[i]]) {
                std::stringstream message;
                message << "SparseObservable::compose maps more than one qubit to "
                        << qargs[i] << ".";
                throw std::invalid_argument(message.str());
            }
            seen[qargs[i]] = true;
            qargs32[i] = (std::uint32_t)qargs[i];
        }

        SparseObservable ret;
        // qargs32 may be empty when the other observable has no qubits, in
        // which case the C-API reads nothing through the pointer.
        ret.obs_ = qk_obs_compose_map(obs_, other.obs_,
                                      qargs32.empty() ? nullptr : qargs32.data());
        if (ret.obs_ == nullptr) {
            throw std::invalid_argument(
                "SparseObservable::compose could not compose the observables.");
        }
        return ret;
    }

    /// @brief Sum like terms of this observable and drop negligible ones.
    /// @param tol the absolute tolerance below which a summed coefficient is
    ///   treated as zero. Must be finite and non-negative.
    /// @return the simplified observable
    /// @throws std::invalid_argument if this observable holds nothing, or if
    ///   ``tol`` is negative or not finite
    /// @details Matches ``SparseObservable.simplify`` in Qiskit Python, which
    ///   is the same operation as the C-API's ``qk_obs_canonicalize``: terms
    ///   acting on the same qubits with the same operators are summed, a term
    ///   whose summed coefficient has absolute value within ``tol`` of zero is
    ///   removed, and the result is sorted into a canonical order. The
    ///   operation is idempotent.
    /// @note This is what makes two mathematically equal observables comparable
    ///   with ``operator==``, which compares term by term and so reports
    ///   observables built in a different order as unequal. Note the caveat
    ///   Qiskit gives: floating-point rounding and the non-associativity of
    ///   addition mean summed coefficients may still differ slightly, so
    ///   comparing the simplified difference against zero is more robust than
    ///   comparing two simplified observables directly.
    SparseObservable simplify(const double tol = 1e-8) const
    {
        if (!obs_) {
            throw std::invalid_argument(
                "SparseObservable::simplify cannot simplify an observable that holds nothing.");
        }
        // A non-finite tolerance would silently discard every term: the C-API
        // keeps a coefficient when ``norm_sqr() > tol * tol``, and any
        // comparison against NaN is false. A negative tolerance would act as
        // its own magnitude, which is just as surprising.
        if (!std::isfinite(tol) || tol < 0.0) {
            std::stringstream message;
            message << "SparseObservable::simplify tolerance must be finite and "
                       "non-negative, got " << tol << ".";
            throw std::invalid_argument(message.str());
        }

        SparseObservable ret;
        ret.obs_ = qk_obs_canonicalize(obs_, tol);
        if (ret.obs_ == nullptr) {
            throw std::invalid_argument(
                "SparseObservable::simplify could not simplify the observable.");
        }
        return ret;
    }

    std::string to_string(void)
    {
        char *str = qk_obs_str(obs_);
        std::string ret = str;
        qk_str_free(str);
        return ret;
    }

    /// @brief Relabel this observable's qubits, optionally widening it.
    /// @param layout the new index of each qubit: the qubit currently at index
    ///   ``i`` is relabelled to ``layout[i]``. Must hold at least
    ///   ``num_qubits()`` entries; any beyond that are ignored, so a full
    ///   transpiler layout can be passed directly.
    /// @param num_output_qubits the number of qubits the result acts on, which
    ///   may be larger than this observable's to widen it. Zero, the default,
    ///   means ``layout.size()``, which is what a transpiler layout gives.
    /// @return the relabelled observable
    /// @throws std::invalid_argument if this observable holds nothing, if
    ///   ``layout`` is too short, if ``num_output_qubits`` is smaller than this
    ///   observable's qubit count, or if the layout maps a qubit out of range
    ///   or two qubits to the same index
    /// @details Wraps ``qk_obs_apply_layout``. The usual use is applying a
    ///   transpiler layout so that an observable defined on virtual qubits
    ///   lines up with the physical qubits of a transpiled circuit:
    ///   ``observable.apply_layout(transpiled.get_qubit_map())`` widens it to
    ///   the device and moves each term onto its physical qubit.
    /// @note The C-API reads exactly ``num_qubits()`` entries of ``layout``
    ///   regardless of how many are supplied, and takes the output width as a
    ///   separate argument. Passing the layout's length as that width is what
    ///   makes the transpiler case work, which is why it is the default.
    /// @note Qiskit validates every entry of the layout it is given, not only
    ///   the ones it uses, so each entry must be a distinct index below
    ///   ``num_output_qubits``. A transpiler layout satisfies this, being a
    ///   permutation of the device's qubits.
    SparseObservable apply_layout(const reg_t &layout,
                                  const uint_t num_output_qubits = 0) const
    {
        if (!obs_) {
            throw std::invalid_argument(
                "SparseObservable::apply_layout cannot relabel an observable that holds nothing.");
        }

        const uint_t num_qubits_here = num_qubits();
        const uint_t width = num_output_qubits > 0 ? num_output_qubits : layout.size();

        // The C-API reads this many entries, so a shorter layout would read
        // past the end of the array.
        if (layout.size() < num_qubits_here) {
            std::stringstream message;
            message << "SparseObservable::apply_layout needs a layout of at least "
                    << num_qubits_here << " entries for an observable on that many qubits, got "
                    << layout.size() << ".";
            throw std::invalid_argument(message.str());
        }

        if (width < num_qubits_here) {
            std::stringstream message;
            message << "SparseObservable::apply_layout cannot narrow an observable on "
                    << num_qubits_here << " qubits to " << width << ".";
            throw std::invalid_argument(message.str());
        }

        // Only the entries the C-API reads are checked and converted: a
        // transpiler layout is sized to the device, not to the observable.
        std::vector<std::uint32_t> layout32(num_qubits_here);
        std::vector<bool> seen(width, false);
        for (uint_t i = 0; i < num_qubits_here; i++) {
            if (layout[i] >= width) {
                std::stringstream message;
                message << "SparseObservable::apply_layout maps qubit " << i << " to "
                        << layout[i] << ", which is out of range for a result on "
                        << width << " qubits.";
                throw std::invalid_argument(message.str());
            }
            if (seen[layout[i]]) {
                std::stringstream message;
                message << "SparseObservable::apply_layout maps more than one qubit to "
                        << layout[i] << ".";
                throw std::invalid_argument(message.str());
            }
            seen[layout[i]] = true;
            layout32[i] = (std::uint32_t)layout[i];
        }

        SparseObservable ret;
        ret.obs_ = qk_obs_copy(obs_);
        if (ret.obs_ == nullptr) {
            throw std::invalid_argument(
                "SparseObservable::apply_layout could not copy the observable.");
        }

        // An observable on no qubits has nothing to relabel, and layout32
        // would be empty; the C-API treats a null layout as "widen only".
        const QkExitCode code = qk_obs_apply_layout(
            ret.obs_, layout32.empty() ? nullptr : layout32.data(), (std::uint32_t)width);
        if (code != QkExitCode_Success) {
            std::stringstream message;
            message << "SparseObservable::apply_layout could not apply the layout ("
                    << code << ").";
            throw std::invalid_argument(message.str());
        }
        return ret;
    }
};

inline SparseObservable Term::to_observable(void) const
{
    return SparseObservable::from_terms(std::vector<Term>(1, *this), num_qubits_);
}

} // namespace quantum_info
} // namespace Qiskit

#endif //__qiskitcpp_quantum_info_sparse_observable_hpp__
