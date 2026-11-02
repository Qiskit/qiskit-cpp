/*
# This code is part of Qiskit.
#
# (C) Copyright IBM 2026.
#
# This code is licensed under the Apache License, Version 2.0. You may
# obtain a copy of this license in the LICENSE.txt file in the root directory
# of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
#
# Any modifications or derivative works of this code must retain this
# copyright notice, and modified files need to carry a notice indicating
# that they have been altered from the originals.
*/

// Read-only view over a classical expression held by the Qiskit C-API
//
// The ``qk_expr_*`` C-API is an inspection interface: an expression pointer can
// only be obtained by reading a control flow instruction that is already in a
// circuit (``qk_control_flow_condition_expr``,
// ``qk_control_flow_switch_target_expr``, ``qk_control_flow_box_duration_expr``).
// There is no constructor and no free function, so the classes here borrow and
// never own. To build a condition in C++, use the expression tree in
// "circuit/classical/expr.hpp" instead.

#ifndef __qiskitcpp_circuit_classical_expr_view_hpp__
#define __qiskitcpp_circuit_classical_expr_view_hpp__

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "utils/types.hpp"

#include "qiskit.h"

namespace Qiskit
{
namespace circuit
{

class ExprView;

/// @brief The kind of node an ExprView refers to.
enum class ExprNodeKind
{
    Unary,   ///< A unary operation.
    Binary,  ///< A binary operation.
    Cast,    ///< A type cast.
    Value,   ///< A literal value.
    Var,     ///< A variable reference.
    Stretch, ///< A stretch, used for timing.
    Index,   ///< An index or subscript.
    Unknown, ///< A node this version of qiskit-cpp does not recognize.
};

/// @brief The data type of a classical expression.
enum class ExprType
{
    Bool,     ///< A boolean.
    Duration, ///< A duration.
    Float,    ///< A floating point number.
    Uint,     ///< An unsigned integer of a given width.
    Unknown,  ///< A type this version of qiskit-cpp does not recognize.
};

/// @brief A unary operator.
enum class UnaryOpType
{
    BitNot,   ///< Bitwise NOT.
    LogicNot, ///< Logical NOT.
    Negate,   ///< Arithmetic negation.
    Unknown,  ///< An operator this version of qiskit-cpp does not recognize.
};

/// @brief A binary operator.
enum class BinaryOpType
{
    BitAnd,       ///< Bitwise AND.
    BitOr,        ///< Bitwise OR.
    BitXor,       ///< Bitwise XOR.
    LogicAnd,     ///< Logical AND.
    LogicOr,      ///< Logical OR.
    Equal,        ///< Equality.
    NotEqual,     ///< Inequality.
    Less,         ///< Less than.
    LessEqual,    ///< Less than or equal.
    Greater,      ///< Greater than.
    GreaterEqual, ///< Greater than or equal.
    ShiftLeft,    ///< Left shift.
    ShiftRight,   ///< Right shift.
    Add,          ///< Addition.
    Sub,          ///< Subtraction.
    Mul,          ///< Multiplication.
    Div,          ///< Division.
    Unknown,      ///< An operator this version of qiskit-cpp does not recognize.
};

/// @brief The unit of a duration value.
enum class DurationType
{
    dt,      ///< Backend cycles.
    ps,      ///< Picoseconds.
    ns,      ///< Nanoseconds.
    us,      ///< Microseconds.
    ms,      ///< Milliseconds.
    s,       ///< Seconds.
    Unknown, ///< A unit this version of qiskit-cpp does not recognize.
};

/// @class ExprTypeInfo
/// @brief The complete type of a classical expression.
class ExprTypeInfo
{
protected:
    ExprType type_ = ExprType::Unknown;
    uint32_t width_ = 0;

public:
    ExprTypeInfo(void) {}

