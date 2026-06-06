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

#include "qiskit.h"
#include "utils/types.hpp"
#include <limits>
#include <stdexcept>

namespace Qiskit
{
namespace quantum_info
{

class SparseObservable;

class SparseObservable
{
protected:
    QkObs *obs_;

public:
    SparseObservable(void)
    {
        obs_ = nullptr;
    }
    SparseObservable(uint_t num_qubits,
                     std::vector<std::complex<double>> &coeffs,
                     std::vector<QkBitTerm> &bits,
                     reg_t &indices,
                     std::vector<size_t> &boundaries)
    {
        if (num_qubits > std::numeric_limits<std::uint32_t>::max()) {
            throw std::invalid_argument("SparseObservable: number of qubits exceeds QkObs limit");
        }
        if (bits.size() != indices.size()) {
            throw std::invalid_argument("SparseObservable: bits/indices size mismatch");
        }
        if (boundaries.size() != coeffs.size() + 1
            || boundaries.empty()
            || boundaries[0] != 0
            || boundaries.back() != bits.size()) {
            throw std::invalid_argument("SparseObservable: invalid boundaries");
        }
        for (uint_t i = 1; i < boundaries.size(); i++) {
            if (boundaries[i] < boundaries[i - 1] || boundaries[i] > bits.size()) {
                throw std::invalid_argument("SparseObservable: invalid boundaries");
            }
        }
        for (uint_t term = 0; term < coeffs.size(); term++) {
            for (size_t pos = boundaries[term] + 1; pos < boundaries[term + 1]; pos++) {
                if (indices[pos] <= indices[pos - 1]) {
                    throw std::invalid_argument("SparseObservable: term indices are not strictly increasing");
                }
            }
        }
        std::vector<std::uint32_t> idx32(indices.size());
        for (uint_t i = 0; i < indices.size(); i++) {
            if (indices[i] >= num_qubits || indices[i] > std::numeric_limits<std::uint32_t>::max()) {
                throw std::invalid_argument("SparseObservable: index exceeds observable width");
            }
            idx32[i] = (std::uint32_t)indices[i];
        }
        obs_ = qk_obs_new((std::uint32_t)num_qubits,
                          coeffs.size(),
                          bits.size(),
                          (QkComplex64 *)coeffs.data(),
                          bits.data(),
                          idx32.data(),
                          boundaries.data());
    }

    SparseObservable(const SparseObservable &other)
    {
        obs_ = other.obs_ ? qk_obs_copy(other.obs_) : nullptr;
    }

    ~SparseObservable()
    {
        if (obs_)
        {
            qk_obs_free(obs_);
        }
    }

    SparseObservable &operator=(const SparseObservable &other)
    {
        if (this == &other) {
            return *this;
        }
        if (obs_) {
            qk_obs_free(obs_);
            obs_ = nullptr;
        }
        obs_ = other.obs_ ? qk_obs_copy(other.obs_) : nullptr;
        return *this;
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

    static SparseObservable from_label(std::string &label)
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

    static SparseObservable from_list(std::vector<std::pair<std::string, std::complex<double>>> &list, uint_t num_qubits_in = 0)
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
        std::vector<QkBitTerm> ret(obs_ ? qk_obs_len(obs_) : 0);
        if (obs_)
        {
            auto terms = qk_obs_bit_terms(obs_);
            for (int i = 0; i < ret.size(); i++)
            {
                ret[i] = terms[i];
            }
        }
        return ret;
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
        reg_t ret(obs_ ? qk_obs_len(obs_) : 0);
        if (obs_)
        {
            auto idx = qk_obs_indices(obs_);
            for (int i = 0; i < ret.size(); i++)
            {
                ret[i] = idx[i];
            }
        }
        return ret;
    }
    reg_t boundaries(void) const
    {
        reg_t ret(obs_ ? qk_obs_num_terms(obs_) + 1 : 0);
        if (obs_)
        {
            auto idx = qk_obs_boundaries(obs_);
            for (int i = 0; i < ret.size(); i++)
            {
                ret[i] = idx[i];
            }
        }
        return ret;
    }

    SparseObservable operator+(SparseObservable &rhs)
    {
        SparseObservable ret;
        ret.obs_ = qk_obs_add(obs_, rhs.obs_);
        return ret;
    }

    SparseObservable &operator+=(SparseObservable &rhs)
    {
        QkObs *t = qk_obs_add(obs_, rhs.obs_);
        qk_obs_free(obs_);
        obs_ = t;
        return *this;
    }

    SparseObservable operator*(std::complex<double> &rhs)
    {
        SparseObservable ret;
        ret.obs_ = qk_obs_multiply(obs_, (QkComplex64 *)&rhs);
        return ret;
    }

    SparseObservable &operator*=(std::complex<double> &rhs)
    {
        QkObs *t = qk_obs_multiply(obs_, (QkComplex64 *)&rhs);
        qk_obs_free(obs_);
        obs_ = t;
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

    std::string to_string(void)
    {
        char *str = qk_obs_str(obs_);
        std::string ret = str;
        qk_str_free(str);
        return ret;
    }
};

} // namespace quantum_info
} // namespace Qiskit

#endif //__qiskitcpp_quantum_info_sparse_observable_hpp__
