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

// class for classical register

#ifndef __qiskitcpp_circuit_classical_register_hpp__
#define __qiskitcpp_circuit_classical_register_hpp__

#include "circuit/register.hpp"
#include "qiskit.h"

namespace Qiskit {
namespace circuit {

class Expr;
class Unary;
class Binary;

// ==================================
// class for classical register
// ==================================
/// @class ClassicalRegister
/// @brief Implement a classical register.
class ClassicalRegister : public Register
{
protected:
  QkClassicalRegister* rust_register_ = nullptr;
  static uint_t instances_counter_;
public:
  /// @brief Create a new ClassicalRegister
  ClassicalRegister()
  {
    size_ = 0;
    name_ = prefix();
    rust_register_ = nullptr;
  }
  /// @brief Create a new ClassicalRegister
  /// @param (size) The number of bits to include in the register
  ClassicalRegister(uint_t size) : Register(size)
  {
    name_ = prefix();
    rust_register_ = qk_classical_register_new((std::uint32_t)size, (const char*)name_.c_str());
  }

  /// @brief Create a new ClassicalRegister
  /// @param (size) The number of bits to include in the register
  /// @param (name) The name of the register. If not provided, a unique name will be auto-generated from the register type.
  ClassicalRegister(uint_t size, std::string name) : Register(size, name) {
    rust_register_ = qk_classical_register_new((std::uint32_t)size, (const char*)name.c_str());
  }

  /// @brief Create a new ClassicalRegister as a copy of reg.
  /// @param (reg) copy source
  ClassicalRegister(const ClassicalRegister& reg) : Register(reg) {
    rust_register_ = qk_classical_register_new((std::uint32_t)reg.size_, (const char*)reg.name_.c_str());
  }

  /// @brief Prefix of the register name
  /// @return prefix
  std::string prefix(void) override
  {
    std::string ret = "c";
    ret += std::to_string(instances_counter_++);
    return ret;
  }

  ~ClassicalRegister() {
    if (rust_register_) {
      qk_classical_register_free(rust_register_);
    }
  }

  const QkClassicalRegister* get_register() const {
    return rust_register_;
  }

  // operator overloads for classical expressions
  Unary operator!(void);

  Binary operator==(ClassicalRegister& right);
  Binary operator!=(ClassicalRegister& right);
  Binary operator<(ClassicalRegister& right);
  Binary operator<=(ClassicalRegister& right);
  Binary operator>(ClassicalRegister& right);
  Binary operator>=(ClassicalRegister& right);

  Binary operator==(uint_t right);
  Binary operator!=(uint_t right);
  Binary operator<(uint_t right);
  Binary operator<=(uint_t right);
  Binary operator>(uint_t right);
  Binary operator>=(uint_t right);

  Binary operator==(Expr& right);
  Binary operator!=(Expr& right);
  Binary operator<(Expr& right);
  Binary operator<=(Expr& right);
  Binary operator>(Expr& right);
  Binary operator>=(Expr& right);

};

uint_t ClassicalRegister::instances_counter_ = 0;

} // namespace circuit
} // namespace Qiskit

#endif  // __qiskitcpp_circuit_classical_register_hpp__

