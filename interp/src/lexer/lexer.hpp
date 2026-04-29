#pragma once
#include "token.hpp"
#include <string>
#include <vector>
#include <stdexcept>

namespace lang {

class LexError : public std::runtime_error {
public:
    explicit LexError(const std::string& msg) : std::runtime_error(msg) {}
};

class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> tokenize();

private:
    std::string source_;
    size_t      pos_  = 0;
    int         line_ = 1;
    int         col_  = 1;

    bool   at_end(int offset = 0) const;
    char   peek(int offset = 0)   const;
    char   advance();
    void   skip_whitespace_and_comments();
    Token  read_string();
    Token  read_number();
    Token  read_word();
    [[noreturn]] void error(const std::string& msg) const;
};

} // namespace lang
