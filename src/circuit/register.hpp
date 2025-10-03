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

// base class for registers

#ifndef __qiskitcpp_circuit_register_hpp__
#define __qiskitcpp_circuit_register_hpp__

#include "utils/types.hpp"

namespace Qiskit
{
namespace circuit
{

class Register;

// =======================
// class for bit
// =======================
/// @class Bit
/// @brief Implement a generic bit
class Bit
{
    friend Register;

protected:
    uint32_t index_;
    Register *register_ = nullptr;

public:
    /// @brief Create a new generic bit
    Bit()
    {
        index_ = 0;
    }

    /// @brief Create a new generic bit
    /// @param (idx) the index of the bit in its containing register
    /// @param (reg) a register containing the bit
    Bit(uint32_t idx, Register *reg = nullptr)
    {
        index_ = idx;
        register_ = reg;
    }

    /// @brief Create a new generic bit as a copy of bit.
    /// @param (bit) copy source
    Bit(const Bit &bit)
    {
        index_ = bit.index_;
        register_ = bit.register_;
    }

    /// @brief Return the index of the bit in its containing register.
    /// @return index value
    uint32_t index(void)
    {
        return index_;
    }
    uint32_t global_index(void);

    /// @brief Return the register in this bit in its containing register.
    /// @return reference to Register
    Register *get_register(void)
    {
        return register_;
    }

    // return bit index
    operator uint32_t() { return global_index(); }
};

// use same Bit class for both quantum and classical
using Qubit = Bit;
using Clbit = Bit;

// =======================
// base class for register
// =======================
/// @class Register
/// @brief Implement a generic register
class Register
{
    friend Bit;

protected:
    uint_t size_;
    std::string name_;
    std::vector<Bit> bits_;
    uint_t base_index_ = 0;

public:
    /// @brief Create a new generic register
    Register()
    {
    }
    /// @brief Create a new generic register
    /// @param (size) The number of bits to include in the register
    Register(uint_t size) : size_(size)
    {
        allocate_bits();
    }
    /// @brief Create a new generic register
    /// @param (size) The number of bits to include in the register
    /// @param (name) The name of the register. If not provided, a unique name will be auto-generated from the register type.
    Register(uint_t size, std::string name) : size_(size), name_(name)
    {
        allocate_bits();
    }
    /// @brief Create a new generic register
    /// @param (size) The number of bits to include in the register
    /// @param (name) The name of the register. If not provided, a unique name will be auto-generated from the register type.
    /// @param (bits) A list of Bit instances to be used to populate the register.
    Register(uint_t size, std::string name, std::vector<Bit> &bits);

    /// @brief Create a new generic register as a copy of reg
    /// @param (reg) copy source
    Register(const Register &reg);

    /// @brief Resize this register with size
    /// @param (size) new size of this register
    void resize(uint_t size)
    {
        size_ = size;
        allocate_bits();
    }

    /// @brief Make this register as 1 bit register
    void make_one_bit_register(Bit &bit)
    {
        resize(1);
        bits_[0] = bit;
    }

    /// @brief Return the size of this register
    /// @return the size of this register
    uint_t size(void) { return size_; }

    /// @brief Return the name of this register
    /// @return the name of this register
    const std::string& name() const { return name_; }

    void set_base_index(uint_t base)
    {
        base_index_ = base;
    }

    Bit &operator[](const uint_t i) { return bits_[i]; }

protected:
    virtual std::string prefix(void) = 0;

    void allocate_bits(void);
};

// =======================
// class for bit
// =======================
uint32_t Bit::global_index(void)
{
    if (register_)
        return (uint32_t)register_->base_index_ + index_;
    return index_;
}

// =======================
// base class for register
// =======================
Register::Register(uint_t size, std::string name, std::vector<Bit> &bits)
{
    if (size != bits.size())
    {
        throw std::runtime_error("Register : size of bits and size is different");
    }

    size_ = size;
    bits_ = bits;

    for (uint_t i = 0; i < size; i++)
    {
        bits[i].register_ = this;
        bits[i].index_ = (uint32_t)i;
        bits_[i].register_ = this;
        bits_[i].index_ = (uint32_t)i;
    }
}

Register::Register(const Register &reg)
{
    size_ = reg.size_;
    name_ = reg.name_;
    bits_ = reg.bits_;
    base_index_ = reg.base_index_;
}

void Register::allocate_bits(void)
{
    bits_.resize(size_);
    for (uint_t i = 0; i < size_; i++)
    {
        bits_[i].register_ = this;
        bits_[i].index_ = (uint32_t)i;
    }
}

} // namespace circuit
} // namespace Qiskit

#endif // __qiskitcpp_circuit_register_hpp__
