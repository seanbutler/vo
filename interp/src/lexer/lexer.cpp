#include "lexer.hpp"
#include <unordered_map>
#include <sstream>
#include <cctype>

namespace lang {

static const std::unordered_map<std::string, TokenType> KEYWORDS = {};

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

bool Lexer::at_end(int offset) const {
    return static_cast<int>(pos_) + offset >= static_cast<int>(source_.size());
}

char Lexer::peek(int offset) const {
    size_t i = pos_ + offset;
    return i < source_.size() ? source_[i] : '\0';
}

char Lexer::advance() {
    char c = source_[pos_++];
    if (c == '\n') { ++line_; col_ = 1; } else ++col_;
    return c;
}

void Lexer::error(const std::string& msg) const {
    std::ostringstream oss;
    oss << "Lex error L" << line_ << ":" << col_ << " — " << msg;
    throw LexError(oss.str());
}

void Lexer::skip_whitespace_and_comments() {
    while (!at_end()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peek(1) == '/') {
            while (!at_end() && peek() != '\n') advance();
        } else {
            break;
        }
    }
}

Token Lexer::read_string() {
    int ln = line_, cl = col_;
    advance(); // opening "
    std::string buf;
    while (!at_end()) {
        char c = peek();
        if (c == '"')  { advance(); return { TokenType::String, '"' + buf + '"', buf, ln, cl }; }
        if (c == '\n') error("Unterminated string literal");
        if (c == '\\') {
            advance();
            char e = advance();
            switch (e) {
                case 'n':  buf += '\n'; break;
                case 't':  buf += '\t'; break;
                case '"':  buf += '"';  break;
                case '\\': buf += '\\'; break;
                default:   buf += e;
            }
        } else {
            buf += advance();
        }
    }
    error("Unterminated string literal");
}

Token Lexer::read_number() {
    int ln = line_, cl = col_;
    std::string buf;
    while (!at_end() && std::isdigit(peek())) buf += advance();

    bool is_float = false;

    if (!at_end() && peek() == '.' && std::isdigit(peek(1))) {
        is_float = true;
        buf += advance(); // '.'
        while (!at_end() && std::isdigit(peek())) buf += advance();
    }

    if (!at_end() && (peek() == 'e' || peek() == 'E')) {
        int expo_pos = static_cast<int>(buf.size());
        buf += advance(); // e/E
        if (!at_end() && (peek() == '+' || peek() == '-'))
            buf += advance();
        if (at_end() || !std::isdigit(peek()))
            error("Malformed exponent in numeric literal");
        is_float = true;
        while (!at_end() && std::isdigit(peek())) buf += advance();
        (void)expo_pos;
    }

    if (is_float)
        return { TokenType::Float, buf, std::stod(buf), ln, cl };
    return { TokenType::Integer, buf, static_cast<int64_t>(std::stoll(buf)), ln, cl };
}

Token Lexer::read_word() {
    int ln = line_, cl = col_;
    std::string buf;
    while (!at_end() && (std::isalnum(peek()) || peek() == '_')) buf += advance();
    auto it = KEYWORDS.find(buf);
    TokenType tt = (it != KEYWORDS.end()) ? it->second : TokenType::Identifier;
    return { tt, buf, ln, cl };
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    int ln = 1, cl = 1;

    while (true) {
        skip_whitespace_and_comments();
        if (at_end()) break;

        ln = line_; cl = col_;
        char c = peek();

        if (c == '"')              { tokens.push_back(read_string());  continue; }
        if (std::isdigit(c))       { tokens.push_back(read_number()); continue; }
        if (std::isalpha(c) || c == '_') { tokens.push_back(read_word()); continue; }

        advance(); // consume single-char token

        switch (c) {
        case '$':
            if (peek() == '$') { advance(); tokens.push_back({ TokenType::DollarDollar, "$$", ln, cl }); }
            else error("Unexpected character '$'");
            break;
        case '+': tokens.push_back({ TokenType::Plus,    "+", ln, cl }); break;
        case '-': tokens.push_back({ TokenType::Minus,   "-", ln, cl }); break;
        case '*': tokens.push_back({ TokenType::Star,    "*", ln, cl }); break;
        case '/': tokens.push_back({ TokenType::Slash,   "/", ln, cl }); break;
        case '%': tokens.push_back({ TokenType::Percent, "%", ln, cl }); break;
        case '.': tokens.push_back({ TokenType::Dot,     ".", ln, cl }); break;
        case ',': tokens.push_back({ TokenType::Comma,   ",", ln, cl }); break;
        case ':': tokens.push_back({ TokenType::Colon,   ":", ln, cl }); break;
        case '(': tokens.push_back({ TokenType::LParen,  "(", ln, cl }); break;
        case ')': tokens.push_back({ TokenType::RParen,  ")", ln, cl }); break;
        case '{': tokens.push_back({ TokenType::LBrace,  "{", ln, cl }); break;
        case '}': tokens.push_back({ TokenType::RBrace,  "}", ln, cl }); break;
        case '@': tokens.push_back({ TokenType::At,      "@", ln, cl }); break;
        case '?': tokens.push_back({ TokenType::Question,"?", ln, cl }); break;
        case '=':
            if (peek() == '=') { advance(); tokens.push_back({ TokenType::Eq,  "==", ln, cl }); }
            else                             tokens.push_back({ TokenType::Assign, "=", ln, cl });
            break;
        case '!':
            if (peek() == '=') { advance(); tokens.push_back({ TokenType::Neq, "!=", ln, cl }); }
            else error("Expected '=' after '!'");
            break;
        case '<':
            if (peek() == '-') { advance(); tokens.push_back({ TokenType::Arrow, "<-", ln, cl }); }
            else if (peek() == '=') { advance(); tokens.push_back({ TokenType::Lte,  "<=", ln, cl }); }
            else                                  tokens.push_back({ TokenType::Lt,   "<",  ln, cl });
            break;
        case '>':
            if (peek() == '=') { advance(); tokens.push_back({ TokenType::Gte,  ">=", ln, cl }); }
            else if (peek() == '>') { advance(); tokens.push_back({ TokenType::GtGt, ">>", ln, cl }); }
            else                             tokens.push_back({ TokenType::Gt,   ">",  ln, cl });
            break;
        default:
            error(std::string("Unexpected character '") + c + "'");
        }
    }

    tokens.push_back({ TokenType::Eof, "", ln, cl });
    return tokens;
}

} // namespace lang
