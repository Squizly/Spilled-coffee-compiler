#include "../include/interpreter.h"
#include "../include/lexer.h"
#include "../include/parser.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string readFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("cannot open file: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string tokenName(TokenType type) {
    switch (type) {
        case T_ERROR: return "ERROR";
        case T_EOF: return "EOF";
        case T_ID: return "ID";
        case T_NUM_INT: return "NUM_INT";
        case T_NUM_FLOAT: return "NUM_FLOAT";
        case T_IF: return "IF";
        case T_ELSE: return "ELSE";
        case T_WHILE: return "WHILE";
        case T_INPUT: return "INPUT";
        case T_PRINT: return "PRINT";
        case T_INT: return "INT";
        case T_FLOAT: return "FLOAT";
        case T_PLUS: return "PLUS";
        case T_MINUS: return "MINUS";
        case T_MUL: return "MUL";
        case T_DIV: return "DIV";
        case T_ASSIGN: return "ASSIGN";
        case T_EQ: return "EQ";
        case T_NE: return "NE";
        case T_LT: return "LT";
        case T_GT: return "GT";
        case T_LE: return "LE";
        case T_GE: return "GE";
        case T_LPAR: return "LPAR";
        case T_RPAR: return "RPAR";
        case T_LBRACKET: return "LBRACKET";
        case T_RBRACKET: return "RBRACKET";
        case T_LBRACE: return "LBRACE";
        case T_RBRACE: return "RBRACE";
        case T_COMMA: return "COMMA";
        case T_SEMICOLON: return "SEMICOLON";
        case T_SQRT: return "SQRT";
        case T_EXP: return "EXP";
        case T_LOG: return "LOG";
    }
    return "UNKNOWN";
}

bool printTokens(const std::string& sourceCode) {
    Lexer lexer(sourceCode);

    std::cout << "=== Lexer ===\n";
    std::cout << std::left
              << std::setw(8) << "Code"
              << std::setw(14) << "Token"
              << std::setw(18) << "Value"
              << std::setw(8) << "Line"
              << std::setw(8) << "Col" << '\n';
    std::cout << std::string(56, '-') << '\n';

    while (true) {
        Token token = lexer.getNextToken();
        std::cout << std::left
                  << std::setw(8) << static_cast<int>(token.type)
                  << std::setw(14) << tokenName(token.type)
                  << std::setw(18) << token.value
                  << std::setw(8) << token.line
                  << std::setw(8) << token.col << '\n';

        if (token.type == T_ERROR) {
            std::cerr << "Lexical error at line " << token.line
                      << ", column " << token.col
                      << ": " << token.value << '\n';
            return false;
        }

        if (token.type == T_EOF) {
            return true;
        }
    }
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <source-file>\n";
        return 1;
    }

    try {
        std::string sourceCode = readFile(argv[1]);

        if (!printTokens(sourceCode)) {
            return 1;
        }

        std::cout << "\n=== Parser / RPN ===\n";
        Parser parser(sourceCode);
        parser.parse();
        std::cout << parser.rpnToString() << "\n";

        std::cout << "\n=== Interpreter output ===\n";
        Interpreter interpreter(parser);
        interpreter.run(std::cin, std::cout);

        std::cout << "\n=== Variables after run ===\n";
        std::cout << interpreter.dumpVariables() << "\n";
    } catch (const ParserError& error) {
        std::cerr << error.what() << '\n';
        return 1;
    } catch (const InterpreterError& error) {
        std::cerr << error.what() << '\n';
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