    /// @brief Wrap a type reported by the C-API.
    /// @param info the type information
    explicit ExprTypeInfo(const QkExprTypeInfo &info)
    {
        switch (info.ty) {
        case QkExprType_Bool:
            type_ = ExprType::Bool;
            break;
        case QkExprType_Duration:
            type_ = ExprType::Duration;
            break;
        case QkExprType_Float:
            type_ = ExprType::Float;
            break;
        case QkExprType_Uint:
            type_ = ExprType::Uint;
            break;
        default:
            type_ = ExprType::Unknown;
            break;
        }
        width_ = info.width;
    }

    /// @brief Return the type.
    ExprType type(void) const { return type_; }

    /// @brief Return the bit width, for an unsigned integer type.
    /// @return the width; zero for every other type
    uint32_t width(void) const { return width_; }

    /// @brief Return the type as a string, e.g. "uint[8]" or "bool".
    std::string to_string(void) const
    {
        switch (type_) {
        case ExprType::Bool:
            return "bool";
        case ExprType::Duration:
            return "duration";
        case ExprType::Float:
            return "float";
        case ExprType::Uint: {
            std::stringstream ret;
            ret << "uint[" << width_ << "]";
            return ret.str();
        }
        default:
            return "unknown";
        }
    }
};

/// @class DurationValue
/// @brief A duration held by a classical expression value.
class DurationValue
{
protected:
    DurationType type_ = DurationType::Unknown;
    std::int64_t dt_ = 0;
    double time_ = 0.0;

public:
    DurationValue(void) {}

    /// @brief Wrap a duration reported by the C-API.
    /// @param info the duration information
    explicit DurationValue(const QkDurationInfo &info)
    {
        switch (info.ty) {
        case QkDurationType_Dt:
            type_ = DurationType::dt;
            // The value is a union: only the dt member is live for this type.
            dt_ = info.value.dt;
            return;
        case QkDurationType_Ps:
            type_ = DurationType::ps;
            break;
        case QkDurationType_Ns:
            type_ = DurationType::ns;
            break;
        case QkDurationType_Us:
            type_ = DurationType::us;
            break;
        case QkDurationType_Ms:
            type_ = DurationType::ms;
            break;
        case QkDurationType_S:
            type_ = DurationType::s;
            break;
        default:
            type_ = DurationType::Unknown;
            return;
        }
        time_ = info.value.time;
    }

    /// @brief Return the unit of this duration.
    DurationType type(void) const { return type_; }

    /// @brief Return whether this duration is counted in backend cycles.
    bool is_dt(void) const { return type_ == DurationType::dt; }

    /// @brief Return the duration in backend cycles.
    /// @return the cycle count; zero unless ``is_dt()``
    std::int64_t dt(void) const { return dt_; }

    /// @brief Return the duration as a real time.
    /// @return the time in the unit returned by ``type()``; zero when ``is_dt()``
    double time(void) const { return time_; }

    /// @brief Return the unit name, one of "dt", "ps", "ns", "us", "ms", "s".
    const char *unit_name(void) const
    {
        switch (type_) {
        case DurationType::dt:
            return "dt";
        case DurationType::ps:
            return "ps";
        case DurationType::ns:
            return "ns";
        case DurationType::us:
            return "us";
        case DurationType::ms:
            return "ms";
        case DurationType::s:
            return "s";
        default:
            return "unknown";
        }
    }

    /// @brief Return the duration as a string, e.g. "100dt" or "1.5ns".
    std::string to_string(void) const
    {
        std::stringstream ret;
        if (is_dt()) {
            ret << dt_;
        } else {
            ret << time_;
        }
        ret << unit_name();
        return ret.str();
    }
};

/// @class ExprView
/// @brief A read-only view over one node of a classical expression tree.
/// @details Borrows a ``QkExprNode`` owned by the circuit it was read from, so
///   it must not outlive that circuit. Copying a view is cheap and copies only
///   the borrow.
/// @note Every accessor that applies to a single node kind reports its
///   precondition in its documentation and throws ``std::logic_error`` when the
///   node is of another kind. This matters: the underlying C-API panics in that
///   situation, which would abort the process.
class ExprView
{
protected:
    const QkExprNode *expr_ = nullptr;

