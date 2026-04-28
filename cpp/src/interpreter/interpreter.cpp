#include "interpreter/interpreter.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include <dlfcn.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cassert>

namespace lang {

namespace {

using SharedLibraryHandle = std::shared_ptr<void>;

SharedLibraryHandle open_library(const std::string& path) {
    void* handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!handle)
        throw RuntimeError("dlopen failed for '" + path + "': " + (dlerror() ? dlerror() : "unknown error"));
    return SharedLibraryHandle(handle, [](void* h) {
        if (h) dlclose(h);
    });
}

std::string get_required_string(HashPtr hash, const std::string& key) {
    if (!hash->has(key))
        throw RuntimeError("Missing required foreign spec field '" + key + "'");
    ValuePtr value = hash->get(key);
    if (!value->is_string())
        throw RuntimeError("Foreign spec field '" + key + "' must be a string");
    return value->as_string();
}

int count_params(HashPtr params_hash) {
    int count = 0;
    while (true) {
        std::string name = "p" + std::to_string(count + 1);
        if (!params_hash->has(name)) break;
        ++count;
    }
    return count;
}

std::vector<std::string> read_param_types(HashPtr spec_hash) {
    if (!spec_hash->has("params")) return {};
    ValuePtr params_val = spec_hash->get("params");
    if (!params_val->is_hash())
        throw RuntimeError("Foreign spec field 'params' must be a hash");
    HashPtr params_hash = params_val->as_hash();
    int arity = count_params(params_hash);
    std::vector<std::string> result;
    result.reserve(arity);
    for (int i = 1; i <= arity; ++i) {
        ValuePtr type_val = params_hash->get("p" + std::to_string(i));
        if (!type_val->is_string())
            throw RuntimeError("Foreign param type must be a string");
        result.push_back(type_val->as_string());
    }
    return result;
}

void require_arity(const std::vector<ValuePtr>& args, size_t expected) {
    if (args.size() != expected) {
        throw RuntimeError("Native call expected " + std::to_string(expected) +
                           " argument(s), got " + std::to_string(args.size()));
    }
}

int64_t require_int(ValuePtr arg, const std::string& type_name) {
    if (!arg->is_int())
        throw RuntimeError("Native argument for type '" + type_name + "' must be an int");
    return arg->as_int();
}

double require_double(ValuePtr arg, const std::string& type_name) {
    if (arg->is_double()) return arg->as_double();
    if (arg->is_int()) return static_cast<double>(arg->as_int());
    throw RuntimeError("Native argument for type '" + type_name + "' must be numeric");
}

const char* require_cstring(ValuePtr arg) {
    if (!arg->is_string())
        throw RuntimeError("Native argument for type 'cstring' must be a string");
    return arg->as_string().c_str();
}

void require_null_ptr_out(ValuePtr arg) {
    if (!(arg->is_nil() || arg->is_hash()))
        throw RuntimeError("Native argument for type 'ptr_out' must be {} / nil");
}

} // namespace

// --- construction -------------------------------------------------------

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

// --- program entry --------------------------------------------------------------

void Interpreter::run(const ast::Program& program) {
    for (auto& stmt : program.statements)
        exec(*stmt, globals_);
}

// --- statement execution --------------------------------------------------------------

