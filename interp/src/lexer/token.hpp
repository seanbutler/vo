#pragma once
#include <string>
#include <variant>
#include <cstdint>

namespace lang {

enum class TokenType {
    // Literals
    Integer, Float, String, Identifier,

    // Foreign binding
    DollarDollar,   // $$

    // Binding operators
    Assign,   // =
    ColonEq,  // :=

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

    // Conditional
    Question,  // ?

    // Loop
    Tilde,     // ~
    Backslash, // \ (loop break/escape)

    // Logical NOT
    Bang,      // !

    // Callable sigil
    At,        // @  (callable literal marker)

    // Module import
    Hash,      // #

    Eof
};

struct Token {
    TokenType                                    type;
    std::string                                  lexeme;
    std::variant<std::monostate, int64_t, double, std::string> literal;
    int line, col;

    // Plain token
    Token(TokenType t, std::string lex, int ln, int cl)
        : type(t), lexeme(std::move(lex)), line(ln), col(cl) {}

    // Integer literal
    Token(TokenType t, std::string lex, int64_t v, int ln, int cl)
        : type(t), lexeme(std::move(lex)), literal(v), line(ln), col(cl) {}

    // Float literal
    Token(TokenType t, std::string lex, double v, int ln, int cl)
        : type(t), lexeme(std::move(lex)), literal(v), line(ln), col(cl) {}

    // String literal
    Token(TokenType t, std::string lex, std::string v, int ln, int cl)
        : type(t), lexeme(std::move(lex)), literal(std::move(v)), line(ln), col(cl) {}

    int64_t     int_value()    const { return std::get<int64_t>(literal); }
    double      float_value()  const { return std::get<double>(literal); }
    std::string string_value() const { return std::get<std::string>(literal); }
};

} // namespace lang