    /// @brief Throw if this view is not of the expected kind.
    /// @param expected the required kind
    /// @param accessor the accessor being called, for the message
    void require(const ExprNodeKind expected, const char *accessor) const
    {
        if (expr_ == nullptr) {
            std::stringstream message;
            message << "ExprView::" << accessor << " called on an empty view";
            throw std::logic_error(message.str());
        }
        if (kind() != expected) {
            std::stringstream message;
            message << "ExprView::" << accessor << " called on a "
                    << kind_name() << " expression";
            throw std::logic_error(message.str());
        }
    }

public:
    ExprView(void) {}

    /// @brief Wrap an expression owned by a circuit.
    /// @param expr the expression to borrow, which may be null
    explicit ExprView(const QkExprNode *expr) : expr_(expr) {}

    /// @brief Return whether this view refers to an expression.
    bool valid(void) const { return expr_ != nullptr; }

    /// @brief Return the borrowed C-API expression.
    const QkExprNode *get(void) const { return expr_; }

    /// @brief Return the kind of node this view refers to.
    /// @return the kind; ExprNodeKind::Unknown for an empty view
    ExprNodeKind kind(void) const
    {
        if (expr_ == nullptr) {
            return ExprNodeKind::Unknown;
        }
        switch (qk_expr_kind(expr_)) {
        case QkExprNodeKind_Unary:
            return ExprNodeKind::Unary;
        case QkExprNodeKind_Binary:
            return ExprNodeKind::Binary;
        case QkExprNodeKind_Cast:
            return ExprNodeKind::Cast;
        case QkExprNodeKind_Value:
            return ExprNodeKind::Value;
        case QkExprNodeKind_Var:
            return ExprNodeKind::Var;
        case QkExprNodeKind_Stretch:
            return ExprNodeKind::Stretch;
        case QkExprNodeKind_Index:
            return ExprNodeKind::Index;
        default:
            return ExprNodeKind::Unknown;
        }
    }

    /// @brief Return the name of this node's kind, for diagnostics.
    const char *kind_name(void) const
    {
        switch (kind()) {
        case ExprNodeKind::Unary:
            return "unary";
        case ExprNodeKind::Binary:
            return "binary";
        case ExprNodeKind::Cast:
            return "cast";
        case ExprNodeKind::Value:
            return "value";
        case ExprNodeKind::Var:
            return "var";
        case ExprNodeKind::Stretch:
            return "stretch";
        case ExprNodeKind::Index:
            return "index";
        default:
            return "unknown";
        }
    }

    /// @brief Return the result type of this expression.
    /// @return the type; an Unknown type for a node kind that reports none
    /// @details A value's type comes from the value itself and a var's from the
    ///   variable, so this works for every kind except Stretch, which has no
    ///   type of its own.
    ExprTypeInfo type(void) const
    {
        switch (kind()) {
        case ExprNodeKind::Unary:
            return ExprTypeInfo(qk_expr_unary_info(expr_).ty);
        case ExprNodeKind::Binary:
            return ExprTypeInfo(qk_expr_binary_info(expr_).ty);
        case ExprNodeKind::Cast:
            return ExprTypeInfo(qk_expr_cast_info(expr_).ty);
        case ExprNodeKind::Index:
            return ExprTypeInfo(qk_expr_index_info(expr_).ty);
        case ExprNodeKind::Value:
            return ExprTypeInfo(qk_value_type_info(qk_expr_as_value(expr_)));
        case ExprNodeKind::Var:
            return ExprTypeInfo(qk_var_type_info(qk_expr_as_var(expr_)));
        default:
            return ExprTypeInfo();
        }
    }

    /// @brief Return whether this expression is constant.
    /// @return true if constant; false for a kind that does not report it
    bool is_constant(void) const
    {
        switch (kind()) {
        case ExprNodeKind::Unary:
            return qk_expr_unary_info(expr_).constant;
        case ExprNodeKind::Binary:
            return qk_expr_binary_info(expr_).constant;
        case ExprNodeKind::Cast:
            return qk_expr_cast_info(expr_).constant;
        case ExprNodeKind::Index:
            return qk_expr_index_info(expr_).constant;
        default:
            return false;
        }
    }

