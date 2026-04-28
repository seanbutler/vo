#pragma once
#include "interpreter/value.hpp"
#include "interpreter/environment.hpp"
#include "ast/nodes.hpp"
#include <memory>
#include <stdexcept>

namespace lang {

class RuntimeError : public std::runtime_error {
public:
    explicit RuntimeError(const std::string& msg) : std::runtime_error(msg) {}
};

// Used for non-local return from function bodies.
struct ReturnSignal {
    ValuePtr value;
};

class Interpreter {
public:
    explicit Interpreter(bool verbose = true);

    void run(const ast::Program& program);

    // Exposed so that built-ins and tests can inspect globals.
    std::shared_ptr<Environment> globals() const { return globals_; }

private:
    std::shared_ptr<Environment> globals_;
    bool                         verbose_;

    // --- statement execution --------------------------------------------------
    ValuePtr exec(const ast::Stmt& stmt,  std::shared_ptr<Environment> env);
    ValuePtr exec_block(const std::vector<ast::StmtPtr>& stmts,
                        std::shared_ptr<Environment> env);

    ValuePtr exec_decl  (const ast::DeclStmt&   s, std::shared_ptr<Environment> env);
    ValuePtr exec_assign(const ast::AssignStmt& s, std::shared_ptr<Environment> env);
    ValuePtr exec_expr_stmt(const ast::ExprStmt& s, std::shared_ptr<Environment> env);
    ValuePtr exec_import(const ast::ImportStmt& s, std::shared_ptr<Environment> env);

    // --- expression evaluation --------------------------------------------------
    ValuePtr eval(const ast::Expr& expr, std::shared_ptr<Environment> env);

    ValuePtr eval_int      (const ast::IntLiteral&    e, std::shared_ptr<Environment> env);
    ValuePtr eval_float    (const ast::FloatLiteral&  e, std::shared_ptr<Environment> env);
    ValuePtr eval_string   (const ast::StringLiteral& e, std::shared_ptr<Environment> env);
    ValuePtr eval_ident    (const ast::Identifier&    e, std::shared_ptr<Environment> env);
    ValuePtr eval_binary   (const ast::BinaryExpr&    e, std::shared_ptr<Environment> env);
    ValuePtr eval_unary    (const ast::UnaryExpr&     e, std::shared_ptr<Environment> env);
    ValuePtr eval_member   (const ast::MemberExpr&    e, std::shared_ptr<Environment> env);
    ValuePtr eval_call     (const ast::CallExpr&      e, std::shared_ptr<Environment> env);
    ValuePtr eval_cond     (const ast::CondExpr&      e, std::shared_ptr<Environment> env);
    ValuePtr eval_hash_lit (const ast::HashLiteral&   e, std::shared_ptr<Environment> env);
    ValuePtr eval_callable (const ast::CallableExpr&  e, std::shared_ptr<Environment> env);
    ValuePtr eval_dyn_member(const ast::DynMemberExpr& e, std::shared_ptr<Environment> env);
    ValuePtr eval_iter     (const ast::IterExpr&       e, std::shared_ptr<Environment> env);

    // --- call dispatch --------------------------------------------------
    ValuePtr call_value(ValuePtr callee,
                        const std::vector<ValuePtr>& args,
                        ValuePtr receiver);

    ValuePtr call_callable(const Callable& fn,
                           const std::vector<ValuePtr>& args,
                           ValuePtr self_val);

    ValuePtr bind_foreign_function(ValuePtr spec);

    // --- built-ins -------------------------------------------------------------
    void register_builtins();
};

} // namespace lang
