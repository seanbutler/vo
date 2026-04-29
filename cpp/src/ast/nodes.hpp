#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>

// ── AST node hierarchy ────────────────────────────────────────────────────────
//
//  Expr  — evaluates to a value
//  Stmt  — performs an action (may produce a value as the "last expression")
//
//  Ownership: all nodes are held by shared_ptr so that Callable values inside
//  the interpreter can safely reference their body stmts after parsing is done.

namespace lang::ast {

struct Expr { virtual ~Expr() = default; };
struct Stmt { virtual ~Stmt() = default; };

using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

// ── parameter (used in CallableExpr and hash () member) ───────────────────────

struct Param {
    std::string name;
    std::optional<std::string> type_ann;
};

// ── expressions ───────────────────────────────────────────────────────────────

struct IntLiteral : Expr {
    int64_t value;
    explicit IntLiteral(int64_t v) : value(v) {}
};

struct FloatLiteral : Expr {
    double value;
    explicit FloatLiteral(double v) : value(v) {}
};

struct StringLiteral : Expr {
    std::string value;
    explicit StringLiteral(std::string v) : value(std::move(v)) {}
};

struct Identifier : Expr {
    std::string name;
    explicit Identifier(std::string n) : name(std::move(n)) {}
};

struct BinaryExpr : Expr {
    ExprPtr     left;
    std::string op;
    ExprPtr     right;
    BinaryExpr(ExprPtr l, std::string o, ExprPtr r)
        : left(std::move(l)), op(std::move(o)), right(std::move(r)) {}
};

struct UnaryExpr : Expr {
    std::string op;
    ExprPtr     operand;
    UnaryExpr(std::string o, ExprPtr expr)
        : op(std::move(o)), operand(std::move(expr)) {}
};

struct MemberExpr : Expr {
    ExprPtr     object;
    std::string member;
    MemberExpr(ExprPtr obj, std::string mem)
        : object(std::move(obj)), member(std::move(mem)) {}
};

struct CallExpr : Expr {
    ExprPtr              callee;
    std::vector<ExprPtr> args;
    CallExpr(ExprPtr c, std::vector<ExprPtr> a)
        : callee(std::move(c)), args(std::move(a)) {}
};

// ? / if  condition { then… } { else… }
struct CondExpr : Expr {
    ExprPtr              condition;
    std::vector<StmtPtr> then_block;
    std::vector<StmtPtr> else_block;
    CondExpr(ExprPtr cond, std::vector<StmtPtr> t, std::vector<StmtPtr> e)
        : condition(std::move(cond))
        , then_block(std::move(t))
        , else_block(std::move(e)) {}
};

// { name [: type] = expr  … }
struct HashMember {
    std::string                name;      // "()" for the constructor slot
    std::optional<std::string> type_ann;
    ExprPtr                    value;
};

struct HashLiteral : Expr {
    std::vector<HashMember> members;
    explicit HashLiteral(std::vector<HashMember> m) : members(std::move(m)) {}
};

// (params) { body… }
struct CallableExpr : Expr {
    std::vector<Param>   params;
    std::vector<StmtPtr> body;
    CallableExpr(std::vector<Param> p, std::vector<StmtPtr> b)
        : params(std::move(p)), body(std::move(b)) {}
};

// obj.(key_expr)  — dynamic member access by string key
struct DynMemberExpr : Expr {
    ExprPtr object;
    ExprPtr key;
    DynMemberExpr(ExprPtr obj, ExprPtr k)
        : object(std::move(obj)), key(std::move(k)) {}
};

// obj >> (k, v) { body… }  — iterate all members of a hash
struct IterExpr : Expr {
    ExprPtr object;    // the hash
    ExprPtr callable;  // (k, v) { body }
    IterExpr(ExprPtr obj, ExprPtr fn)
        : object(std::move(obj)), callable(std::move(fn)) {}
};

// ── statements ────────────────────────────────────────────────────────────────

// @ "path"  — import all definitions from a file
struct ImportStmt : Stmt {
    std::string path;
    explicit ImportStmt(std::string p) : path(std::move(p)) {}
};

// name [: type] (= | <-) expr
struct DeclStmt : Stmt {
    std::string                name;
    std::optional<std::string> type_ann;
    bool                       is_mutable;
    ExprPtr                    value;
    DeclStmt(std::string n, std::optional<std::string> t, bool mut, ExprPtr v)
        : name(std::move(n)), type_ann(std::move(t)), is_mutable(mut), value(std::move(v)) {}
};

// lvalue <- expr  (lvalue is Identifier or MemberExpr)
struct AssignStmt : Stmt {
    ExprPtr target;
    ExprPtr value;
    AssignStmt(ExprPtr tgt, ExprPtr val)
        : target(std::move(tgt)), value(std::move(val)) {}
};

struct ExprStmt : Stmt {
    ExprPtr expression;
    explicit ExprStmt(ExprPtr e) : expression(std::move(e)) {}
};

// ── program ───────────────────────────────────────────────────────────────────

struct Program {
    std::vector<StmtPtr> statements;
};

} // namespace lang::ast