    // ----- unary -----

    /// @brief Return the operator of this unary expression.
    /// @throws std::logic_error unless this is a unary expression
    UnaryOpType unary_op(void) const
    {
        require(ExprNodeKind::Unary, "unary_op");
        switch (qk_expr_unary_info(expr_).op) {
        case QkUnaryOpType_BitNot:
            return UnaryOpType::BitNot;
        case QkUnaryOpType_LogicNot:
            return UnaryOpType::LogicNot;
        case QkUnaryOpType_Negate:
            return UnaryOpType::Negate;
        default:
            return UnaryOpType::Unknown;
        }
    }

    /// @brief Return the operand of this unary expression.
    /// @throws std::logic_error unless this is a unary expression
    ExprView operand(void) const
    {
        require(ExprNodeKind::Unary, "operand");
        return ExprView(qk_expr_unary_info(expr_).operand);
    }

    // ----- binary -----

    /// @brief Return the operator of this binary expression.
    /// @throws std::logic_error unless this is a binary expression
    BinaryOpType binary_op(void) const
    {
        require(ExprNodeKind::Binary, "binary_op");
        switch (qk_expr_binary_info(expr_).op) {
        case QkBinaryOpType_BitAnd:
            return BinaryOpType::BitAnd;
        case QkBinaryOpType_BitOr:
            return BinaryOpType::BitOr;
        case QkBinaryOpType_BitXor:
            return BinaryOpType::BitXor;
        case QkBinaryOpType_LogicAnd:
            return BinaryOpType::LogicAnd;
        case QkBinaryOpType_LogicOr:
            return BinaryOpType::LogicOr;
        case QkBinaryOpType_Equal:
            return BinaryOpType::Equal;
        case QkBinaryOpType_NotEqual:
            return BinaryOpType::NotEqual;
        case QkBinaryOpType_Less:
            return BinaryOpType::Less;
        case QkBinaryOpType_LessEqual:
            return BinaryOpType::LessEqual;
        case QkBinaryOpType_Greater:
            return BinaryOpType::Greater;
        case QkBinaryOpType_GreaterEqual:
            return BinaryOpType::GreaterEqual;
        case QkBinaryOpType_ShiftLeft:
            return BinaryOpType::ShiftLeft;
        case QkBinaryOpType_ShiftRight:
            return BinaryOpType::ShiftRight;
        case QkBinaryOpType_Add:
            return BinaryOpType::Add;
        case QkBinaryOpType_Sub:
            return BinaryOpType::Sub;
        case QkBinaryOpType_Mul:
            return BinaryOpType::Mul;
        case QkBinaryOpType_Div:
            return BinaryOpType::Div;
        default:
            return BinaryOpType::Unknown;
        }
    }

    /// @brief Return the left operand of this binary expression.
    /// @throws std::logic_error unless this is a binary expression
    ExprView left(void) const
    {
        require(ExprNodeKind::Binary, "left");
        return ExprView(qk_expr_binary_info(expr_).left);
    }

    /// @brief Return the right operand of this binary expression.
    /// @throws std::logic_error unless this is a binary expression
    ExprView right(void) const
    {
        require(ExprNodeKind::Binary, "right");
        return ExprView(qk_expr_binary_info(expr_).right);
    }

    // ----- cast -----

    /// @brief Return the operand of this cast expression.
    /// @throws std::logic_error unless this is a cast expression
    ExprView cast_operand(void) const
    {
        require(ExprNodeKind::Cast, "cast_operand");
        return ExprView(qk_expr_cast_info(expr_).operand);
    }

    /// @brief Return whether this cast is implicit rather than written out.
    /// @throws std::logic_error unless this is a cast expression
    bool is_implicit_cast(void) const
    {
        require(ExprNodeKind::Cast, "is_implicit_cast");
        return qk_expr_cast_info(expr_).implicit;
    }

    // ----- index -----

