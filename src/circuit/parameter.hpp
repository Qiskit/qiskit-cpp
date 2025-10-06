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

// parameter storage

#ifndef __qiskitcpp_circuit_parameter_hpp__
#define __qiskitcpp_circuit_parameter_hpp__

#include <memory>
#include <complex>

#include "utils/types.hpp"
#include "qiskit.h"

using qiskit_param = ::QkParam;

namespace Qiskit
{
namespace circuit
{

class Parameter
{
    friend class QuantumCircuit;
protected:
    std::shared_ptr<qiskit_param> qiskit_param_ = nullptr;
public:
    /// @brief Create a new Parameter
    Parameter()
    {
        qiskit_param_ = std::shared_ptr<qiskit_param>(qk_param_zero(), qk_param_free);
    }

    /// @brief Create a new Parameter with v
    /// @param v floating point value to initialize the Parameter
    Parameter(double v)
    {
        qiskit_param_ = std::shared_ptr<qiskit_param>(qk_param_from_double(v), qk_param_free);
    }

    /// @brief Create a new Parameter with v
    /// @param v floating point value to initialize the Parameter
    Parameter(std::complex<double> v)
    {
        qiskit_param_ = std::shared_ptr<qiskit_param>(qk_param_from_complex(qk_complex64_from_native(&v)), qk_param_free);
    }

    /// @brief Create a new Parameter with v
    /// @param v integer value to initialize the Parameter
    Parameter(int_t v)
    {
        qiskit_param_ = std::shared_ptr<qiskit_param>(qk_param_from_double((double)v), qk_param_free);
    }

    /// @brief Create a new symbol
    /// @param name a name of symbol
    Parameter(std::string name)
    {
        qiskit_param_ = std::shared_ptr<qiskit_param>(qk_param_new_symbol((char *)name.c_str()), qk_param_free);
    }

    /// @brief Create a new Parameter from other Parameter
    /// @param prm source Parameter
    Parameter(const Parameter &prm)
    {
        // copying reference
        qiskit_param_ = prm.qiskit_param_;
    }

    /// @brief Create a new Parameter from QkParam
    /// @param prm source Parameter
    Parameter(qiskit_param* prm)
    {
        qiskit_param_ = std::shared_ptr<qiskit_param>(prm, qk_param_free);
    }

    ~Parameter()
    {
        if (qiskit_param_)
            qiskit_param_.reset();
    }

    /// @brief get string expression of the Parameter
	/// @return a string expression of the Parameter
    std::string as_str(void)
    {
        char* str = qk_param_str(qiskit_param_.get());
        std::string ret = str;
        qk_str_free(str);
        return ret;
    }

    /// @brief get value of the Parameter expression
    /// @param out a reference to the returned value
	/// @return true if real number can be obtained
    double as_real(void)
    {
        return qk_param_as_real(qiskit_param_.get());
    }

    /// @brief Negate a Parameter
	/// @return a new Parameter (-this)
    Parameter operator-()
    {
        Parameter ret;
        qk_param_neg(ret.qiskit_param_.get(), qiskit_param_.get());
        return ret;
    }

    /// @brief add parameters
    /// @param prm a Parameter to be added
	/// @return a new Parameter (this + prm)
    Parameter operator+(const Parameter &prm)
    {
        Parameter ret;
        qk_param_add(ret.qiskit_param_.get(), qiskit_param_.get(), prm.qiskit_param_.get());
        return ret;
    }

    /// @brief subtract parameters
    /// @param prm a Parameter to be subtracted
	/// @return a new Parameter (this - prm)
    Parameter operator-(const Parameter &prm)
    {
        Parameter ret;
        qk_param_sub(ret.qiskit_param_.get(), qiskit_param_.get(), prm.qiskit_param_.get());
        return ret;
    }

    /// @brief multiply parameters
    /// @param prm a Parameter to be multiplied
	/// @return a new Parameter (this * prm)
    Parameter operator*(const Parameter &prm)
    {
        Parameter ret;
        qk_param_mul(ret.qiskit_param_.get(), qiskit_param_.get(), prm.qiskit_param_.get());
        return ret;
    }

