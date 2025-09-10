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

// classical expression classes

#ifndef __qiskitcpp_circuit_classical_expr_hpp__
#define __qiskitcpp_circuit_classical_expr_hpp__

#include "circuit/classicalregister.hpp"

namespace Qiskit
{
namespace circuit
{

// operation types
enum class OpType
{
    NONE,
    BIT_NOT,
    LOGIC_NOT,
    BIT_AND,
    BIT_OR,
    BIT_XOR,
    LOGIC_AND,
    LOGIC_OR,
    EQUAL,
    NOT_EQUAL,
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL,
    SHIFT_LEFT,
    SHIFT_RIGHT,
};

// ==================================
// base class for expressions
// ==================================
/// @class Expr
/// @brief Root base of all nodes in the expression tree.
class Expr
{
protected:
public:
    Expr() {}

    // operator overloads
    Unary operator!(void);

    Binary operator==(uint_t right);
    Binary operator!=(uint_t right);
    Binary operator<(uint_t right);
    Binary operator<=(uint_t right);
    Binary operator>(uint_t right);
    Binary operator>=(uint_t right);

    Binary operator==(ClassicalRegister &right);
    Binary operator!=(ClassicalRegister &right);
    Binary operator<=(ClassicalRegister &right);
    Binary operator<(ClassicalRegister &right);
    Binary operator>=(ClassicalRegister &right);
    Binary operator>(ClassicalRegister &right);

    Binary operator==(Expr &right);
    Binary operator!=(Expr &right);
    Binary operator<=(Expr &right);
    Binary operator<(Expr &right);
    Binary operator>=(Expr &right);
    Binary operator>(Expr &right);

    Binary operator||(Expr &right);
    Binary operator&&(Expr &right);

    virtual std::string type(void)
    {
        return std::string("expr");
    }
    virtual uint_t value(void)
    {
        return 0;
    }
    virtual ClassicalRegister creg(void)
    {
        ClassicalRegister cr(0, "dummy");
        return cr;
    }
    virtual OpType Op(void)
    {
        return OpType::NONE;
    }
};

// ==================================
// variables
// ==================================
/// @class Var
/// @brief A classical variable
class Var : public Expr
{
protected:
    ClassicalRegister register_;

public:
    /// @brief Create a new classical variable
    Var() {}

    /// @brief Create a new classical variable
    /// @param (bit)
    Var(Clbit &bit)
    {
        register_.make_one_bit_register(bit);
    }
    /// @brief Create a new classical variable
    /// @param (cr)
    Var(ClassicalRegister &cr) : register_(cr) {}

    /// @brief Create a new classical variable as a copy of var
    /// @param (var) copy source
    Var(const Var &var)
    {
        register_ = var.register_;
    }

    /// @brief Return the type string ("var")
    std::string type(void) override
    {
        return std::string("var");
    }
    ClassicalRegister creg(void) override
    {
        return register_;
    }
};

// ==================================
// value
// ==================================
/// @class Value
/// @brief A single scalar value
class Value : public Expr
{
protected:
    uint_t value_;

public:
    /// @brief Create a new single scalar value
    Value() { value_ = 0; }

    /// @brief Create a new single scalar value
    /// @param (v) scalar value
    Value(uint_t v)
    {
        value_ = v;
    }
    /// @brief Create a new single scalar value as a copy of val
    /// @param (v) copy source
    Value(const Value &val)
    {
        value_ = val.value_;
    }

    /// @brief Return the type string ("value)
    std::string type(void) override
    {
        return std::string("value");
    }
    uint_t value(void) override
    {
        return value_;
    }
};

// ==================================
// unary expression
// ==================================
/// @class Unary
/// @brief A unary expression
class Unary : public Expr
{
public:
    enum Op
    {
    };

protected:
    Expr &operand_;
    OpType op_;

public:
    /// @brief Create a new unary expression
    /// @param (op) The opcode describing which operation is being done
    /// @param (operand) The operand of the operation
    Unary(OpType op, Expr &operand) : op_(op), operand_(operand) {}

    /// @brief Create a new unary expression as a copy of src
    /// @param (src) copy source
    Unary(const Unary &src) : operand_(src.operand_)
    {
        op_ = src.op_;
    }

    /// @brief Return the type string ("unary)
    std::string type(void) override
    {
        return std::string("unary");
    }

    OpType Op(void) override
    {
        return op_;
    }
};

// ==================================
// binary expression
// ==================================
/// @class Binary
/// @brief A binary expression
class Binary : public Expr
{
protected:
    Expr &left_;
    Expr &right_;
    OpType op_;

public:
    /// @brief Create a new binary expression
    /// @param (op) The opcode describing which operation is being done
    /// @param (left) The left-hand operand
    /// @param (right) The right-hand operand
    Binary(OpType op, Expr &left, Expr &right) : op_(op), left_(left), right_(right) {}

    /// @brief Create a new binary expression as a copy of src
    /// @param (src) copy source
    Binary(const Binary &src) : left_(src.left_), right_(src.right_)
    {
        op_ = src.op_;
    }