    /// @brief Return the expression being indexed.
    /// @throws std::logic_error unless this is an index expression
    ExprView target(void) const
    {
        require(ExprNodeKind::Index, "target");
        return ExprView(qk_expr_index_info(expr_).target);
    }

    /// @brief Return the index expression.
    /// @throws std::logic_error unless this is an index expression
    ExprView index(void) const
    {
        require(ExprNodeKind::Index, "index");
        return ExprView(qk_expr_index_info(expr_).index);
    }

    // ----- value -----

    /// @brief Return this value as an unsigned integer.
    /// @throws std::logic_error unless this is a value of unsigned integer type
    /// @note Only safe for a width of 64 bits or fewer, which ``type().width()``
    ///   reports. A wider value is rejected here rather than passed to the
    ///   C-API, which panics when it does not fit in ``uint64_t``.
    uint_t value_uint(void) const
    {
        require(ExprNodeKind::Value, "value_uint");
        const ExprTypeInfo info = type();
        if (info.type() != ExprType::Uint) {
            throw std::logic_error(
                "ExprView::value_uint called on a value of type " + info.to_string());
        }
        if (info.width() > 64) {
            std::stringstream message;
            message << "ExprView::value_uint cannot represent a " << info.width()
                    << "-bit value";
            throw std::logic_error(message.str());
        }
        return (uint_t)qk_value_uint(qk_expr_as_value(expr_));
    }

    /// @brief Return this value as a boolean.
    /// @throws std::logic_error unless this is a value of boolean type
    bool value_bool(void) const
    {
        require(ExprNodeKind::Value, "value_bool");
        const ExprTypeInfo info = type();
        if (info.type() != ExprType::Bool) {
            throw std::logic_error(
                "ExprView::value_bool called on a value of type " + info.to_string());
        }
        return qk_value_bool(qk_expr_as_value(expr_));
    }

    /// @brief Return this value as a floating point number.
    /// @throws std::logic_error unless this is a value of floating point type
    double value_float(void) const
    {
        require(ExprNodeKind::Value, "value_float");
        const ExprTypeInfo info = type();
        if (info.type() != ExprType::Float) {
            throw std::logic_error(
                "ExprView::value_float called on a value of type " + info.to_string());
        }
        return qk_value_float(qk_expr_as_value(expr_));
    }

    /// @brief Return this value as a duration.
    /// @throws std::logic_error unless this is a value of duration type
    DurationValue value_duration(void) const
    {
        require(ExprNodeKind::Value, "value_duration");
        const ExprTypeInfo info = type();
        if (info.type() != ExprType::Duration) {
            throw std::logic_error(
                "ExprView::value_duration called on a value of type " + info.to_string());
        }
        return DurationValue(qk_value_duration_info(qk_expr_as_value(expr_)));
    }

    // ----- var and stretch -----

    /// @brief Return the name of this variable.
    /// @return the name, or an empty string for a variable that has none
    /// @throws std::logic_error unless this is a variable
    /// @details Only a standalone variable carries a name. One that aliases a
    ///   classical register or a single bit reports none, in which case the
    ///   register or bit it refers to has to be recovered from the instruction
    ///   the expression came from.
    std::string var_name(void) const
    {
        require(ExprNodeKind::Var, "var_name");
        char *name = qk_var_name(qk_expr_as_var(expr_));
        if (name == nullptr) {
            return std::string();
        }
        std::string ret(name);
        qk_str_free(name);
        return ret;
    }

    /// @brief Return the name of this stretch.
    /// @throws std::logic_error unless this is a stretch
    std::string stretch_name(void) const
    {
        require(ExprNodeKind::Stretch, "stretch_name");
        char *name = qk_stretch_name(qk_expr_as_stretch(expr_));
        if (name == nullptr) {
            return std::string();
        }
        std::string ret(name);
        qk_str_free(name);
        return ret;
    }

    // ----- traversal -----

