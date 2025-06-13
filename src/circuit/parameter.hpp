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

// this macro replaces Complex64 used in rust FFI
#define Complex64 std::complex<double>

// We will enable Parameter if C-API will support ParameterExpression
#ifdef ENABLE_QISKIT_PARAMETER
#include "qiskit.h"
using rust_parameter = ::QkParameterExpression;
#else
using rust_parameter = void*;
#endif

namespace qiskitcpp {
namespace circuit {


#ifdef ENABLE_QISKIT_PARAMETER
class Parameter
{
  friend class QuantumCircuit;
protected:
  rust_parameter* expr_ = nullptr;
public:
  /// @brief Create a new Parameter
  Parameter()
  {
    expr_ = qk_parameter_from_int(0);
  }

  /// @brief Create a new Parameter with v
  /// @param (v)
  Parameter(double v)
  {
    expr_ = qk_parameter_from_real(v);
  }

  /// @brief Create a new Parameter with v
  /// @param (v)
  Parameter(int_t v)
  {
    expr_ = qk_parameter_from_int(v);
  }

  /// @brief Create a new symbol
  /// @param (expr)
  Parameter(std::string name)
  {
    expr_ = qk_parameter_symbol((char*)name.c_str());
  }

  /// @brief Create a new Parameter as a copy of prm
  /// @param (prm) copy source
  Parameter(const Parameter& prm)
  {
    expr_ = qk_parameter_copy(prm.expr_);
  }

  /// @brief Create a new Parameter with expr
  /// @param (expr)
  Parameter(rust_parameter* expr)
  {
    expr_ = expr;
  }

  ~Parameter()
  {
    qk_parameter_free(expr_);
  }

  std::string as_str(void)
  {
    return std::string(qk_parameter_to_string(expr_));
  }


  Parameter operator-();

  Parameter operator+(Parameter& prm);
  Parameter operator-(Parameter& prm);
  Parameter operator*(Parameter& prm);
  Parameter operator/(Parameter& prm);

  Parameter operator+(double prm);
  Parameter operator-(double prm);
  Parameter operator*(double prm);
  Parameter operator/(double prm);

  Parameter& operator=(Parameter& prm)
  {
//    qc_param_assign(expr_, prm.expr_);
    return *this;
  }

  Parameter& operator+=(Parameter& rhs)
  {
    rust_parameter* t = qk_parameter_add(expr_, rhs.expr_);
    qk_parameter_free(expr_);
    expr_ = t;
    return *this;
  }

  Parameter& operator-=(Parameter& rhs)
  {
    rust_parameter* t = qk_parameter_sub(expr_, rhs.expr_);
    qk_parameter_free(expr_);
    expr_ = t;
    return *this;
  }

  Parameter& operator*=(Parameter& rhs)
  {
    rust_parameter* t = qk_parameter_mul(expr_, rhs.expr_);
    qk_parameter_free(expr_);
    expr_ = t;
    return *this;
  }

  Parameter& operator/=(Parameter& rhs)
  {
    rust_parameter* t = qk_parameter_div(expr_, rhs.expr_);
    qk_parameter_free(expr_);
    expr_ = t;
    return *this;
  }

  Parameter& pow(Parameter& prm)
  {
    rust_parameter* t = qk_parameter_pow(expr_, prm.expr_);
    qk_parameter_free(expr_);
    expr_ = t;
    return *this;
  }

  Parameter exp(void)
  {
    Parameter ret(qk_parameter_exp(expr_));
    return ret;
  }

  Parameter log(void)
  {
    Parameter ret(qk_parameter_log(expr_));
    return ret;
  }

  Parameter abs(void)
  {
    Parameter ret(qk_parameter_abs(expr_));
    return ret;
  }

  Parameter sin(void)
  {
    Parameter ret(qk_parameter_sin(expr_));
    return ret;
  }

