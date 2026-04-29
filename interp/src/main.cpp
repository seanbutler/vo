#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "interpreter/interpreter.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    std::ostringstream buf;
    buf << f.rdbuf();
    return buf.str();
}

static void run(const std::string& source, lang::Interpreter& interp, bool trace = false) {
    lang::Lexer lexer(source);
    auto tokens = lexer.tokenize();

    if (trace) {
        std::cout << "══ TOKENS ══════════════════════════════════════════\n";
        for (auto& t : tokens)
            std::cout << "  [" << t.line << ":" << t.col << "] "
                      << static_cast<int>(t.type) << "  " << t.lexeme << "\n";
    }

    lang::Parser parser(std::move(tokens));
    auto ast = parser.parse();
    interp.run(ast);
}

int main(int argc, char** argv) {
    bool trace = false;
    std::string file;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--trace") trace = true;
        else                  file  = arg;
    }

    try {
        if (file.empty()) {
            std::cerr << "Usage: vo <file.vo> [--trace]\n";
            return 1;
        }

        lang::Interpreter interp(trace);

        std::cout << "══ OUTPUT ══════════════════════════════════════════\n";
        run(read_file(file), interp, trace);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