    /// @brief divide by a Parameter
    /// @param prm a Parameter to be divided by
	/// @return a new Parameter (this / prm)
    Parameter operator/(const Parameter &prm)
    {
        Parameter ret;
        qk_param_div(ret.qiskit_param_.get(), qiskit_param_.get(), prm.qiskit_param_.get());
        return ret;
    }

    /// @brief add a value
    /// @param prm a floating point value to be added
	/// @return a new Parameter (this + prm)
    Parameter operator+(const double prm)
    {
        Parameter ret;
        Parameter rhs(prm);
        qk_param_add(ret.qiskit_param_.get(), qiskit_param_.get(), rhs.qiskit_param_.get());
        return ret;
    }

    /// @brief subtract a value
    /// @param prm a floating point value to be subtracted
	/// @return a new Parameter (this - prm)
    Parameter operator-(const double prm)
    {
        Parameter ret;
        Parameter rhs(prm);
        qk_param_sub(ret.qiskit_param_.get(), qiskit_param_.get(), rhs.qiskit_param_.get());
        return ret;
    }

    /// @brief multiply a value
    /// @param prm a floating point value to be multiplied
	/// @return a new Parameter (this * prm)
    Parameter operator*(const double prm)
    {
        Parameter ret;
        Parameter rhs(prm);
        qk_param_mul(ret.qiskit_param_.get(), qiskit_param_.get(), rhs.qiskit_param_.get());
        return ret;
    }

    /// @brief divide by a value
    /// @param prm a floating point value to be divided by
	/// @return a new Parameter (this / prm)
    Parameter operator/(const double prm)
    {
        Parameter ret;
        Parameter rhs(prm);
        qk_param_div(ret.qiskit_param_.get(), qiskit_param_.get(), rhs.qiskit_param_.get());
        return ret;
    }

    /// @brief calculate this Parameter raised to the power other Parameter
    /// @param prm an exponent Parameter
	/// @return a new Parameter (this ^ prm)
    Parameter pow(const Parameter &prm)
    {
        Parameter ret;
        qk_param_pow(ret.qiskit_param_.get(), qiskit_param_.get(), prm.qiskit_param_.get());
        return ret;
    }

    /// @brief calculate this Parameter raised to the power a value
    /// @param prm an exponent value
	/// @return a new Parameter (this ^ prm)
    Parameter pow(const double prm)
    {
        Parameter ret;
        Parameter rhs(prm);
        qk_param_pow(ret.qiskit_param_.get(), qiskit_param_.get(), rhs.qiskit_param_.get());
        return ret;
    }

    /// @brief Copy parameter from other
    /// @param prm source Parameter
    Parameter &operator=(const Parameter &prm)
    {
        if (qiskit_param_)
            qiskit_param_.reset();
        qiskit_param_ = std::shared_ptr<qiskit_param>(qk_param_copy(prm.qiskit_param_.get()), qk_param_free);
        return *this;
    }

    /// @brief add other Parameter to this Parameter
    /// @param rhs a Parameter to be added
	/// @return a reference to this
    Parameter &operator+=(const Parameter &rhs)
    {
        qk_param_add(qiskit_param_.get(), qiskit_param_.get(), rhs.qiskit_param_.get());
        return *this;
    }

    /// @brief add a value to this Parameter
    /// @param rhs a value to be added
	/// @return a reference to this
    Parameter &operator+=(const double rhs)
    {
        qiskit_param* rval = qk_param_from_double(rhs);
        qk_param_add(qiskit_param_.get(), qiskit_param_.get(), rval);
        qk_param_free(rval);
        return *this;
    }

    /// @brief subtract this Parameter and other Parameter
    /// @param rhs a Parameter to be subtracted
	/// @return a reference to this
    Parameter &operator-=(const Parameter &rhs)
    {
        qk_param_sub(qiskit_param_.get(), qiskit_param_.get(), rhs.qiskit_param_.get());
        return *this;
    }

