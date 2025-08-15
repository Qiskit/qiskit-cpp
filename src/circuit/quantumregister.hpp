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

// class for quantum register

#ifndef __qiskitcpp_circuit_quantum_register_hpp__
#define __qiskitcpp_circuit_quantum_register_hpp__

#include "circuit/register.hpp"
#include "qiskit.h"

namespace Qiskit {
namespace circuit {

// ==================================
// class for quantum register
// ==================================
/// @class QuantumRegister
/// @brief Implement a quantum register.
class QuantumRegister : public Register
{
protected:
  std::shared_ptr<QkQuantumRegister> rust_register_ = nullptr;
  static uint_t instances_counter_;
public:
  /// @brief Create a new generic register
  QuantumRegister()
  {
    size_ = 0;
    name_ = prefix();
    rust_register_ = nullptr;
  }
  /// @brief Create a new QuantumRegister
  /// @param (size) The number of bits to include in the register
  QuantumRegister(uint_t size) : Register(size)
  {
    name_ = prefix();
    std::shared_ptr<QkQuantumRegister> reg(qk_quantum_register_new((std::uint32_t)size, (const char*)name_.c_str()), qk_quantum_register_free);
    rust_register_ = reg;
  }

  /// @brief Create a new QuantumRegister
  /// @param (size) The number of bits to include in the register
  /// @param (name) The name of the register. If not provided, a unique name will be auto-generated from the register type
  QuantumRegister(uint_t size, std::string name) : Register(size, name) {
    std::shared_ptr<QkQuantumRegister> reg(qk_quantum_register_new((std::uint32_t)size, (const char*)name.c_str()), qk_quantum_register_free);
    rust_register_ = reg;
  }

  /// @brief Create a new QuantumRegister as a copy of reg.
  /// @param (reg) copy source
  QuantumRegister(const QuantumRegister& reg) : Register(reg) {
    rust_register_ = reg.rust_register_;
  }

  ~QuantumRegister() {
    if (rust_register_) {
      rust_register_.reset();
    }
  }

  const std::shared_ptr<QkQuantumRegister>& get_register() const {
    return rust_register_;
  }

  std::string prefix(void) override
  {
    std::string ret = "q";
    ret += std::to_string(instances_counter_++);
    return ret;
  }

};

uint_t QuantumRegister::instances_counter_ = 0;

} // namespace circuit
} // namespace Qiskit

#endif  // __qiskitcpp_circuit_quantum_register_hpp__

