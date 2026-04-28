#include "interpreter/interpreter.hpp"
#include <iostream>
#include <sstream>
#include <cassert>

namespace lang {

// ── construction ──────────────────────────────────────────────────────────────

Interpreter::Interpreter(bool verbose)
    : globals_(std::make_shared<Environment>())
    , verbose_(verbose)
{
    register_builtins();
}

void Interpreter::register_builtins() {
    // print(value) — displays any value
    auto print_fn = std::make_shared<Callable>();
    print_fn->param_names = { "value" };
    // Body is empty — handled as a native override in call_value via name lookup
    // We store a special marker hash instead.
    // Simpler: wrap native functions as HashInstances with a flag.
    // Cleanest for this codebase: store a Callable with no body and intercept
    // in call_callable based on a sentinel.  We use a dedicated NativeCallable
    // approach via std::function stored in a subclass.
    // For simplicity here we just register print as a Value::Callable whose
    // body is empty — then intercept by param_names == {"__native_print"}.
    auto print_callable = std::make_shared<Callable>();
    print_callable->param_names = { "__native_print__value" };
    print_callable->closure     = globals_;
    globals_->define("print", Value::from(print_callable));
}

// ── program entry ─────────────────────────────────────────────────────────────

void Interpreter::run(const ast::Program& program) {
    for (auto& stmt : program.statements)
        exec(*stmt, globals_);
}

// ── statement execution ───────────────────────────────────────────────────────

ValuePtr Interpreter::exec(const ast::Stmt& stmt, std::shared_ptr<Environment> env) {
    if (auto* s = dynamic_cast<const ast::DeclStmt*>(&stmt))
        return exec_decl(*s, env);
    if (auto* s = dynamic_cast<const ast::AssignStmt*>(&stmt))
        return exec_assign(*s, env);
    if (auto* s = dynamic_cast<const ast::ExprStmt*>(&stmt))
        return exec_expr_stmt(*s, env);
    throw RuntimeError("Unknown statement type");
}

// Execute a block and return the value of the last statement.
ValuePtr Interpreter::exec_block(const std::vector<ast::StmtPtr>& stmts,
                                  std::shared_ptr<Environment> env) {
    ValuePtr last = Value::nil();
    for (auto& s : stmts)
        last = exec(*s, env);
    return last;
}

ValuePtr Interpreter::exec_decl(const ast::DeclStmt& s,
                                 std::shared_ptr<Environment> env) {
    ValuePtr val = eval(*s.value, env);
    env->define(s.name, val);
    return val;
}

ValuePtr Interpreter::exec_assign(const ast::AssignStmt& s,
                                   std::shared_ptr<Environment> env) {
    ValuePtr val = eval(*s.value, env);

    if (auto* id = dynamic_cast<const ast::Identifier*>(s.target.get())) {
        env->set(id->name, val);
        return val;
    }
    if (auto* mem = dynamic_cast<const ast::MemberExpr*>(s.target.get())) {
        ValuePtr obj = eval(*mem->object, env);
        if (!obj->is_hash())
            throw RuntimeError("Member assignment on non-hash value");
        obj->as_hash()->set(mem->member, val);
        return val;
    }
    throw RuntimeError("Invalid assignment target");
}

ValuePtr Interpreter::exec_expr_stmt(const ast::ExprStmt& s,
                                      std::shared_ptr<Environment> env) {
    ValuePtr result = eval(*s.expression, env);
    if (verbose_ && result && !result->is_nil())
        std::cout << "  >> " << result->to_display_string() << "\n";
    return result;
}

// ── expression evaluation ─────────────────────────────────────────────────────

ValuePtr Interpreter::eval(const ast::Expr& expr,
                            std::shared_ptr<Environment> env) {
    if (auto* e = dynamic_cast<const ast::IntLiteral*>(&expr))
        return eval_int(*e, env);
    if (auto* e = dynamic_cast<const ast::StringLiteral*>(&expr))
        return eval_string(*e, env);
    if (auto* e = dynamic_cast<const ast::Identifier*>(&expr))
        return eval_ident(*e, env);
    if (auto* e = dynamic_cast<const ast::BinaryExpr*>(&expr))
        return eval_binary(*e, env);
    if (auto* e = dynamic_cast<const ast::UnaryExpr*>(&expr))
        return eval_unary(*e, env);
    if (auto* e = dynamic_cast<const ast::MemberExpr*>(&expr))
        return eval_member(*e, env);
    if (auto* e = dynamic_cast<const ast::CallExpr*>(&expr))
        return eval_call(*e, env);
    if (auto* e = dynamic_cast<const ast::CondExpr*>(&expr))
        return eval_cond(*e, env);
    if (auto* e = dynamic_cast<const ast::HashLiteral*>(&expr))
        return eval_hash_lit(*e, env);
    if (auto* e = dynamic_cast<const ast::CallableExpr*>(&expr))
        return eval_callable(*e, env);
    throw RuntimeError("Unknown expression type");
}

ValuePtr Interpreter::eval_int(const ast::IntLiteral& e,
                                std::shared_ptr<Environment>) {
    return Value::from(e.value);
}

ValuePtr Interpreter::eval_string(const ast::StringLiteral& e,
                                   std::shared_ptr<Environment>) {
    return Value::from(e.value);
}

ValuePtr Interpreter::eval_ident(const ast::Identifier& e,
                                  std::shared_ptr<Environment> env) {
    return env->get(e.name);
}

ValuePtr Interpreter::eval_binary(const ast::BinaryExpr& e,
                                   std::shared_ptr<Environment> env) {
    ValuePtr lv = eval(*e.left,  env);
    ValuePtr rv = eval(*e.right, env);
    const std::string& op = e.op;

    // String concatenation when either side is a string
    if (op == "+" && (lv->is_string() || rv->is_string()))
        return Value::from(lv->to_display_string() + rv->to_display_string());

    // Integer arithmetic
    if (lv->is_int() && rv->is_int()) {
        int64_t a = lv->as_int(), b = rv->as_int();
        if (op == "+")  return Value::from(a + b);
        if (op == "-")  return Value::from(a - b);
        if (op == "*")  return Value::from(a * b);
        if (op == "/")  { if (!b) throw RuntimeError("Division by zero");
                          return Value::from(a / b); }
        if (op == "%")  { if (!b) throw RuntimeError("Modulo by zero");
                          return Value::from(a % b); }
        if (op == "==") return Value::from(static_cast<int64_t>(a == b));
        if (op == "!=") return Value::from(static_cast<int64_t>(a != b));
        if (op == "<")  return Value::from(static_cast<int64_t>(a <  b));
        if (op == "<=") return Value::from(static_cast<int64_t>(a <= b));
        if (op == ">")  return Value::from(static_cast<int64_t>(a >  b));
        if (op == ">=") return Value::from(static_cast<int64_t>(a >= b));
    }

    // Generic equality
    if (op == "==") return Value::from(static_cast<int64_t>(
                        lv->kind() == rv->kind() &&
                        lv->to_display_string() == rv->to_display_string()));
    if (op == "!=") return Value::from(static_cast<int64_t>(
                        lv->kind() != rv->kind() ||
                        lv->to_display_string() != rv->to_display_string()));

    throw RuntimeError("Unsupported operand types for '" + op + "'");
}

ValuePtr Interpreter::eval_unary(const ast::UnaryExpr& e,
                                  std::shared_ptr<Environment> env) {
    ValuePtr v = eval(*e.operand, env);
    if (e.op == "-" && v->is_int()) return Value::from(-v->as_int());
    throw RuntimeError("Unsupported unary operator '" + e.op + "'");
}

ValuePtr Interpreter::eval_member(const ast::MemberExpr& e,
                                   std::shared_ptr<Environment> env) {
    ValuePtr obj = eval(*e.object, env);
    if (!obj->is_hash())
        throw RuntimeError("Member access on non-hash value '" + e.member + "'");
    return obj->as_hash()->get(e.member);
}

ValuePtr Interpreter::eval_call(const ast::CallExpr& e,
                                 std::shared_ptr<Environment> env) {
    // Collect arguments
    std::vector<ValuePtr> args;
    args.reserve(e.args.size());
    for (auto& a : e.args)
        args.push_back(eval(*a, env));

    // Capture receiver for method calls (self binding)
    ValuePtr receiver = nullptr;
    if (auto* mem = dynamic_cast<const ast::MemberExpr*>(e.callee.get()))
        receiver = eval(*mem->object, env);

    ValuePtr callee = eval(*e.callee, env);
    return call_value(callee, args, receiver);
}

ValuePtr Interpreter::eval_cond(const ast::CondExpr& e,
                                 std::shared_ptr<Environment> env) {
    ValuePtr cond = eval(*e.condition, env);
    auto branch_env = std::make_shared<Environment>(env);

    if (cond->is_truthy())
        return exec_block(e.then_block, branch_env);
    else
        return exec_block(e.else_block, branch_env);
}

ValuePtr Interpreter::eval_hash_lit(const ast::HashLiteral& e,
                                     std::shared_ptr<Environment> env) {
    auto instance = std::make_shared<HashInstance>();
    for (auto& m : e.members)
        instance->members[m.name] = eval(*m.value, env);
    return Value::from(instance);
}

ValuePtr Interpreter::eval_callable(const ast::CallableExpr& e,
                                     std::shared_ptr<Environment> env) {
    auto fn = std::make_shared<Callable>();
    for (auto& p : e.params)
        fn->param_names.push_back(p.name);
    fn->body    = e.body;
    fn->closure = env;
    return Value::from(fn);
}

// ── call dispatch ─────────────────────────────────────────────────────────────

ValuePtr Interpreter::call_value(ValuePtr callee,
                                  const std::vector<ValuePtr>& args,
                                  ValuePtr receiver) {
    // ── Hash constructor call ────────────────────────────────────────────────
    if (callee->is_hash()) {
        HashPtr tmpl     = callee->as_hash();
        HashPtr instance = tmpl->clone();

        if (instance->has("()")) {
            ValuePtr ctor = instance->get("()");
            if (ctor->is_callable())
                call_callable(*ctor->as_callable(), args, Value::from(instance));
        }
        return Value::from(instance);
    }

    // ── Callable call ────────────────────────────────────────────────────────
    if (callee->is_callable())
        return call_callable(*callee->as_callable(), args, receiver);

    throw RuntimeError("Cannot call a non-callable value: " +
                       callee->to_display_string());
}

ValuePtr Interpreter::call_callable(const Callable& fn,
                                     const std::vector<ValuePtr>& args,
                                     ValuePtr self_val) {
    // ── Native built-in: print ───────────────────────────────────────────────
    if (fn.param_names.size() == 1 &&
        fn.param_names[0] == "__native_print__value") {
        if (!args.empty())
            std::cout << args[0]->to_display_string() << "\n";
        return Value::nil();
    }

    if (args.size() != fn.param_names.size()) {
        throw RuntimeError("Expected " + std::to_string(fn.param_names.size()) +
                           " argument(s), got " + std::to_string(args.size()));
    }

    auto call_env = std::make_shared<Environment>(fn.closure);

    for (size_t i = 0; i < fn.param_names.size(); ++i)
        call_env->define(fn.param_names[i], args[i]);

    if (self_val)
        call_env->define("self", self_val);

    try {
        return exec_block(fn.body, call_env);
    } catch (ReturnSignal& ret) {
        return ret.value;
    }
}

} // namespace lang