    /// @brief subtract this Parameter and a value
    /// @param rhs a value to be subtracted
	/// @return a reference to this
    Parameter &operator-=(const double rhs)
    {
        qiskit_param* rval = qk_param_from_double(rhs);
        qk_param_sub(qiskit_param_.get(), qiskit_param_.get(), rval);
        qk_param_free(rval);
        return *this;
    }

    /// @brief multiply other Parameter to this Parameter
    /// @param rhs a Parameter to be multiplied
	/// @return a reference to this
    Parameter &operator*=(const Parameter &rhs)
    {
        qk_param_mul(qiskit_param_.get(), qiskit_param_.get(), rhs.qiskit_param_.get());
        return *this;
    }

    /// @brief multiply a value to this Parameter
    /// @param rhs a value to be multiplied
	/// @return a reference to this
    Parameter &operator*=(const double rhs)
    {
        qiskit_param* rval = qk_param_from_double(rhs);
        qk_param_mul(qiskit_param_.get(), qiskit_param_.get(), rval);
        qk_param_free(rval);
        return *this;
    }

    /// @brief divide this Parameter by other Parameter
    /// @param rhs a Parameter to be divided by
	/// @return a reference to this
    Parameter &operator/=(const Parameter &rhs)
    {
        qk_param_div(qiskit_param_.get(), qiskit_param_.get(), rhs.qiskit_param_.get());
        return *this;
    }

    /// @brief divide this Parameter by a value
    /// @param rhs a value to be divided by
	/// @return a reference to this
    Parameter &operator/=(const double rhs)
    {
        qiskit_param* rval = qk_param_from_double(rhs);
        qk_param_div(qiskit_param_.get(), qiskit_param_.get(), rval);
        qk_param_free(rval);
        return *this;
    }

    /// @brief compare 2 Parameters
    /// @param rhs a Parameter to be compared
	/// @return true if 2 Parameters are equal
    bool operator==(const Parameter &rhs) const
    {
        return qk_param_equal(qiskit_param_.get(), rhs.qiskit_param_.get());
    }

    /// @brief compare 2 Parameters
    /// @param rhs a Parameter to be compared
	/// @return true if 2 Parameters are equal
    bool operator!=(const Parameter &rhs) const
    {
        return !qk_param_equal(qiskit_param_.get(), rhs.qiskit_param_.get());
    }

    /// @brief calculate exponent of this Parameter
	/// @return a new Parameter for the result
    Parameter exp(void)
    {
        Parameter ret;
        qk_param_exp(ret.qiskit_param_.get(), qiskit_param_.get());
        return ret;
    }

    /// @brief calculate log of this Parameter
	/// @return a new Parameter for the result
    Parameter log(void)
    {
        Parameter ret;
        qk_param_log(ret.qiskit_param_.get(), qiskit_param_.get());
        return ret;
    }

    /// @brief calculate absolute of this Parameter
	/// @return a new Parameter for the result
    Parameter abs(void)
    {
        Parameter ret;
        qk_param_abs(ret.qiskit_param_.get(), qiskit_param_.get());
        return ret;
    }

    /// @brief calculate sine of this Parameter
	/// @return a new Parameter for the result
    Parameter sin(void)
    {
        Parameter ret;
        qk_param_sin(ret.qiskit_param_.get(), qiskit_param_.get());
        return ret;
    }

    /// @brief calculate cosine of this Parameter
	/// @return a new Parameter for the result
    Parameter cos(void)
    {
        Parameter ret;
        qk_param_cos(ret.qiskit_param_.get(), qiskit_param_.get());
        return ret;
    }

    /// @brief calculate tangent of this Parameter
	/// @return a new Parameter for the result
    Parameter tan(void)
    {
        Parameter ret;
        qk_param_tan(ret.qiskit_param_.get(), qiskit_param_.get());
        return ret;
    }

    /// @brief calculate arcsine of this Parameter
	/// @return a new Parameter for the result
    Parameter asin(void)
    {
        Parameter ret;
        qk_param_asin(ret.qiskit_param_.get(), qiskit_param_.get());
        return ret;
    }

