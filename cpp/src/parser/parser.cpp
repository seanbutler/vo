#include "parser/parser.hpp"
#include <sstream>

namespace lang {

using TT = TokenType;
using namespace ast;

// ── constructor ───────────────────────────────────────────────────────────────

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

// ── token helpers ─────────────────────────────────────────────────────────────

const Token& Parser::peek(int offset) const {
    size_t i = pos_ + offset;
    return i < tokens_.size() ? tokens_[i] : tokens_.back(); // back() == Eof
}

TokenType Parser::peek_type(int offset) const { return peek(offset).type; }

const Token& Parser::advance() {
    const Token& t = tokens_[pos_];
    if (t.type != TT::Eof) ++pos_;
    return t;
}

const Token& Parser::expect(TokenType t) {
    if (peek_type() != t) {
        std::ostringstream oss;
        oss << "L" << peek().line << ":" << peek().col
            << " — expected token type " << static_cast<int>(t)
            << " but got '" << peek().lexeme << "'";
        throw ParseError(oss.str());
    }
    return advance();
}

bool Parser::check(TokenType t, int offset) const { return peek_type(offset) == t; }

bool Parser::match(TokenType t) {
    if (peek_type() == t) { advance(); return true; }
    return false;
}

void Parser::error(const std::string& msg) const {
    std::ostringstream oss;
    oss << "L" << peek().line << ":" << peek().col << " — " << msg
        << " (got '" << peek().lexeme << "')";
    throw ParseError(oss.str());
}

// Scan to the matching ')' and check whether '{' immediately follows.
bool Parser::looks_like_callable() const {
    int depth = 0;
    for (size_t i = pos_; i < tokens_.size(); ++i) {
        if (tokens_[i].type == TT::LParen)  ++depth;
        if (tokens_[i].type == TT::RParen) {
            if (--depth == 0)
                return i + 1 < tokens_.size() &&
                       tokens_[i + 1].type == TT::LBrace;
        }
        if (tokens_[i].type == TT::Eof) break;
    }
    return false;
}

// ── top level ─────────────────────────────────────────────────────────────────

Program Parser::parse() {
    Program prog;
    while (!check(TT::Eof))
        prog.statements.push_back(parse_stmt());
    return prog;
}

// ── statements ────────────────────────────────────────────────────────────────

StmtPtr Parser::parse_stmt() {
    // Import:  '@' string
    if (check(TT::At)) {
        advance();
        std::string path = expect(TT::String).string_value();
        return std::make_shared<ImportStmt>(std::move(path));
    }

    // Typed declaration:  name ':' type ('=' | '<-') expr
    if (check(TT::Identifier) && check(TT::Colon, 1)) {
        std::string name    = advance().lexeme;
        advance();                                         // ':'
        std::string type_ann = expect(TT::Identifier).lexeme;
        bool is_mut = match(TT::Arrow);
        if (!is_mut) expect(TT::Assign);
        auto val = parse_expr();
        return std::make_shared<DeclStmt>(name,
                   std::optional<std::string>{type_ann}, is_mut, std::move(val));
    }

    // Untyped immutable declaration:  name '=' expr
    if (check(TT::Identifier) && check(TT::Assign, 1)) {
        std::string name = advance().lexeme;
        advance(); // '='
        auto val = parse_expr();
        return std::make_shared<DeclStmt>(name, std::nullopt, false, std::move(val));
    }

    // Everything else: parse as expression, then check for '<-' (assignment)
    auto expr = parse_expr();
    if (match(TT::Arrow)) {
        auto val = parse_expr();
        return std::make_shared<AssignStmt>(std::move(expr), std::move(val));
    }
    return std::make_shared<ExprStmt>(std::move(expr));
}

std::vector<StmtPtr> Parser::parse_block_body() {
    std::vector<StmtPtr> stmts;
    while (!check(TT::RBrace) && !check(TT::Eof))
        stmts.push_back(parse_stmt());
    return stmts;
}

// ── expressions (precedence ladder) ──────────────────────────────────────────

ExprPtr Parser::parse_expr()           { return parse_iter(); }

ExprPtr Parser::parse_iter() {
    auto left = parse_comparison();
    if (check(TT::GtGt)) {
        advance(); // consume '>>'
        auto fn = parse_callable();
        return std::make_shared<IterExpr>(std::move(left), std::move(fn));
    }
    return left;
}

ExprPtr Parser::parse_comparison() {
    auto left = parse_additive();
    while (check(TT::Eq)  || check(TT::Neq) ||
           check(TT::Lt)  || check(TT::Lte) ||
           check(TT::Gt)  || check(TT::Gte)) {
        std::string op = advance().lexeme;
        left = std::make_shared<BinaryExpr>(std::move(left), op, parse_additive());
    }
    return left;
}

ExprPtr Parser::parse_additive() {
    auto left = parse_multiplicative();
    while (check(TT::Plus) || check(TT::Minus)) {
        std::string op = advance().lexeme;
        left = std::make_shared<BinaryExpr>(std::move(left), op, parse_multiplicative());
    }
    return left;
}

ExprPtr Parser::parse_multiplicative() {
    auto left = parse_unary();
    while (check(TT::Star) || check(TT::Slash) || check(TT::Percent)) {
        std::string op = advance().lexeme;
        left = std::make_shared<BinaryExpr>(std::move(left), op, parse_unary());
    }
    return left;
}

ExprPtr Parser::parse_unary() {
    if (check(TT::Minus)) {
        advance();
        return std::make_shared<UnaryExpr>("-", parse_postfix());
    }
    if (check(TT::DollarDollar)) {
        advance();
        return std::make_shared<UnaryExpr>("$$", parse_postfix());
    }
    return parse_postfix();
}

ExprPtr Parser::parse_postfix() {
    auto expr = parse_primary();
    while (true) {
        if (match(TT::Dot)) {
            // Dynamic member access: .(expr)
            if (check(TT::LParen)) {
                advance(); // consume '('
                auto key = parse_expr();
                expect(TT::RParen);
                expr = std::make_shared<DynMemberExpr>(std::move(expr), std::move(key));
            } else {
                std::string mem = expect(TT::Identifier).lexeme;
                expr = std::make_shared<MemberExpr>(std::move(expr), mem);
            }
        } else if (check(TT::LParen)) {
            // '() =' is a hash constructor member — stop here, don't eat it as a call
            if (check(TT::RParen, 1) && check(TT::Assign, 2)) break;
            advance(); // '('
            std::vector<ExprPtr> args;
            if (!check(TT::RParen)) {
                args.push_back(parse_expr());
                while (match(TT::Comma))
                    args.push_back(parse_expr());
            }
            expect(TT::RParen);
            expr = std::make_shared<CallExpr>(std::move(expr), std::move(args));
        } else {
            break;
        }
    }
    return expr;
}

ExprPtr Parser::parse_primary() {
    // Integer literal
    if (check(TT::Integer)) {
        auto v = advance().int_value();
        return std::make_shared<IntLiteral>(v);
    }

    // String literal
    if (check(TT::String)) {
        auto v = advance().string_value();
        return std::make_shared<StringLiteral>(v);
    }

    // Identifier
    if (check(TT::Identifier)) {
        return std::make_shared<Identifier>(advance().lexeme);
    }

    // Conditional:  '?' / 'if'
    if (check(TT::Question) || check(TT::If))
        return parse_cond();

    // Callable:  '(' params ')' '{' body '}'
    if (check(TT::LParen) && looks_like_callable())
        return parse_callable();

    // Grouped expression:  '(' expr ')'
    if (check(TT::LParen)) {
        advance();
        auto e = parse_expr();
        expect(TT::RParen);
        return e;
    }

    // Hash literal:  '{' members '}'
    if (check(TT::LBrace))
        return parse_hash_literal();

    error("Expected expression");
}

// ── sub-parsers ───────────────────────────────────────────────────────────────

ExprPtr Parser::parse_cond() {
    advance(); // consume '?' or 'if'
    auto cond = parse_expr();

    expect(TT::LBrace);
    auto then_block = parse_block_body();
    expect(TT::RBrace);

    std::vector<StmtPtr> else_block;
    if (check(TT::LBrace)) {
        advance();
        else_block = parse_block_body();
        expect(TT::RBrace);
    }

    return std::make_shared<CondExpr>(
        std::move(cond), std::move(then_block), std::move(else_block));
}

ExprPtr Parser::parse_hash_literal() {
    expect(TT::LBrace);
    std::vector<HashMember> members;

    while (!check(TT::RBrace) && !check(TT::Eof)) {
        // Constructor slot:  '(' ')' '=' expr
        if (check(TT::LParen)) {
            expect(TT::LParen);
            expect(TT::RParen);
            expect(TT::Assign);
            auto val = parse_expr();
            members.push_back({ "()", std::nullopt, std::move(val) });
            continue;
        }

        // Named member:  name [':' type] '=' expr
        std::string name = expect(TT::Identifier).lexeme;
        std::optional<std::string> type_ann;
        if (match(TT::Colon))
            type_ann = expect(TT::Identifier).lexeme;
        expect(TT::Assign);
        auto val = parse_expr();
        members.push_back({ name, std::move(type_ann), std::move(val) });
    }

    expect(TT::RBrace);
    return std::make_shared<HashLiteral>(std::move(members));
}

ExprPtr Parser::parse_callable() {
    auto params = parse_param_list();

    expect(TT::LBrace);
    auto body = parse_block_body();
    expect(TT::RBrace);

    return std::make_shared<CallableExpr>(std::move(params), std::move(body));
}

std::vector<Param> Parser::parse_param_list() {
    expect(TT::LParen);
    std::vector<Param> params;
    if (!check(TT::RParen)) {
        params.push_back(parse_param());
        while (match(TT::Comma))
            params.push_back(parse_param());
    }
    expect(TT::RParen);
    return params;
}

Param Parser::parse_param() {
    Param p;
    p.name = expect(TT::Identifier).lexeme;
    if (match(TT::Colon))
        p.type_ann = expect(TT::Identifier).lexeme;
    return p;
}

} // namespace lang
