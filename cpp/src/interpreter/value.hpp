#pragma once
#include "ast/nodes.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace lang {

class Environment;

// ── forward declarations ──────────────────────────────────────────────────────

struct Value;
struct HashInstance;
struct Callable;
struct NativeCallable;

using ValuePtr    = std::shared_ptr<Value>;
using HashPtr     = std::shared_ptr<HashInstance>;
using CallablePtr = std::shared_ptr<Callable>;
using NativeCallablePtr = std::shared_ptr<NativeCallable>;

// ── HashInstance ──────────────────────────────────────────────────────────────
//
//  Both a live object and a "type template" — calling a hash clones it and
//  invokes the "()" member (constructor) if one is present.

struct HashInstance {
    std::unordered_map<std::string, ValuePtr> members;

    HashPtr  clone()                                   const;
    ValuePtr get(const std::string& name)              const;
    void     set(const std::string& name, ValuePtr v);
    bool     has(const std::string& name)              const;
};

// ── Callable ──────────────────────────────────────────────────────────────────
//
//  A first-class function value.  Carries its body AST and the lexical
//  environment it was defined in (closure).

struct Callable {
    std::vector<std::string>           param_names;
    std::vector<ast::StmtPtr>          body;
    std::shared_ptr<Environment>       closure;
};

struct NativeCallable {
    std::string name;
    std::shared_ptr<void> library_handle;
    std::function<ValuePtr(const std::vector<ValuePtr>&)> invoke;
};

// ── Value ─────────────────────────────────────────────────────────────────────

struct Value {
    enum class Kind { Nil, Integer, String, Hash, Callable, NativeCallable };

    using Data = std::variant<
        std::monostate,   // Nil
        int64_t,          // Integer
        std::string,      // String
        HashPtr,          // Hash
        CallablePtr,      // Callable
        NativeCallablePtr // NativeCallable
    >;

    Data data;

    Value()                          : data(std::monostate{}) {}
    explicit Value(int64_t v)        : data(v) {}
    explicit Value(std::string v)    : data(std::move(v)) {}
    explicit Value(HashPtr v)        : data(std::move(v)) {}
    explicit Value(CallablePtr v)    : data(std::move(v)) {}
    explicit Value(NativeCallablePtr v) : data(std::move(v)) {}

    Kind kind() const { return static_cast<Kind>(data.index()); }

    bool is_nil()      const { return kind() == Kind::Nil; }
    bool is_int()      const { return kind() == Kind::Integer; }
    bool is_string()   const { return kind() == Kind::String; }
    bool is_hash()     const { return kind() == Kind::Hash; }
    bool is_callable() const { return kind() == Kind::Callable; }
    bool is_native_callable() const { return kind() == Kind::NativeCallable; }

    bool is_truthy() const;

    int64_t            as_int()      const { return std::get<int64_t>(data); }
    const std::string& as_string()   const { return std::get<std::string>(data); }
    HashPtr            as_hash()     const { return std::get<HashPtr>(data); }
    CallablePtr        as_callable() const { return std::get<CallablePtr>(data); }
    NativeCallablePtr  as_native_callable() const { return std::get<NativeCallablePtr>(data); }

    std::string to_display_string() const;

    // ── factory helpers ───────────────────────────────────────────────────────
    static ValuePtr nil()                  { return std::make_shared<Value>(); }
    static ValuePtr from(int64_t v)        { return std::make_shared<Value>(v); }
    static ValuePtr from(std::string v)    { return std::make_shared<Value>(std::move(v)); }
    static ValuePtr from(HashPtr v)        { return std::make_shared<Value>(std::move(v)); }
    static ValuePtr from(CallablePtr v)    { return std::make_shared<Value>(std::move(v)); }
    static ValuePtr from(NativeCallablePtr v) { return std::make_shared<Value>(std::move(v)); }
};

} // namespace lang