  Parameter cos(void)
  {
    Parameter ret(qk_parameter_cos(expr_));
    return ret;
  }

  Parameter tan(void)
  {
    Parameter ret(qk_parameter_tan(expr_));
    return ret;
  }

  Parameter asin(void)
  {
    Parameter ret(qk_parameter_asin(expr_));
    return ret;
  }

  Parameter acos(void)
  {
    Parameter ret(qk_parameter_acos(expr_));
    return ret;
  }

  Parameter atan(void)
  {
    Parameter ret(qk_parameter_atan(expr_));
    return ret;
  }

};

Parameter Parameter::operator-()
{
  Parameter ret(qk_parameter_neg(expr_));
  return ret;
}

Parameter Parameter::operator+(Parameter& rhs)
{
  Parameter ret(qk_parameter_add(expr_, rhs.expr_));
  return ret;
}

Parameter Parameter::operator-(Parameter& rhs)
{
  Parameter ret(qk_parameter_sub(expr_, rhs.expr_));
  return ret;
}

Parameter Parameter::operator*(Parameter& rhs)
{
  Parameter ret(qk_parameter_mul(expr_, rhs.expr_));
  return ret;
}

Parameter Parameter::operator/(Parameter& rhs)
{
  Parameter ret(qk_parameter_div(expr_, rhs.expr_));
  return ret;
}

Parameter Parameter::operator+(double prm)
{
  Parameter t(prm);
  Parameter ret(qk_parameter_add(expr_, t.expr_));
  return ret;
}

Parameter Parameter::operator-(double prm)
{
  Parameter t(prm);
  Parameter ret(qk_parameter_sub(expr_, t.expr_));
  return ret;
}

Parameter Parameter::operator*(double prm)
{
  Parameter t(prm);
  Parameter ret(qk_parameter_mul(expr_, t.expr_));
  return ret;
}

Parameter Parameter::operator/(double prm)
{
  Parameter t(prm);
  Parameter ret(qk_parameter_div(expr_, t.expr_));
  return ret;
}
#else
class Parameter
{
  friend class QuantumCircuit;
protected:
  rust_parameter* expr_ = nullptr;
public:
  /// @brief Create a new Parameter
  Parameter()
  {
  }

  /// @brief Create a new Parameter with v
  /// @param (v)
  Parameter(double v)
  {
  }

  /// @brief Create a new Parameter with v
  /// @param (v)
  Parameter(int_t v)
  {
  }

  /// @brief Create a new symbol
  /// @param (expr)
  Parameter(std::string name)
  {
  }

  /// @brief Create a new Parameter as a copy of prm
  /// @param (prm) copy source
  Parameter(const Parameter& prm)
  {
  }

  /// @brief Create a new Parameter with expr
  /// @param (expr)
  Parameter(rust_parameter* expr)
  {
    expr_ = expr;
  }

  ~Parameter()
  {
  }

  /*
  std::string as_str(void)
  {
  }
  */


  /*
  Parameter operator-();

  Parameter operator+(Parameter& prm);
  Parameter operator-(Parameter& prm);
  Parameter operator*(Parameter& prm);
  Parameter operator/(Parameter& prm);

  Parameter operator+(double prm);
  Parameter operator-(double prm);
  Parameter operator*(double prm);
  Parameter operator/(double prm);
  */

  Parameter& operator=(Parameter& prm)
  {
    return *this;
  }

  Parameter& operator+=(Parameter& rhs)
  {
    return *this;
  }

  Parameter& operator-=(Parameter& rhs)
  {
    return *this;
  }

  Parameter& operator*=(Parameter& rhs)
  {
    return *this;
  }

  Parameter& operator/=(Parameter& rhs)
  {
    return *this;
  }

  Parameter& pow(Parameter& prm)
  {
    return *this;
  }
};

#endif



} // namespace circuit
} // namespace qiskitcpp

#endif  // __qiskitcpp_circuit_parameter_expression_hpp__