    /// @brief Return the direct children of this node.
    /// @return the operands, in left-to-right order; empty for a leaf
    std::vector<ExprView> children(void) const
    {
        std::vector<ExprView> ret;
        switch (kind()) {
        case ExprNodeKind::Unary:
            ret.push_back(operand());
            break;
        case ExprNodeKind::Binary:
            ret.push_back(left());
            ret.push_back(right());
            break;
        case ExprNodeKind::Cast:
            ret.push_back(cast_operand());
            break;
        case ExprNodeKind::Index:
            ret.push_back(target());
            ret.push_back(index());
            break;
        default:
            break;
        }
        return ret;
    }

    /// @brief Return this expression as a readable string.
    /// @return an infix rendering, e.g. "(c0 == 3)" or "!(a && b)"
    /// @note Intended for inspection and diagnostics, not as OpenQASM 3. A
    ///   variable with no name of its own renders as "<var>".
    std::string to_string(void) const
    {
        if (expr_ == nullptr) {
            return std::string("<none>");
        }

        switch (kind()) {
        case ExprNodeKind::Unary: {
            std::stringstream ret;
            ret << unary_op_name(unary_op()) << operand().to_string();
            return ret.str();
        }
        case ExprNodeKind::Binary: {
            std::stringstream ret;
            ret << "(" << left().to_string() << " "
                << binary_op_name(binary_op()) << " " << right().to_string() << ")";
            return ret.str();
        }
        case ExprNodeKind::Cast: {
            std::stringstream ret;
            ret << type().to_string() << "(" << cast_operand().to_string() << ")";
            return ret.str();
        }
        case ExprNodeKind::Index: {
            std::stringstream ret;
            ret << target().to_string() << "[" << index().to_string() << "]";
            return ret.str();
        }
        case ExprNodeKind::Value:
            return value_to_string();
        case ExprNodeKind::Var: {
            const std::string name = var_name();
            return name.empty() ? std::string("<var>") : name;
        }
        case ExprNodeKind::Stretch: {
            const std::string name = stretch_name();
            return name.empty() ? std::string("<stretch>") : name;
        }
        default:
            return std::string("<unknown>");
        }
    }

    /// @brief Return the symbol for a unary operator, e.g. "!" or "~".
    static const char *unary_op_name(const UnaryOpType op)
    {
        switch (op) {
        case UnaryOpType::BitNot:
            return "~";
        case UnaryOpType::LogicNot:
            return "!";
        case UnaryOpType::Negate:
            return "-";
        default:
            return "?";
        }
    }

    /// @brief Return the symbol for a binary operator, e.g. "==" or "&&".
    static const char *binary_op_name(const BinaryOpType op)
    {
        switch (op) {
        case BinaryOpType::BitAnd:
            return "&";
        case BinaryOpType::BitOr:
            return "|";
        case BinaryOpType::BitXor:
            return "^";
        case BinaryOpType::LogicAnd:
            return "&&";
        case BinaryOpType::LogicOr:
            return "||";
        case BinaryOpType::Equal:
            return "==";
        case BinaryOpType::NotEqual:
            return "!=";
        case BinaryOpType::Less:
            return "<";
        case BinaryOpType::LessEqual:
            return "<=";
        case BinaryOpType::Greater:
            return ">";
        case BinaryOpType::GreaterEqual:
            return ">=";
        case BinaryOpType::ShiftLeft:
            return "<<";
        case BinaryOpType::ShiftRight:
            return ">>";
        case BinaryOpType::Add:
            return "+";
        case BinaryOpType::Sub:
            return "-";
        case BinaryOpType::Mul:
            return "*";
        case BinaryOpType::Div:
            return "/";
        default:
            return "?";
        }
    }

protected:
    /// @brief Render a value node, whatever its type.
    std::string value_to_string(void) const
    {
        std::stringstream ret;
        switch (type().type()) {
        case ExprType::Bool:
            ret << (value_bool() ? "true" : "false");
            break;
        case ExprType::Float:
            ret << value_float();
            break;
        case ExprType::Duration:
            ret << value_duration().to_string();
            break;
        case ExprType::Uint:
            // A value wider than 64 bits cannot be read through the C-API, so
            // it is rendered by its type rather than its digits.
            if (type().width() > 64) {
                ret << "<" << type().to_string() << ">";
            } else {
                ret << value_uint();
            }
            break;
        default:
            ret << "<unknown>";
            break;
        }
        return ret.str();
    }
};

/// @brief What a control flow instruction tests to decide its branch.
enum class ConditionType
{
    Clbit,    ///< A single classical bit.
    Clreg,    ///< A classical register compared against a value.
    Expr,     ///< A classical expression.
    Unknown,  ///< A condition this version of qiskit-cpp does not recognize.
};

/// @class ControlFlowInstruction
/// @brief A control flow instruction read out of a circuit.
/// @details Owns the ``QkControlFlowInstruction`` the C-API allocates and frees
///   it on destruction. The expressions reached through it are borrowed from it,
///   so an ExprView obtained here must not outlive this object.
/// @note Provided so that an ExprView can be obtained at all: the C-API has no
///   way to construct an expression, only to read one back from a circuit. Only
///   the condition and switch target are exposed; the rest of the
///   ``qk_control_flow_*`` interface is not wrapped yet.
class ControlFlowInstruction
{
protected:
    QkControlFlowInstruction *inst_ = nullptr;

public:
    /// @brief Read the control flow instruction at an index of a circuit.
    /// @param circuit the circuit to read from
    /// @param index the index of the instruction, which must be a control flow
    ///   instruction. Check with ``qk_circuit_instruction_kind`` first: the
    ///   C-API's behavior is undefined otherwise.
    ControlFlowInstruction(const QkCircuit *circuit, const uint_t index)
    {
        if (circuit == nullptr) {
            return;
        }
        inst_ = qk_circuit_get_control_flow_instruction(circuit, index, nullptr);
    }

