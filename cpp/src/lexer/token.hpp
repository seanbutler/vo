#pragma once
#include <string>
#include <variant>
#include <cstdint>

namespace lang {

enum class TokenType {
    // Literals
    Integer, String, Identifier,

    // Foreign binding
    DollarDollar,   // $$

    // Binding operators
    Assign,   // =
    Arrow,    // <-

    // Arithmetic
    Plus, Minus, Star, Slash, Percent,

    // Comparison
    Eq, Neq, Lt, Lte, Gt, Gte,

    // Iteration
    GtGt,   // >>

    // Punctuation
    Dot, Comma, Colon,
    LParen, RParen,
    LBrace, RBrace,

    // Conditional (aliased pair)
    Question,  // ?
    If,        // if

    // Module (aliased pair)
    From,      // from
    At,        // @

    Eof
};

struct Token {
    TokenType                                    type;
    std::string                                  lexeme;
    std::variant<std::monostate, int64_t, std::string> literal;
    int line, col;

    // Plain token
    Token(TokenType t, std::string lex, int ln, int cl)
        : type(t), lexeme(std::move(lex)), line(ln), col(cl) {}

    // Integer literal
    Token(TokenType t, std::string lex, int64_t v, int ln, int cl)
        : type(t), lexeme(std::move(lex)), literal(v), line(ln), col(cl) {}

    // String literal
    Token(TokenType t, std::string lex, std::string v, int ln, int cl)
        : type(t), lexeme(std::move(lex)), literal(std::move(v)), line(ln), col(cl) {}

    int64_t     int_value()    const { return std::get<int64_t>(literal); }
    std::string string_value() const { return std::get<std::string>(literal); }
};

} // namespace lang
