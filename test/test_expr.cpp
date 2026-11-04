// This code is part of Qiskit.
//
// (C) Copyright IBM 2026.
//
// This code is licensed under the Apache License, Version 2.0. You may
// obtain a copy of this license in the LICENSE.txt file in the root directory
// of this source tree or at http://www.apache.org/licenses/LICENSE-2.0.
//
// Any modifications or derivative works of this code must retain this
// copyright notice, and modified files need to carry a notice indicating
// that they have been altered from the originals.

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "common.hpp"

#include "circuit/quantumcircuit.hpp"

using namespace Qiskit;
using namespace Qiskit::circuit;

// ===========================================================================
// The expression builders in circuit/classical/expr.hpp
// ===========================================================================

static int test_expr_value(void) {
    Value v(7);
    if (v.type() != "value" || v.value() != 7) {
        std::cerr << "  expr_value test : type \"" << v.type() << "\" value " << v.value()
            << ", expected \"value\" 7" << std::endl;
        return EqualityError;
    }
    if (v.to_string() != "7") {
        std::cerr << "  expr_value test : rendered \"" << v.to_string()
            << "\", expected \"7\"" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_expr_var(void) {
    auto creg = ClassicalRegister(2, std::string("meas"));
    Var v(creg);
    if (v.type() != "var" || v.creg().name() != "meas") {
        std::cerr << "  expr_var test : type \"" << v.type() << "\" register \""
            << v.creg().name() << "\", expected \"var\" \"meas\"" << std::endl;
        return EqualityError;
    }
    if (v.to_string() != "meas") {
        std::cerr << "  expr_var test : rendered \"" << v.to_string()
            << "\", expected \"meas\"" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_expr_binary_outlives_operands(void) {
    // The operator overloads build their right-hand operand as a local, so the
    // returned node has to own a copy of it. Reading the operands back after
    // the call is what this checks: previously the node held a reference to a
    // destroyed temporary.
    auto creg = ClassicalRegister(2, std::string("c"));
    Binary cond = (creg == (uint_t)3);

    if (cond.Op() != OpType::EQUAL || cond.type() != "binary") {
        std::cerr << "  expr_binary_outlives_operands test : op/type mismatch" << std::endl;
        return EqualityError;
    }
    if (cond.left().type() != "var" || cond.left().creg().name() != "c") {
        std::cerr << "  expr_binary_outlives_operands test : left is \""
            << cond.left().type() << "\" on \"" << cond.left().creg().name()
            << "\", expected a var on \"c\"" << std::endl;
        return EqualityError;
    }
    if (cond.right().type() != "value" || cond.right().value() != 3) {
        std::cerr << "  expr_binary_outlives_operands test : right is \""
            << cond.right().type() << "\" of " << cond.right().value()
            << ", expected a value of 3" << std::endl;
        return EqualityError;
    }
    if (cond.to_string() != "(c == 3)") {
        std::cerr << "  expr_binary_outlives_operands test : rendered \""
            << cond.to_string() << "\", expected \"(c == 3)\"" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_expr_survives_copy(void) {
    // A node must stay readable after the expression it came from is gone,
    // which is what makes returning one from a function safe.
    Binary outer(OpType::EQUAL, Value(1), Value(2));
    {
        auto creg = ClassicalRegister(1, std::string("tmp"));
        Binary inner = (creg == (uint_t)1);
        outer = inner;
    }
    if (outer.left().creg().name() != "tmp" || outer.right().value() != 1) {
        std::cerr << "  expr_survives_copy test : the copy did not keep its operands"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_expr_nested(void) {
    auto a = ClassicalRegister(1, std::string("a"));
    auto b = ClassicalRegister(1, std::string("b"));

    Binary lhs = (a == (uint_t)1);
    Binary rhs = (b == (uint_t)0);
    Binary both = (lhs && rhs);

    if (both.Op() != OpType::LOGIC_AND) {
        std::cerr << "  expr_nested test : expected a LOGIC_AND" << std::endl;
        return EqualityError;
    }
    const std::string expected = "((a == 1) && (b == 0))";
    if (both.to_string() != expected) {
        std::cerr << "  expr_nested test : rendered \"" << both.to_string()
            << "\", expected \"" << expected << "\"" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_expr_unary(void) {
    auto creg = ClassicalRegister(1, std::string("flag"));
    Unary negated = !creg;

    if (negated.Op() != OpType::LOGIC_NOT || negated.type() != "unary") {
        std::cerr << "  expr_unary test : op/type mismatch" << std::endl;
        return EqualityError;
    }
    if (negated.operand().creg().name() != "flag") {
        std::cerr << "  expr_unary test : operand is \""
            << negated.operand().creg().name() << "\", expected \"flag\"" << std::endl;
        return EqualityError;
    }
    if (negated.to_string() != "!flag") {
        std::cerr << "  expr_unary test : rendered \"" << negated.to_string()
            << "\", expected \"!flag\"" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_expr_register_comparisons(void) {
    auto a = ClassicalRegister(2, std::string("a"));
    auto b = ClassicalRegister(2, std::string("b"));

    const OpType ops[] = {OpType::EQUAL, OpType::NOT_EQUAL, OpType::LESS,
                          OpType::LESS_EQUAL, OpType::GREATER, OpType::GREATER_EQUAL};
    const char *names[] = {"==", "!=", "<", "<=", ">", ">="};

    std::vector<Binary> built;
    built.push_back(a == b);
    built.push_back(a != b);
    built.push_back(a < b);
    built.push_back(a <= b);
    built.push_back(a > b);
    built.push_back(a >= b);

    for (std::size_t i = 0; i < built.size(); i++) {
        if (built[i].Op() != ops[i]) {
            std::cerr << "  expr_register_comparisons test : operator " << names[i]
                << " built the wrong OpType" << std::endl;
            return EqualityError;
        }
        const std::string expected = std::string("(a ") + names[i] + " b)";
        if (built[i].to_string() != expected) {
            std::cerr << "  expr_register_comparisons test : rendered \""
                << built[i].to_string() << "\", expected \"" << expected << "\""
                << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

// ===========================================================================
// The read-only ExprView over a C-API expression
// ===========================================================================

static int test_expr_view_empty(void) {
    // An empty view must report itself as empty rather than calling into the
    // C-API with a null pointer.
    ExprView view;
    if (view.valid() || view.get() != nullptr) {
        std::cerr << "  expr_view_empty test : a default view is not empty" << std::endl;
        return EqualityError;
    }
    if (view.kind() != ExprNodeKind::Unknown) {
        std::cerr << "  expr_view_empty test : a default view has a kind" << std::endl;
        return EqualityError;
    }
    if (view.to_string() != "<none>") {
        std::cerr << "  expr_view_empty test : rendered \"" << view.to_string()
            << "\", expected \"<none>\"" << std::endl;
        return EqualityError;
    }
    if (!view.children().empty()) {
        std::cerr << "  expr_view_empty test : a default view has children" << std::endl;
        return EqualityError;
    }
    if (view.type().type() != ExprType::Unknown || view.is_constant()) {
        std::cerr << "  expr_view_empty test : a default view reports a type or constness"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_expr_view_rejects_wrong_kind(void) {
    // Every kind-specific accessor must throw rather than reach the C-API,
    // which panics (aborting the process) when given the wrong node kind.
    ExprView view;
    const char *names[] = {"unary_op", "operand", "binary_op", "left", "right",
                           "cast_operand", "is_implicit_cast", "target", "index",
                           "value_uint", "value_bool", "value_float",
                           "value_duration", "var_name", "stretch_name"};
    bool threw[15];
    for (std::size_t i = 0; i < 15; i++) {
        threw[i] = false;
    }

    try { view.unary_op(); }        catch (const std::logic_error &) { threw[0] = true; }
    try { view.operand(); }         catch (const std::logic_error &) { threw[1] = true; }
    try { view.binary_op(); }       catch (const std::logic_error &) { threw[2] = true; }
    try { view.left(); }            catch (const std::logic_error &) { threw[3] = true; }
    try { view.right(); }           catch (const std::logic_error &) { threw[4] = true; }
    try { view.cast_operand(); }    catch (const std::logic_error &) { threw[5] = true; }
    try { view.is_implicit_cast(); }catch (const std::logic_error &) { threw[6] = true; }
    try { view.target(); }          catch (const std::logic_error &) { threw[7] = true; }
    try { view.index(); }           catch (const std::logic_error &) { threw[8] = true; }
    try { view.value_uint(); }      catch (const std::logic_error &) { threw[9] = true; }
    try { view.value_bool(); }      catch (const std::logic_error &) { threw[10] = true; }
    try { view.value_float(); }     catch (const std::logic_error &) { threw[11] = true; }
    try { view.value_duration(); }  catch (const std::logic_error &) { threw[12] = true; }
    try { view.var_name(); }        catch (const std::logic_error &) { threw[13] = true; }
    try { view.stretch_name(); }    catch (const std::logic_error &) { threw[14] = true; }

    for (std::size_t i = 0; i < 15; i++) {
        if (!threw[i]) {
            std::cerr << "  expr_view_rejects_wrong_kind test : " << names[i]
                << " did not throw on an empty view" << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_expr_type_info(void) {
    QkExprTypeInfo info;
    info.ty = QkExprType_Uint;
    info.width = 8;
    ExprTypeInfo uint8(info);
    if (uint8.type() != ExprType::Uint || uint8.width() != 8
        || uint8.to_string() != "uint[8]") {
        std::cerr << "  expr_type_info test : uint[8] was read as \""
            << uint8.to_string() << "\"" << std::endl;
        return EqualityError;
    }

    info.ty = QkExprType_Bool;
    info.width = 0;
    ExprTypeInfo boolean(info);
    if (boolean.type() != ExprType::Bool || boolean.to_string() != "bool") {
        std::cerr << "  expr_type_info test : bool was read as \""
            << boolean.to_string() << "\"" << std::endl;
        return EqualityError;
    }

    info.ty = QkExprType_Float;
    ExprTypeInfo floating(info);
    if (floating.type() != ExprType::Float || floating.to_string() != "float") {
        std::cerr << "  expr_type_info test : float was read as \""
            << floating.to_string() << "\"" << std::endl;
        return EqualityError;
    }

    info.ty = QkExprType_Duration;
    ExprTypeInfo duration(info);
    if (duration.type() != ExprType::Duration || duration.to_string() != "duration") {
        std::cerr << "  expr_type_info test : duration was read as \""
            << duration.to_string() << "\"" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_expr_duration_value(void) {
    // A dt duration is stored in the integer arm of the union and every other
    // unit in the floating point arm, so both have to be read correctly.
    QkDurationInfo info;
    info.ty = QkDurationType_Dt;
    info.value.dt = 100;
    DurationValue cycles(info);
    if (!cycles.is_dt() || cycles.dt() != 100 || cycles.to_string() != "100dt") {
        std::cerr << "  expr_duration_value test : dt was read as \""
            << cycles.to_string() << "\"" << std::endl;
        return EqualityError;
    }

    info.ty = QkDurationType_Ns;
    info.value.time = 2.5;
    DurationValue nanos(info);
    if (nanos.is_dt() || nanos.time() != 2.5 || nanos.to_string() != "2.5ns") {
        std::cerr << "  expr_duration_value test : ns was read as \""
            << nanos.to_string() << "\"" << std::endl;
        return EqualityError;
    }

    const QkDurationType types[] = {QkDurationType_Ps, QkDurationType_Us,
                                    QkDurationType_Ms, QkDurationType_S};
    const char *names[] = {"ps", "us", "ms", "s"};
    for (std::size_t i = 0; i < 4; i++) {
        info.ty = types[i];
        info.value.time = 1.0;
        DurationValue d(info);
        if (std::string(d.unit_name()) != names[i]) {
            std::cerr << "  expr_duration_value test : unit " << names[i]
                << " was read as \"" << d.unit_name() << "\"" << std::endl;
            return EqualityError;
        }
    }
    return Ok;
}

static int test_expr_view_operator_names(void) {
    if (std::string(ExprView::unary_op_name(UnaryOpType::LogicNot)) != "!"
        || std::string(ExprView::unary_op_name(UnaryOpType::BitNot)) != "~"
        || std::string(ExprView::unary_op_name(UnaryOpType::Negate)) != "-") {
        std::cerr << "  expr_view_operator_names test : a unary symbol is wrong" << std::endl;
        return EqualityError;
    }
    if (std::string(ExprView::binary_op_name(BinaryOpType::Equal)) != "=="
        || std::string(ExprView::binary_op_name(BinaryOpType::LogicAnd)) != "&&"
        || std::string(ExprView::binary_op_name(BinaryOpType::ShiftRight)) != ">>"
        || std::string(ExprView::binary_op_name(BinaryOpType::Div)) != "/") {
        std::cerr << "  expr_view_operator_names test : a binary symbol is wrong" << std::endl;
        return EqualityError;
    }
    return Ok;
}

static int test_control_flow_instruction_on_plain_circuit(void) {
    // A circuit with no control flow must not yield a condition. Reading
    // instruction 0 of such a circuit is undefined in the C-API, so this only
    // checks the guards on an unread instruction.
    ControlFlowInstruction inst(nullptr, 0);
    if (inst.valid()) {
        std::cerr << "  control_flow_instruction test : a null circuit produced an instruction"
            << std::endl;
        return EqualityError;
    }
    if (inst.condition_type() != ConditionType::Unknown) {
        std::cerr << "  control_flow_instruction test : an unread instruction has a condition type"
            << std::endl;
        return EqualityError;
    }
    if (inst.condition_expr().valid() || inst.switch_target_expr().valid()) {
        std::cerr << "  control_flow_instruction test : an unread instruction yielded an expression"
            << std::endl;
        return EqualityError;
    }
    return Ok;
}

#if defined(_WIN32)
int test_expr(int argc, char** const argv) {
#else
int test_expr(int argc, char** argv) {
#endif
    int num_failed = 0;
    num_failed += RUN_TEST(test_expr_value);
    num_failed += RUN_TEST(test_expr_var);
    num_failed += RUN_TEST(test_expr_binary_outlives_operands);
    num_failed += RUN_TEST(test_expr_survives_copy);
    num_failed += RUN_TEST(test_expr_nested);
    num_failed += RUN_TEST(test_expr_unary);
    num_failed += RUN_TEST(test_expr_register_comparisons);
    num_failed += RUN_TEST(test_expr_view_empty);
    num_failed += RUN_TEST(test_expr_view_rejects_wrong_kind);
    num_failed += RUN_TEST(test_expr_type_info);
    num_failed += RUN_TEST(test_expr_duration_value);
    num_failed += RUN_TEST(test_expr_view_operator_names);
    num_failed += RUN_TEST(test_control_flow_instruction_on_plain_circuit);

    std::cerr << "=== Number of failed subtests: " << num_failed << std::endl;
    return num_failed;
}
