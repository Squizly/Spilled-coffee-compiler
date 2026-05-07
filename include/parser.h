#pragma once

#include "lexer.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

enum class ElementType {
    Address,
    Constant,
    Operation,
    Label
};

enum class ParserValueType {
    Int,
    Float
};

enum class VariableForm {
    Scalar,
    Array
};

enum RpnOperationCode {
    RPN_OP_ADD = T_PLUS,
    RPN_OP_SUB = T_MINUS,
    RPN_OP_MUL = T_MUL,
    RPN_OP_DIV = T_DIV,
    RPN_OP_ASSIGN = T_ASSIGN,
    RPN_OP_EQ = T_EQ,
    RPN_OP_NE = T_NE,
    RPN_OP_LT = T_LT,
    RPN_OP_GT = T_GT,
    RPN_OP_LE = T_LE,                                                                  RPN_OP_GE = T_GE,
    RPN_OP_SQRT = T_SQRT,
    RPN_OP_EXP = T_EXP,
    RPN_OP_LOG = T_LOG,

    RPN_OP_UNARY_MINUS = 1000,
    RPN_OP_INDEX = 1001,
    RPN_OP_READ = 1002,
    RPN_OP_WRITE = 1003,
    RPN_OP_JUMP = 1004,
    RPN_OP_JUMP_FALSE = 1005
};

struct RpnElement {
    ElementType type;
    int value;

    RpnElement(ElementType elementType, int elementValue);
};

struct VariableInfo {
    std::string name;
    ParserValueType type;
    VariableForm form;
    int size;
};

struct ConstantInfo {
    std::string value;
    ParserValueType type;
};

class ParserError : public std::runtime_error {
public:
    ParserError(int line, int col, const std::string& message);

    int line() const;
    int col() const;

private:
    int errorLine;
    int errorCol;
};

class Parser {
public:
    explicit Parser(const std::string& source);
    explicit Parser(Lexer& lexer);

    bool parse();

    const std::vector<RpnElement>& getRpn() const;
    const std::vector<VariableInfo>& getVariables() const;
    const std::vector<ConstantInfo>& getConstants() const;

    std::string rpnToString() const;
    std::string rpnElementToString(const RpnElement& element) const;
    std::string operationToString(int operationCode) const;

private:
    std::unique_ptr<Lexer> ownedLexer;
    Lexer* lexer;

    Token currentToken;
    Token lastToken;
    bool hasCurrentToken;
    bool hasLastToken;

    std::vector<RpnElement> rpnString;
    std::vector<VariableInfo> variables;
    std::vector<ConstantInfo> constants;
    std::vector<int> labelStack;

    ParserValueType currentDeclarationType;
    int lastDeclaredIndex;
    TokenType pendingComparison;
    bool hasPendingComparison;

    void advance();
    void executeAction(int actionCode);

    void emitAddress(const std::string& name, const Token& token);
    void emitLastDeclaredAddress();
    void emitConstant(const Token& token);
    void emitOperation(int operationCode);
    int emitLabel(int value);
    void patchLabel(int rpnIndex, int target);

    void declareScalar(const Token& token);
    void markLastDeclarationAsArray(const Token& token);
    int findVariable(const std::string& name) const;
    int findOrAddConstant(const Token& token);

    void programFalseJump();
    void programElseJump();
    void programFinishIf();
    void programLoopStart();
    void programFinishLoop();

    void syntaxError(const std::string& expected) const;
    void semanticError(const Token& token, const std::string& message) const;
};