    /// @brief calculate arccosine of this Parameter
	/// @return a new Parameter for the result
    Parameter acos(void)
    {
        Parameter ret;
        qk_param_acos(ret.qiskit_param_.get(), qiskit_param_.get());
        return ret;
    }

    /// @brief calculate arctangent of this Parameter
	/// @return a new Parameter for the result
    Parameter atan(void)
    {
        Parameter ret;
        qk_param_atan(ret.qiskit_param_.get(), qiskit_param_.get());
        return ret;
    }

    /// @brief calculate sign of this Parameter
	/// @return a new Parameter for the result
    Parameter sign(void)
    {
        Parameter ret;
        qk_param_sign(ret.qiskit_param_.get(), qiskit_param_.get());
        return ret;
    }

    /// @brief calculate conjugate of this Parameter
	/// @return a new Parameter for the result
    Parameter conjugate(void)
    {
        Parameter ret;
        qk_param_conjugate(ret.qiskit_param_.get(), qiskit_param_.get());
        return ret;
    }

#ifdef QISKIT_CAPI_HAS_SUBS
    /// @brief bind a value to a symbol
    /// @param symbol a symbol to be bound
    /// @param value a value to be set
	/// @return a new bound Parameter
    Parameter bind(const Parameter& symbol, const double value);

    /// @brief bind values to symbols
    /// @param symbols a list symbols to be bound
    /// @param value a list of values to be set
	/// @return a new bound Parameter
    Parameter bind(const std::vector<Parameter>& symbols, const std::vector<double> values);

    /// @brief substitute a symbol to other symbol
    /// @param symbol a symbol to be bound
    /// @param other a symbol to be set
	/// @return a new substituted Parameter
    Parameter subs(const Parameter& symbol, const Parameter& other);

    /// @brief substitute symbols to other symbols
    /// @param symbols a list of symbols to be bound
    /// @param others a list of symbols to be set
	/// @return a new substituted Parameter
    Parameter subs(const std::vector<Parameter>& symbols, const std::vector<Parameter>& others);
#endif
};

#ifdef QISKIT_CAPI_HAS_SUBS
Parameter Parameter::bind(const Parameter& symbol, const double value)
{
    Parameter ret;

    qk_param_bind(ret.qiskit_param_.get(), qiskit_param_.get(), &symbol.qiskit_param_.get(), &value, 1);
    return ret;
}

Parameter Parameter::bind(const std::vector<Parameter>& symbols, const std::vector<double> values)
{
    size_t size = std::min(symbols.size(), values.size());
    Parameter ret;
    std::vector<qiskit_param*> list(size);
    for (uint_t i = 0; i < size; i++) {
        list[i] = symbols[i].qiskit_param_.get();
    }

    qk_param_bind(ret.qiskit_param_.get(), qiskit_param_.get(), list.data(), values.data(), size);
    return ret;

}

Parameter Parameter::subs(const Parameter& symbol, const Parameter& other)
{
    Parameter ret;

    qk_param_subs(ret.qiskit_param_.get(), qiskit_param_.get(), &symbol.qiskit_param_.get(), &other.qiskit_param_.get(), 1);
    return ret;
}

Parameter Parameter::subs(const std::vector<Parameter>& symbol, const std::vector<Parameter>& others)
{
    size_t size = std::min(symbols.size(), values.size());
    Parameter ret;
    std::vector<qiskit_param*> slist(size);
    std::vector<qiskit_param*> olist(size);
    for (uint_t i = 0; i < size; i++) {
        slist[i] = symbols[i].qiskit_param_.get();
        olist[i] = others[i].qiskit_param_.get();
    }

    qk_param_bind(ret.qiskit_param_.get(), qiskit_param_.get(), slist.data(), olist.data(), size);
    return ret;
}
#endif



} // namespace circuit
} // namespace Qiskit

#endif // __qiskitcpp_circuit_parameter_hpp__
