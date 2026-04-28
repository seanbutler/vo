#pragma once
#include "ast/nodes.hpp"
#include "lexer/token.hpp"
#include <vector>
#include <stdexcept>

namespace lang {

class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& msg) : std::runtime_error(msg) {}
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    ast::Program parse();

private:
    std::vector<Token> tokens_;
    size_t             pos_ = 0;

    // ── token helpers ─────────────────────────────────────────────────────────
    const Token& peek(int offset = 0) const;
    TokenType    peek_type(int offset = 0) const;
    const Token& advance();
    const Token& expect(TokenType t);
    bool         check(TokenType t, int offset = 0) const;
    bool         match(TokenType t);
    [[noreturn]] void error(const std::string& msg) const;

    // Scan ahead to decide if '(' starts a callable or a grouped expression.
    // Returns true if the matching ')' is immediately followed by '{'.
    bool looks_like_callable() const;

    // ── statement parsers ─────────────────────────────────────────────────────
    ast::StmtPtr             parse_stmt();
    std::vector<ast::StmtPtr> parse_block_body();   // stmts until '}'

    // ── expression parsers (precedence ladder) ────────────────────────────────
    ast::ExprPtr parse_expr();
    ast::ExprPtr parse_comparison();
    ast::ExprPtr parse_additive();
    ast::ExprPtr parse_multiplicative();
    ast::ExprPtr parse_unary();
    ast::ExprPtr parse_postfix();
    ast::ExprPtr parse_primary();

    // ── sub-parsers ───────────────────────────────────────────────────────────
    ast::ExprPtr             parse_cond();
    ast::ExprPtr             parse_hash_literal();
    ast::ExprPtr             parse_callable();
    std::vector<ast::Param>  parse_param_list();
    ast::Param               parse_param();
};

} // namespace lang