    // Owns a C-API allocation, so it must not be copied.
    ControlFlowInstruction(const ControlFlowInstruction &) = delete;
    ControlFlowInstruction &operator=(const ControlFlowInstruction &) = delete;

    ~ControlFlowInstruction()
    {
        if (inst_ != nullptr) {
            qk_control_flow_instruction_free(inst_);
        }
    }

    /// @brief Return whether the instruction was read successfully.
    bool valid(void) const { return inst_ != nullptr; }

    /// @brief Return what this instruction tests.
    /// @return the condition type; Unknown if the instruction has no condition
    /// @note Only an if/else or a while loop carries a condition.
    ConditionType condition_type(void) const
    {
        if (inst_ == nullptr) {
            return ConditionType::Unknown;
        }
        const QkControlFlowKind kind = qk_control_flow_kind(inst_);
        if (kind != QkControlFlowKind_IfElse && kind != QkControlFlowKind_While) {
            return ConditionType::Unknown;
        }
        switch (qk_control_flow_condition_type(inst_)) {
        case QkConditionType_ClBit:
            return ConditionType::Clbit;
        case QkConditionType_ClReg:
            return ConditionType::Clreg;
        case QkConditionType_Expr:
            return ConditionType::Expr;
        default:
            return ConditionType::Unknown;
        }
    }

    /// @brief Return the condition of this instruction as an expression.
    /// @return a view over the condition, or an empty view when the condition
    ///   is not an expression
    /// @note Returns an empty view rather than calling the C-API, which panics
    ///   when the condition is a bit or a register comparison.
    ExprView condition_expr(void) const
    {
        if (condition_type() != ConditionType::Expr) {
            return ExprView();
        }
        return ExprView(qk_control_flow_condition_expr(inst_));
    }

    /// @brief Return the value a switch instruction selects on.
    /// @return a view over the target, or an empty view when this is not a
    ///   switch whose target is an expression
    ExprView switch_target_expr(void) const
    {
        if (inst_ == nullptr || qk_control_flow_kind(inst_) != QkControlFlowKind_Switch) {
            return ExprView();
        }
        if (qk_control_flow_switch_target_type(inst_) != QkConditionType_Expr) {
            return ExprView();
        }
        return ExprView(qk_control_flow_switch_target_expr(inst_));
    }
};

} // namespace circuit
} // namespace Qiskit

#endif // __qiskitcpp_circuit_classical_expr_view_hpp__
