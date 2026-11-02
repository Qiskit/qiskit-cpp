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
//
// These classes build an expression tree in C++. They are not backed by the
// Qiskit C-API: the ``qk_expr_*`` interface is read-only, with no constructor
// for an expression, so a tree built here cannot yet be handed to a circuit.
// To inspect an expression read back out of a circuit, use the ExprView class
// in "circuit/classical/expr_view.hpp" instead.

#ifndef __qiskitcpp_circuit_classical_expr_hpp__
#define __qiskitcpp_circuit_classical_expr_hpp__

#include <memory>
#include <string>

#include "circuit/classicalregister.hpp"
#include "circuit/classical/expr_view.hpp"

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

/// @brief Return the symbol for an operation, e.g. "==" or "&&".
/// @param op the operation
/// @return the symbol, or "?" for OpType::NONE
inline const char *op_type_name(const OpType op)
{
    switch (op) {
    case OpType::BIT_NOT:
        return "~";
    case OpType::LOGIC_NOT:
        return "!";
    case OpType::BIT_AND:
        return "&";
    case OpType::BIT_OR:
        return "|";
    case OpType::BIT_XOR:
        return "^";
    case OpType::LOGIC_AND:
        return "&&";
    case OpType::LOGIC_OR:
        return "||";
    case OpType::EQUAL:
        return "==";
    case OpType::NOT_EQUAL:
        return "!=";
    case OpType::LESS:
        return "<";
    case OpType::LESS_EQUAL:
        return "<=";
    case OpType::GREATER:
        return ">";
    case OpType::GREATER_EQUAL:
        return ">=";
    case OpType::SHIFT_LEFT:
        return "<<";
    case OpType::SHIFT_RIGHT:
        return ">>";
    default:
        return "?";
    }
}

// ==================================
// base class for expressions
// ==================================
/// @class Expr
/// @brief Root base of all nodes in the expression tree.
/// @details Nodes are held by shared pointer, so an expression can be returned
///   from a function and copied freely. The operator overloads below take a
///   copy of each operand into the node they build.
class Expr
{
protected:
public:
    Expr() {}
    virtual ~Expr() {}

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

    Binary operator==(const Expr &right);
    Binary operator!=(const Expr &right);
    Binary operator<=(const Expr &right);
    Binary operator<(const Expr &right);
    Binary operator>=(const Expr &right);
    Binary operator>(const Expr &right);

    Binary operator||(const Expr &right);
    Binary operator&&(const Expr &right);

    virtual std::string type(void) const
    {
        return std::string("expr");
    }
    virtual uint_t value(void) const
    {
        return 0;
    }
    virtual ClassicalRegister creg(void) const
    {
        ClassicalRegister cr(0, "dummy");
        return cr;
    }
    virtual OpType Op(void) const
    {
        return OpType::NONE;
    }

    /// @brief Return a copy of this node, preserving its concrete type.
    /// @return an owning pointer to the copy
    /// @details Needed so that an operand can be stored by value in the node
    ///   that uses it without being sliced down to Expr.
    virtual std::shared_ptr<Expr> clone(void) const
    {
        return std::shared_ptr<Expr>(new Expr(*this));
    }