ValuePtr Interpreter::exec(const ast::Stmt& stmt, std::shared_ptr<Environment> env) {
    if (auto* s = dynamic_cast<const ast::DeclStmt*>(&stmt))
        return exec_decl(*s, env);
    if (auto* s = dynamic_cast<const ast::AssignStmt*>(&stmt))
        return exec_assign(*s, env);
    if (auto* s = dynamic_cast<const ast::ExprStmt*>(&stmt))
        return exec_expr_stmt(*s, env);
    if (auto* s = dynamic_cast<const ast::ImportStmt*>(&stmt))
        return exec_import(*s, env);
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
    if (auto* dyn = dynamic_cast<const ast::DynMemberExpr*>(s.target.get())) {
        ValuePtr obj = eval(*dyn->object, env);
        ValuePtr key = eval(*dyn->key, env);
        if (!obj->is_hash())
            throw RuntimeError("Dynamic member assignment on non-hash value");
        if (!key->is_string())
            throw RuntimeError("Dynamic member key must be a string");
        obj->as_hash()->set(key->as_string(), val);
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

// --- expression evaluation --------------------------------------------------------------

ValuePtr Interpreter::eval(const ast::Expr& expr,
                            std::shared_ptr<Environment> env) {
    if (auto* e = dynamic_cast<const ast::IntLiteral*>(&expr))
        return eval_int(*e, env);
    if (auto* e = dynamic_cast<const ast::FloatLiteral*>(&expr))
        return eval_float(*e, env);
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
    if (auto* e = dynamic_cast<const ast::DynMemberExpr*>(&expr))
        return eval_dyn_member(*e, env);
    if (auto* e = dynamic_cast<const ast::IterExpr*>(&expr))
        return eval_iter(*e, env);
    throw RuntimeError("Unknown expression type");
}

ValuePtr Interpreter::eval_int(const ast::IntLiteral& e,
                                std::shared_ptr<Environment>) {
    return Value::from(e.value);
}

ValuePtr Interpreter::eval_float(const ast::FloatLiteral& e,
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

    // Mixed numeric arithmetic/comparison (exact equality)
    if (lv->is_numeric() && rv->is_numeric()) {
        auto to_double = [](ValuePtr v) -> double {
            return v->is_double() ? v->as_double() : static_cast<double>(v->as_int());
        };
        double a = to_double(lv), b = to_double(rv);
        if (op == "+")  return Value::from(a + b);
        if (op == "-")  return Value::from(a - b);
        if (op == "*")  return Value::from(a * b);
        if (op == "/")  { if (b == 0.0) throw RuntimeError("Division by zero");
                           return Value::from(a / b); }
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
    if (e.op == "-" && v->is_double()) return Value::from(-v->as_double());
    if (e.op == "$$") return bind_foreign_function(v);
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

ValuePtr Interpreter::eval_dyn_member(const ast::DynMemberExpr& e,
                                       std::shared_ptr<Environment> env) {
    ValuePtr obj = eval(*e.object, env);
    ValuePtr key = eval(*e.key, env);
    if (!obj->is_hash())
        throw RuntimeError("Dynamic member access on non-hash value");
    if (!key->is_string())
        throw RuntimeError("Dynamic member key must be a string");
    return obj->as_hash()->get(key->as_string());
}

ValuePtr Interpreter::eval_iter(const ast::IterExpr& e,
                                 std::shared_ptr<Environment> env) {
    ValuePtr obj = eval(*e.object, env);
    if (!obj->is_hash())
        throw RuntimeError(">> requires a hash on the left-hand side");
    ValuePtr fn_val = eval(*e.callable, env);
    if (!fn_val->is_callable())
        throw RuntimeError(">> requires a callable on the right-hand side");
    const Callable& fn = *fn_val->as_callable();
    if (fn.param_names.size() != 2)
        throw RuntimeError(">> callable must take exactly two parameters (key, value)");
    for (auto& [k, v] : obj->as_hash()->members) {
        if (k == "()") continue;   // skip constructor slot
        call_callable(fn, { Value::from(k), v }, nullptr);
    }
    return Value::nil();
}

ValuePtr Interpreter::exec_import(const ast::ImportStmt& s,
                                   std::shared_ptr<Environment> env) {
    std::ifstream f(s.path);
    if (!f)
        throw RuntimeError("Cannot open import file: '" + s.path + "'");
    std::ostringstream buf;
    buf << f.rdbuf();

    Lexer  lexer(buf.str());
    Parser parser(lexer.tokenize());
    auto   program = parser.parse();

    // Run imported file in the caller's environment so all definitions
    // become visible in that scope.
    for (auto& stmt : program.statements)
        exec(*stmt, env);

    return Value::nil();
}

// --- call dispatch ------------------------------------------------------

ValuePtr Interpreter::call_value(ValuePtr callee,
                                  const std::vector<ValuePtr>& args,
                                  ValuePtr receiver) {

    // --- Hash constructor call ------------------------------------------------

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

    // --- Callable call --------------------------------------------------

    if (callee->is_callable())
        return call_callable(*callee->as_callable(), args, receiver);

    if (callee->is_native_callable())
        return callee->as_native_callable()->invoke(args);

    throw RuntimeError("Cannot call a non-callable value: " +
                       callee->to_display_string());
}

ValuePtr Interpreter::bind_foreign_function(ValuePtr spec) {
    if (!spec->is_hash())
        throw RuntimeError("$$ expects a hash spec");

    HashPtr spec_hash = spec->as_hash();
    const std::string lib = get_required_string(spec_hash, "lib");
    const std::string abi = get_required_string(spec_hash, "abi");
    const std::string symbol = get_required_string(spec_hash, "symbol");
    const std::string return_type = get_required_string(spec_hash, "returns");
    const std::vector<std::string> param_types = read_param_types(spec_hash);

    if (abi != "c")
        throw RuntimeError("Only abi='c' is supported by $$");

    SharedLibraryHandle handle = open_library(lib);
    dlerror();
    void* raw_symbol = dlsym(handle.get(), symbol.c_str());
    const char* err = dlerror();
    if (err || !raw_symbol)
        throw RuntimeError("dlsym failed for '" + symbol + "': " + std::string(err ? err : "unknown error"));

    auto native = std::make_shared<NativeCallable>();
    native->name = symbol;
    native->library_handle = handle;

    if (param_types.empty() && return_type == "int") {
        using Fn = int (*)();
        Fn fn = reinterpret_cast<Fn>(raw_symbol);
        native->invoke = [fn](const std::vector<ValuePtr>& args) {
            require_arity(args, 0);
            return Value::from(static_cast<int64_t>(fn()));
        };
    } else if (param_types.size() == 1 && param_types[0] == "int" && return_type == "int") {
        using Fn = int (*)(int);
        Fn fn = reinterpret_cast<Fn>(raw_symbol);
        native->invoke = [fn](const std::vector<ValuePtr>& args) {
            require_arity(args, 1);
            return Value::from(static_cast<int64_t>(fn(static_cast<int>(require_int(args[0], "int")))));
        };
    } else if (param_types.size() == 1 && param_types[0] == "uint" && return_type == "void") {
        using Fn = void (*)(unsigned int);
        Fn fn = reinterpret_cast<Fn>(raw_symbol);
        native->invoke = [fn](const std::vector<ValuePtr>& args) {
            require_arity(args, 1);
            fn(static_cast<unsigned int>(require_int(args[0], "uint")));
            return Value::nil();
        };
    } else if (param_types.size() == 1 && param_types[0] == "cstring" && return_type == "int") {
        using Fn = int (*)(const char*);
        Fn fn = reinterpret_cast<Fn>(raw_symbol);
        native->invoke = [fn](const std::vector<ValuePtr>& args) {
            require_arity(args, 1);
            return Value::from(static_cast<int64_t>(fn(require_cstring(args[0]))));
        };
    } else if (param_types.size() == 1 && param_types[0] == "cstring" && return_type == "usize") {
        using Fn = size_t (*)(const char*);
        Fn fn = reinterpret_cast<Fn>(raw_symbol);
        native->invoke = [fn](const std::vector<ValuePtr>& args) {
            require_arity(args, 1);
            return Value::from(static_cast<int64_t>(fn(require_cstring(args[0]))));
        };
    } else if (param_types.size() == 2 && param_types[0] == "cstring" && param_types[1] == "cstring" && return_type == "int") {
        using Fn = int (*)(const char*, const char*);
        Fn fn = reinterpret_cast<Fn>(raw_symbol);
        native->invoke = [fn](const std::vector<ValuePtr>& args) {
            require_arity(args, 2);
            return Value::from(static_cast<int64_t>(fn(require_cstring(args[0]), require_cstring(args[1]))));
        };
    } else if (param_types.size() == 3 && param_types[0] == "cstring" && param_types[1] == "cstring" && param_types[2] == "usize" && return_type == "int") {
        using Fn = int (*)(const char*, const char*, size_t);
        Fn fn = reinterpret_cast<Fn>(raw_symbol);
        native->invoke = [fn](const std::vector<ValuePtr>& args) {
            require_arity(args, 3);
            return Value::from(static_cast<int64_t>(
                fn(require_cstring(args[0]), require_cstring(args[1]),
                   static_cast<size_t>(require_int(args[2], "usize")))));
        };
    } else if (param_types.size() == 3 && param_types[0] == "cstring" && param_types[1] == "ptr_out" && param_types[2] == "int" && return_type == "long") {
        using Fn = long (*)(const char*, char**, int);
        Fn fn = reinterpret_cast<Fn>(raw_symbol);
        native->invoke = [fn](const std::vector<ValuePtr>& args) {
            require_arity(args, 3);
            require_null_ptr_out(args[1]);
            return Value::from(static_cast<int64_t>(
                fn(require_cstring(args[0]), nullptr,
                   static_cast<int>(require_int(args[2], "int")))));
        };
    } else if (param_types.size() == 1 && param_types[0] == "double" && return_type == "double") {
        using Fn = double (*)(double);
        Fn fn = reinterpret_cast<Fn>(raw_symbol);
        native->invoke = [fn](const std::vector<ValuePtr>& args) {
            require_arity(args, 1);
            return Value::from(fn(require_double(args[0], "double")));
        };
    } else if (param_types.size() == 2 && param_types[0] == "double" && param_types[1] == "double" && return_type == "double") {
        using Fn = double (*)(double, double);
        Fn fn = reinterpret_cast<Fn>(raw_symbol);
        native->invoke = [fn](const std::vector<ValuePtr>& args) {
            require_arity(args, 2);
            return Value::from(fn(require_double(args[0], "double"),
                                  require_double(args[1], "double")));
        };
    } else {
        throw RuntimeError("Unsupported foreign signature for symbol '" + symbol + "'");
    }

    return Value::from(native);
}

ValuePtr Interpreter::call_callable(const Callable& fn,
                                     const std::vector<ValuePtr>& args,
                                     ValuePtr self_val) {
    // --- Native built-in: print --------------------------------------------------

    if (fn.param_names.size() == 1 &&
        fn.param_names[0] == "__native_print__value") {
        if (!args.empty())
            std::cout << args[0]->to_display_string();
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