    /// @brief Return the type string ("binary")
    std::string type(void) override
    {
        return std::string("binary");
    }
    OpType Op(void) override
    {
        return op_;
    }
};

// ==================================
// implementations of expressions
// ==================================
Unary Expr::operator!(void)
{
    return Unary(OpType::LOGIC_NOT, *this);
}

Binary Expr::operator==(uint_t right)
{
    Value r(right);
    return Binary(OpType::EQUAL, *this, r);
}

Binary Expr::operator!=(uint_t right)
{
    Value r(right);
    return Binary(OpType::NOT_EQUAL, *this, r);
}

Binary Expr::operator<(uint_t right)
{
    Value r(right);
    return Binary(OpType::LESS, *this, r);
}

Binary Expr::operator<=(uint_t right)
{
    Value r(right);
    return Binary(OpType::LESS_EQUAL, *this, r);
}

Binary Expr::operator>(uint_t right)
{
    Value r(right);
    return Binary(OpType::GREATER, *this, r);
}

Binary Expr::operator>=(uint_t right)
{
    Value r(right);
    return Binary(OpType::GREATER_EQUAL, *this, r);
}

Binary Expr::operator==(ClassicalRegister &right)
{
    Var r(right);
    return Binary(OpType::EQUAL, *this, r);
}

Binary Expr::operator!=(ClassicalRegister &right)
{
    Var r(right);
    return Binary(OpType::NOT_EQUAL, *this, r);
}

Binary Expr::operator<(ClassicalRegister &right)
{
    Var r(right);
    return Binary(OpType::LESS, *this, r);
}

Binary Expr::operator<=(ClassicalRegister &right)
{
    Var r(right);
    return Binary(OpType::LESS_EQUAL, *this, r);
}

Binary Expr::operator>(ClassicalRegister &right)
{
    Var r(right);
    return Binary(OpType::GREATER, *this, r);
}

Binary Expr::operator>=(ClassicalRegister &right)
{
    Var r(right);
    return Binary(OpType::GREATER_EQUAL, *this, r);
}

Binary Expr::operator==(Expr &r)
{
    return Binary(OpType::EQUAL, *this, r);
}

Binary Expr::operator!=(Expr &r)
{
    return Binary(OpType::NOT_EQUAL, *this, r);
}

Binary Expr::operator<(Expr &r)
{
    return Binary(OpType::LESS, *this, r);
}

Binary Expr::operator<=(Expr &r)
{
    return Binary(OpType::LESS_EQUAL, *this, r);
}

Binary Expr::operator>(Expr &r)
{
    return Binary(OpType::GREATER, *this, r);
}

Binary Expr::operator>=(Expr &r)
{
    return Binary(OpType::GREATER_EQUAL, *this, r);
}

Binary Expr::operator&&(Expr &r)
{
    return Binary(OpType::LOGIC_AND, *this, r);
}

Binary Expr::operator||(Expr &r)
{
    return Binary(OpType::LOGIC_OR, *this, r);
}

Unary ClassicalRegister::operator!(void)
{
    Var v(*this);
    return Unary(OpType::LOGIC_NOT, v);
}

Binary ClassicalRegister::operator==(ClassicalRegister &right)
{
    Var v(*this);
    Var r(right);
    return Binary(OpType::EQUAL, v, r);
}

Binary ClassicalRegister::operator!=(ClassicalRegister &right)
{
    Var v(*this);
    Var r(right);
    return Binary(OpType::NOT_EQUAL, v, r);
}

Binary ClassicalRegister::operator<(ClassicalRegister &right)
{
    Var v(*this);
    Var r(right);
    return Binary(OpType::LESS, v, r);
}

Binary ClassicalRegister::operator<=(ClassicalRegister &right)
{
    Var v(*this);
    Var r(right);
    return Binary(OpType::LESS_EQUAL, v, r);
}

Binary ClassicalRegister::operator>(ClassicalRegister &right)
{
    Var v(*this);
    Var r(right);
    return Binary(OpType::GREATER, v, r);
}

Binary ClassicalRegister::operator>=(ClassicalRegister &right)
{
    Var v(*this);
    Var r(right);
    return Binary(OpType::GREATER_EQUAL, v, r);
}

Binary ClassicalRegister::operator==(uint_t right)
{
    Var v(*this);
    Value r(right);
    return Binary(OpType::EQUAL, v, r);
}

Binary ClassicalRegister::operator!=(uint_t right)
{
    Var v(*this);
    Value r(right);
    return Binary(OpType::NOT_EQUAL, v, r);
}

Binary ClassicalRegister::operator<(uint_t right)
{
    Var v(*this);
    Value r(right);
    return Binary(OpType::LESS, v, r);
}

Binary ClassicalRegister::operator<=(uint_t right)
{
    Var v(*this);
    Value r(right);
    return Binary(OpType::LESS_EQUAL, v, r);
}

Binary ClassicalRegister::operator>(uint_t right)
{
    Var v(*this);
    Value r(right);
    return Binary(OpType::GREATER, v, r);
}

Binary ClassicalRegister::operator>=(uint_t right)
{
    Var v(*this);
    Value r(right);
    return Binary(OpType::GREATER_EQUAL, v, r);
}

Binary ClassicalRegister::operator==(Expr &right)
{
    Var v(*this);
    return Binary(OpType::EQUAL, v, right);
}

Binary ClassicalRegister::operator!=(Expr &right)
{
    Var v(*this);
    return Binary(OpType::NOT_EQUAL, v, right);
}

Binary ClassicalRegister::operator<(Expr &right)
{
    Var v(*this);
    return Binary(OpType::LESS, v, right);
}

Binary ClassicalRegister::operator<=(Expr &right)
{
    Var v(*this);
    return Binary(OpType::LESS_EQUAL, v, right);
}

Binary ClassicalRegister::operator>(Expr &right)
{
    Var v(*this);
    return Binary(OpType::GREATER, v, right);
}

Binary ClassicalRegister::operator>=(Expr &right)
{
    Var v(*this);
    return Binary(OpType::GREATER_EQUAL, v, right);
}

} // namespace circuit
} // namespace Qiskit

#endif // __qiskitcpp_circuit_classical_expr_hpp__