    /// @brief Return this expression as a readable string.
    /// @return an infix rendering, e.g. "(c0 == 3)"
    virtual std::string to_string(void) const
    {
        return std::string("<expr>");
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
    std::string type(void) const override
    {
        return std::string("var");
    }
    ClassicalRegister creg(void) const override
    {
        return register_;
    }

    std::shared_ptr<Expr> clone(void) const override
    {
        return std::shared_ptr<Expr>(new Var(*this));
    }

    std::string to_string(void) const override
    {
        return register_.name();
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
    std::string type(void) const override
    {
        return std::string("value");
    }
    uint_t value(void) const override
    {
        return value_;
    }

    std::shared_ptr<Expr> clone(void) const override
    {
        return std::shared_ptr<Expr>(new Value(*this));
    }

    std::string to_string(void) const override
    {
        return std::to_string(value_);
    }
};

// ==================================
// unary expression
// ==================================
/// @class Unary
/// @brief A unary expression
class Unary : public Expr
{
protected:
    std::shared_ptr<Expr> operand_;
    OpType op_;

public:
    /// @brief Create a new unary expression
    /// @param (op) The opcode describing which operation is being done
    /// @param (operand) The operand of the operation, which is copied
    Unary(OpType op, const Expr &operand) : operand_(operand.clone()), op_(op) {}

    /// @brief Create a new unary expression as a copy of src
    /// @param (src) copy source
    Unary(const Unary &src) : operand_(src.operand_), op_(src.op_) {}

    /// @brief Assign a copy of src to this expression
    /// @param (src) copy source
    Unary &operator=(const Unary &src)
    {
        if (this != &src) {
            operand_ = src.operand_;
            op_ = src.op_;
        }
        return *this;
    }

    /// @brief Return the type string ("unary)
    std::string type(void) const override
    {
        return std::string("unary");
    }

    OpType Op(void) const override
    {
        return op_;
    }

    /// @brief Return the operand of this expression.
    const Expr &operand(void) const
    {
        return *operand_;
    }

    std::shared_ptr<Expr> clone(void) const override
    {
        return std::shared_ptr<Expr>(new Unary(*this));
    }

    std::string to_string(void) const override
    {
        return std::string(op_type_name(op_)) + operand_->to_string();
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
    std::shared_ptr<Expr> left_;
    std::shared_ptr<Expr> right_;
    OpType op_;

public:
    /// @brief Create a new binary expression
    /// @param (op) The opcode describing which operation is being done
    /// @param (left) The left-hand operand, which is copied
    /// @param (right) The right-hand operand, which is copied
    Binary(OpType op, const Expr &left, const Expr &right)
        : left_(left.clone()), right_(right.clone()), op_(op) {}

    /// @brief Create a new binary expression as a copy of src
    /// @param (src) copy source
    Binary(const Binary &src) : left_(src.left_), right_(src.right_), op_(src.op_) {}

    /// @brief Assign a copy of src to this expression
    /// @param (src) copy source
    /// @details Declared explicitly because a user-declared copy constructor
    ///   makes the implicit assignment operator deprecated.
    Binary &operator=(const Binary &src)
    {
        if (this != &src) {
            left_ = src.left_;
            right_ = src.right_;
            op_ = src.op_;
        }
        return *this;
    }

    /// @brief Return the type string ("binary")
    std::string type(void) const override
    {
        return std::string("binary");
    }
    OpType Op(void) const override
    {
        return op_;
    }

    /// @brief Return the left-hand operand of this expression.
    const Expr &left(void) const
    {
        return *left_;
    }

    /// @brief Return the right-hand operand of this expression.
    const Expr &right(void) const
    {
        return *right_;
    }

    std::shared_ptr<Expr> clone(void) const override
    {
        return std::shared_ptr<Expr>(new Binary(*this));
    }

    std::string to_string(void) const override
    {
        return "(" + left_->to_string() + " " + op_type_name(op_) + " "
            + right_->to_string() + ")";
    }
};

// ==================================
// implementations of expressions
// ==================================
inline Unary Expr::operator!(void)
{
    return Unary(OpType::LOGIC_NOT, *this);
}

inline Binary Expr::operator==(uint_t right)
{
    return Binary(OpType::EQUAL, *this, Value(right));
}

inline Binary Expr::operator!=(uint_t right)
{
    return Binary(OpType::NOT_EQUAL, *this, Value(right));
}

inline Binary Expr::operator<(uint_t right)
{
    return Binary(OpType::LESS, *this, Value(right));
}

inline Binary Expr::operator<=(uint_t right)
{
    return Binary(OpType::LESS_EQUAL, *this, Value(right));
}

inline Binary Expr::operator>(uint_t right)
{
    return Binary(OpType::GREATER, *this, Value(right));
}

inline Binary Expr::operator>=(uint_t right)
{
    return Binary(OpType::GREATER_EQUAL, *this, Value(right));
}

inline Binary Expr::operator==(ClassicalRegister &right)
{
    return Binary(OpType::EQUAL, *this, Var(right));
}

inline Binary Expr::operator!=(ClassicalRegister &right)
{
    return Binary(OpType::NOT_EQUAL, *this, Var(right));
}

inline Binary Expr::operator<(ClassicalRegister &right)
{
    return Binary(OpType::LESS, *this, Var(right));
}

inline Binary Expr::operator<=(ClassicalRegister &right)
{
    return Binary(OpType::LESS_EQUAL, *this, Var(right));
}

inline Binary Expr::operator>(ClassicalRegister &right)
{
    return Binary(OpType::GREATER, *this, Var(right));
}

inline Binary Expr::operator>=(ClassicalRegister &right)
{
    return Binary(OpType::GREATER_EQUAL, *this, Var(right));
}

inline Binary Expr::operator==(const Expr &r)
{
    return Binary(OpType::EQUAL, *this, r);
}

inline Binary Expr::operator!=(const Expr &r)
{
    return Binary(OpType::NOT_EQUAL, *this, r);
}

inline Binary Expr::operator<(const Expr &r)
{
    return Binary(OpType::LESS, *this, r);
}

inline Binary Expr::operator<=(const Expr &r)
{
    return Binary(OpType::LESS_EQUAL, *this, r);
}

inline Binary Expr::operator>(const Expr &r)
{
    return Binary(OpType::GREATER, *this, r);
}

inline Binary Expr::operator>=(const Expr &r)
{
    return Binary(OpType::GREATER_EQUAL, *this, r);
}

inline Binary Expr::operator&&(const Expr &r)
{
    return Binary(OpType::LOGIC_AND, *this, r);
}

inline Binary Expr::operator||(const Expr &r)
{
    return Binary(OpType::LOGIC_OR, *this, r);
}

inline Unary ClassicalRegister::operator!(void)
{
    return Unary(OpType::LOGIC_NOT, Var(*this));
}

inline Binary ClassicalRegister::operator==(ClassicalRegister &right)
{
    return Binary(OpType::EQUAL, Var(*this), Var(right));
}

inline Binary ClassicalRegister::operator!=(ClassicalRegister &right)
{
    return Binary(OpType::NOT_EQUAL, Var(*this), Var(right));
}

inline Binary ClassicalRegister::operator<(ClassicalRegister &right)
{
    return Binary(OpType::LESS, Var(*this), Var(right));
}

inline Binary ClassicalRegister::operator<=(ClassicalRegister &right)
{
    return Binary(OpType::LESS_EQUAL, Var(*this), Var(right));
}

inline Binary ClassicalRegister::operator>(ClassicalRegister &right)
{
    return Binary(OpType::GREATER, Var(*this), Var(right));
}

inline Binary ClassicalRegister::operator>=(ClassicalRegister &right)
{
    return Binary(OpType::GREATER_EQUAL, Var(*this), Var(right));
}

inline Binary ClassicalRegister::operator==(uint_t right)
{
    return Binary(OpType::EQUAL, Var(*this), Value(right));
}

inline Binary ClassicalRegister::operator!=(uint_t right)
{
    return Binary(OpType::NOT_EQUAL, Var(*this), Value(right));
}

inline Binary ClassicalRegister::operator<(uint_t right)
{
    return Binary(OpType::LESS, Var(*this), Value(right));
}

inline Binary ClassicalRegister::operator<=(uint_t right)
{
    return Binary(OpType::LESS_EQUAL, Var(*this), Value(right));
}

inline Binary ClassicalRegister::operator>(uint_t right)
{
    return Binary(OpType::GREATER, Var(*this), Value(right));
}

inline Binary ClassicalRegister::operator>=(uint_t right)
{
    return Binary(OpType::GREATER_EQUAL, Var(*this), Value(right));
}

inline Binary ClassicalRegister::operator==(Expr &right)
{
    return Binary(OpType::EQUAL, Var(*this), right);
}

inline Binary ClassicalRegister::operator!=(Expr &right)
{
    return Binary(OpType::NOT_EQUAL, Var(*this), right);
}

inline Binary ClassicalRegister::operator<(Expr &right)
{
    return Binary(OpType::LESS, Var(*this), right);
}

inline Binary ClassicalRegister::operator<=(Expr &right)
{
    return Binary(OpType::LESS_EQUAL, Var(*this), right);
}

inline Binary ClassicalRegister::operator>(Expr &right)
{
    return Binary(OpType::GREATER, Var(*this), right);
}

inline Binary ClassicalRegister::operator>=(Expr &right)
{
    return Binary(OpType::GREATER_EQUAL, Var(*this), right);
}

} // namespace circuit
} // namespace Qiskit

#endif // __qiskitcpp_circuit_classical_expr_hpp__
