#include "../include/parser.h"

#include <cstdlib>
#include <initializer_list>
#include <map>
#include <sstream>
#include <utility>

namespace {

enum class NonTerminal {
    Program,
    Statement,
    Declaration,
    TypeName,
    ArrayMarker,
    DeclarationInit,
    DeclarationRest,
    Assignment,
    Index,                                                                             IfStatement,
    ElsePart,
    WhileStatement,
    InputStatement,
    OutputStatement,
    Condition,
    Comparison,
    Expression,
    ExpressionTail,
    Term,
    TermTail,
    Factor
};

enum class EntryKind {
    Terminal,
    NonTerminal,
    Action
};

enum ActionCode {
    ACT_NONE = 0,
    ACT_SET_INT,
    ACT_SET_FLOAT,
    ACT_DECLARE_SCALAR,
    ACT_MARK_ARRAY,
    ACT_EMIT_ADDRESS,
    ACT_EMIT_LAST_DECLARED_ADDRESS,
    ACT_EMIT_CONSTANT,
    ACT_ADD,
    ACT_SUB,
    ACT_MUL,
    ACT_DIV,
    ACT_ASSIGN,
    ACT_UNARY_MINUS,
    ACT_INDEX,
    ACT_READ,
    ACT_WRITE,
    ACT_SQRT,
    ACT_EXP,
    ACT_LOG,
    ACT_STORE_COMPARISON,
    ACT_EMIT_COMPARISON,
    ACT_FALSE_JUMP,
    ACT_ELSE_JUMP,
    ACT_FINISH_IF,
    ACT_LOOP_START,
    ACT_FINISH_LOOP
};

struct StackEntry {
    EntryKind kind;
    TokenType terminal;
    NonTerminal nonTerminal;
    int action;

    static StackEntry terminalEntry(TokenType tokenType) {
        StackEntry entry;
        entry.kind = EntryKind::Terminal;
        entry.terminal = tokenType;
        entry.nonTerminal = NonTerminal::Program;
        entry.action = ACT_NONE;
        return entry;
    }

    static StackEntry nonTerminalEntry(NonTerminal symbol) {
        StackEntry entry;
        entry.kind = EntryKind::NonTerminal;
        entry.terminal = T_EOF;
        entry.nonTerminal = symbol;
        entry.action = ACT_NONE;
        return entry;
    }

    static StackEntry actionEntry(int actionCode) {
        StackEntry entry;
        entry.kind = EntryKind::Action;
        entry.terminal = T_EOF;
        entry.nonTerminal = NonTerminal::Program;
        entry.action = actionCode;
        return entry;
    }
};

struct Rule {
    std::vector<StackEntry> rhs;
    std::vector<int> actions;
    std::vector<int> onApplyActions;
};

typedef std::pair<NonTerminal, TokenType> TableKey;
typedef std::map<TableKey, Rule> ParseTable;

StackEntry t(TokenType tokenType) {
    return StackEntry::terminalEntry(tokenType);
}

StackEntry nt(NonTerminal symbol) {
    return StackEntry::nonTerminalEntry(symbol);
}

void addRule(ParseTable& table,
             NonTerminal nonTerminal,
             std::initializer_list<TokenType> lookaheads,
             std::initializer_list<StackEntry> rhs,
             std::initializer_list<int> actions = std::initializer_list<int>(),
             std::initializer_list<int> onApplyActions = std::initializer_list<int>()) {
    Rule rule;
    rule.rhs.assign(rhs.begin(), rhs.end());
    rule.onApplyActions.assign(onApplyActions.begin(), onApplyActions.end());

    if (actions.size() == 0) {
        rule.actions.assign(rule.rhs.size(), ACT_NONE);
    } else {
        rule.actions.assign(actions.begin(), actions.end());
    }

    for (TokenType lookahead : lookaheads) {
        table[TableKey(nonTerminal, lookahead)] = rule;
    }
}

ParseTable buildParseTable() {
    ParseTable table;

    const TokenType statementStarts[] = {
        T_INT, T_FLOAT, T_ID, T_IF, T_WHILE, T_INPUT, T_PRINT
    };
    const TokenType expressionStarts[] = {
        T_ID, T_NUM_INT, T_NUM_FLOAT, T_LPAR, T_MINUS, T_SQRT, T_EXP, T_LOG
    };
    const TokenType expressionFollows[] = {
        T_RPAR, T_RBRACKET, T_SEMICOLON, T_COMMA,
        T_EQ, T_NE, T_LT, T_GT, T_LE, T_GE
    };
    const TokenType termFollows[] = {
        T_PLUS, T_MINUS, T_RPAR, T_RBRACKET, T_SEMICOLON, T_COMMA,
        T_EQ, T_NE, T_LT, T_GT, T_LE, T_GE
    };
    const TokenType indexFollows[] = {
        T_ASSIGN, T_RPAR, T_RBRACKET, T_SEMICOLON, T_COMMA,
        T_PLUS, T_MINUS, T_MUL, T_DIV, T_EQ, T_NE, T_LT, T_GT, T_LE, T_GE
    };

    for (TokenType lookahead : statementStarts) {
        addRule(table,
                NonTerminal::Program,
                {lookahead},
                {nt(NonTerminal::Statement), nt(NonTerminal::Program)});
    }
    addRule(table, NonTerminal::Program, {T_RBRACE, T_EOF}, {});

    addRule(table, NonTerminal::Statement, {T_INT, T_FLOAT}, {nt(NonTerminal::Declaration)});
    addRule(table, NonTerminal::Statement, {T_ID}, {nt(NonTerminal::Assignment)});
    addRule(table, NonTerminal::Statement, {T_IF}, {nt(NonTerminal::IfStatement)});
    addRule(table, NonTerminal::Statement, {T_WHILE}, {nt(NonTerminal::WhileStatement)});
    addRule(table, NonTerminal::Statement, {T_INPUT}, {nt(NonTerminal::InputStatement)});
    addRule(table, NonTerminal::Statement, {T_PRINT}, {nt(NonTerminal::OutputStatement)});

    addRule(table,
            NonTerminal::Declaration,
            {T_INT, T_FLOAT},
            {nt(NonTerminal::TypeName),
             t(T_ID),
             nt(NonTerminal::ArrayMarker),
             nt(NonTerminal::DeclarationInit),
             nt(NonTerminal::DeclarationRest),
             t(T_SEMICOLON)},
            {ACT_NONE, ACT_DECLARE_SCALAR, ACT_NONE, ACT_NONE, ACT_NONE, ACT_NONE});

    addRule(table, NonTerminal::TypeName, {T_INT}, {t(T_INT)}, {ACT_SET_INT});
    addRule(table, NonTerminal::TypeName, {T_FLOAT}, {t(T_FLOAT)}, {ACT_SET_FLOAT});

    addRule(table,
            NonTerminal::ArrayMarker,
            {T_LBRACKET},
            {t(T_LBRACKET), t(T_NUM_INT), t(T_RBRACKET)},
            {ACT_NONE, ACT_MARK_ARRAY, ACT_NONE});
    addRule(table, NonTerminal::ArrayMarker, {T_ASSIGN, T_COMMA, T_SEMICOLON}, {});

    addRule(table,
            NonTerminal::DeclarationInit,
            {T_ASSIGN},
            {t(T_ASSIGN), nt(NonTerminal::Expression)},
            {ACT_EMIT_LAST_DECLARED_ADDRESS, ACT_ASSIGN});
    addRule(table, NonTerminal::DeclarationInit, {T_COMMA, T_SEMICOLON}, {});

    addRule(table,
            NonTerminal::DeclarationRest,
            {T_COMMA},
            {t(T_COMMA),
             t(T_ID),
             nt(NonTerminal::ArrayMarker),
             nt(NonTerminal::DeclarationInit),
             nt(NonTerminal::DeclarationRest)},
            {ACT_NONE, ACT_DECLARE_SCALAR, ACT_NONE, ACT_NONE, ACT_NONE});
    addRule(table, NonTerminal::DeclarationRest, {T_SEMICOLON}, {});

    addRule(table,
            NonTerminal::Assignment,
            {T_ID},
            {t(T_ID),
             nt(NonTerminal::Index),
             t(T_ASSIGN),
             nt(NonTerminal::Expression),
             t(T_SEMICOLON)},
            {ACT_EMIT_ADDRESS, ACT_NONE, ACT_NONE, ACT_ASSIGN, ACT_NONE});

    addRule(table,
            NonTerminal::Index,
            {T_LBRACKET},
            {t(T_LBRACKET), nt(NonTerminal::Expression), t(T_RBRACKET)},
            {ACT_NONE, ACT_NONE, ACT_INDEX});
    for (TokenType lookahead : indexFollows) {
        addRule(table, NonTerminal::Index, {lookahead}, {});
    }

    addRule(table,
            NonTerminal::IfStatement,
            {T_IF},
            {t(T_IF),
             t(T_LPAR),
             nt(NonTerminal::Condition),
             t(T_RPAR),
             t(T_LBRACE),
             nt(NonTerminal::Program),
             t(T_RBRACE),
             nt(NonTerminal::ElsePart)},
            {ACT_NONE, ACT_NONE, ACT_NONE, ACT_FALSE_JUMP,
             ACT_NONE, ACT_NONE, ACT_NONE, ACT_NONE});

    addRule(table,
            NonTerminal::ElsePart,
            {T_ELSE},
            {t(T_ELSE), t(T_LBRACE), nt(NonTerminal::Program), t(T_RBRACE)},
            {ACT_ELSE_JUMP, ACT_NONE, ACT_NONE, ACT_FINISH_IF});
    for (TokenType lookahead : statementStarts) {
        addRule(table, NonTerminal::ElsePart, {lookahead}, {}, {}, {ACT_FINISH_IF});
    }
    addRule(table, NonTerminal::ElsePart, {T_RBRACE, T_EOF}, {}, {}, {ACT_FINISH_IF});

    addRule(table,
            NonTerminal::WhileStatement,
            {T_WHILE},
            {t(T_WHILE),
             t(T_LPAR),
             nt(NonTerminal::Condition),
             t(T_RPAR),
             t(T_LBRACE),
             nt(NonTerminal::Program),
             t(T_RBRACE)},
            {ACT_LOOP_START, ACT_NONE, ACT_NONE, ACT_FALSE_JUMP,
             ACT_NONE, ACT_NONE, ACT_FINISH_LOOP});

    addRule(table,
            NonTerminal::InputStatement,
            {T_INPUT},
            {t(T_INPUT),
             t(T_LPAR),
             t(T_ID),
             nt(NonTerminal::Index),
             t(T_RPAR),
             t(T_SEMICOLON)},
            {ACT_NONE, ACT_NONE, ACT_EMIT_ADDRESS, ACT_NONE, ACT_READ, ACT_NONE});

    addRule(table,
            NonTerminal::OutputStatement,
            {T_PRINT},
            {t(T_PRINT),
             t(T_LPAR),
             nt(NonTerminal::Expression),
             t(T_RPAR),
             t(T_SEMICOLON)},
            {ACT_NONE, ACT_NONE, ACT_WRITE, ACT_NONE, ACT_NONE});

    for (TokenType lookahead : expressionStarts) {
        addRule(table,
                NonTerminal::Condition,
                {lookahead},
                {nt(NonTerminal::Expression),
                 nt(NonTerminal::Comparison),
                 nt(NonTerminal::Expression)},
                {ACT_NONE, ACT_NONE, ACT_EMIT_COMPARISON});
    }

    addRule(table, NonTerminal::Comparison, {T_EQ}, {t(T_EQ)}, {ACT_STORE_COMPARISON});
    addRule(table, NonTerminal::Comparison, {T_NE}, {t(T_NE)}, {ACT_STORE_COMPARISON});
    addRule(table, NonTerminal::Comparison, {T_LT}, {t(T_LT)}, {ACT_STORE_COMPARISON});
    addRule(table, NonTerminal::Comparison, {T_GT}, {t(T_GT)}, {ACT_STORE_COMPARISON});
    addRule(table, NonTerminal::Comparison, {T_LE}, {t(T_LE)}, {ACT_STORE_COMPARISON});
    addRule(table, NonTerminal::Comparison, {T_GE}, {t(T_GE)}, {ACT_STORE_COMPARISON});

    for (TokenType lookahead : expressionStarts) {
        addRule(table,
                NonTerminal::Expression,
                {lookahead},
                {nt(NonTerminal::Term), nt(NonTerminal::ExpressionTail)});
        addRule(table,
                NonTerminal::Term,
                {lookahead},
                {nt(NonTerminal::Factor), nt(NonTerminal::TermTail)});
    }

    addRule(table,
            NonTerminal::ExpressionTail,
            {T_PLUS},
            {t(T_PLUS), nt(NonTerminal::Term), nt(NonTerminal::ExpressionTail)},
            {ACT_NONE, ACT_ADD, ACT_NONE});
    addRule(table,
            NonTerminal::ExpressionTail,
            {T_MINUS},
            {t(T_MINUS), nt(NonTerminal::Term), nt(NonTerminal::ExpressionTail)},
            {ACT_NONE, ACT_SUB, ACT_NONE});
    for (TokenType lookahead : expressionFollows) {
        addRule(table, NonTerminal::ExpressionTail, {lookahead}, {});
    }

    addRule(table,
            NonTerminal::TermTail,
            {T_MUL},
            {t(T_MUL), nt(NonTerminal::Factor), nt(NonTerminal::TermTail)},
            {ACT_NONE, ACT_MUL, ACT_NONE});
    addRule(table,
            NonTerminal::TermTail,
            {T_DIV},
            {t(T_DIV), nt(NonTerminal::Factor), nt(NonTerminal::TermTail)},
            {ACT_NONE, ACT_DIV, ACT_NONE});
    for (TokenType lookahead : termFollows) {
        addRule(table, NonTerminal::TermTail, {lookahead}, {});
    }

    addRule(table,
            NonTerminal::Factor,
            {T_LPAR},
            {t(T_LPAR), nt(NonTerminal::Expression), t(T_RPAR)});
    addRule(table,
            NonTerminal::Factor,
            {T_ID},
            {t(T_ID), nt(NonTerminal::Index)},
            {ACT_EMIT_ADDRESS, ACT_NONE});
    addRule(table, NonTerminal::Factor, {T_NUM_INT}, {t(T_NUM_INT)}, {ACT_EMIT_CONSTANT});
    addRule(table, NonTerminal::Factor, {T_NUM_FLOAT}, {t(T_NUM_FLOAT)}, {ACT_EMIT_CONSTANT});
    addRule(table,
            NonTerminal::Factor,
            {T_MINUS},
            {t(T_MINUS), nt(NonTerminal::Factor)},
            {ACT_NONE, ACT_UNARY_MINUS});
    addRule(table,
            NonTerminal::Factor,
            {T_SQRT},
            {t(T_SQRT), t(T_LPAR), nt(NonTerminal::Expression), t(T_RPAR)},
            {ACT_NONE, ACT_NONE, ACT_NONE, ACT_SQRT});
    addRule(table,
            NonTerminal::Factor,
            {T_EXP},
            {t(T_EXP), t(T_LPAR), nt(NonTerminal::Expression), t(T_RPAR)},
            {ACT_NONE, ACT_NONE, ACT_NONE, ACT_EXP});
    addRule(table,
            NonTerminal::Factor,
            {T_LOG},
            {t(T_LOG), t(T_LPAR), nt(NonTerminal::Expression), t(T_RPAR)},
            {ACT_NONE, ACT_NONE, ACT_NONE, ACT_LOG});

    return table;
}

const ParseTable& parseTable() {
    static ParseTable table = buildParseTable();
    return table;
}

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case T_ERROR: return "error";
        case T_EOF: return "EOF";
        case T_ID: return "id";
        case T_NUM_INT: return "num_int";
        case T_NUM_FLOAT: return "num_float";
        case T_IF: return "if";
        case T_ELSE: return "else";
        case T_WHILE: return "while";
        case T_INPUT: return "input";
        case T_PRINT: return "print";
        case T_INT: return "int";
        case T_FLOAT: return "float";
        case T_PLUS: return "+";
        case T_MINUS: return "-";
        case T_MUL: return "*";
        case T_DIV: return "/";
        case T_ASSIGN: return "=";
        case T_EQ: return "==";
        case T_NE: return "!=";
        case T_LT: return "<";
        case T_GT: return ">";
        case T_LE: return "<=";
        case T_GE: return ">=";
        case T_LPAR: return "(";
        case T_RPAR: return ")";
        case T_LBRACKET: return "[";
        case T_RBRACKET: return "]";
        case T_LBRACE: return "{";
        case T_RBRACE: return "}";
        case T_COMMA: return ",";
        case T_SEMICOLON: return ";";
        case T_SQRT: return "sqrt";
        case T_EXP: return "exp";
        case T_LOG: return "log";
    }
    return "unknown";
}

std::string makeParserErrorMessage(int line, int col, const std::string& message) {
    std::ostringstream out;
    out << "Parser error at line " << line << ", column " << col << ": " << message;
    return out.str();
}

} // namespace

RpnElement::RpnElement(ElementType elementType, int elementValue)
    : type(elementType), value(elementValue) {
}

ParserError::ParserError(int line, int col, const std::string& message)
    : std::runtime_error(makeParserErrorMessage(line, col, message)),
      errorLine(line),
      errorCol(col) {
}

int ParserError::line() const {
    return errorLine;
}

int ParserError::col() const {
    return errorCol;
}

Parser::Parser(const std::string& source)
    : ownedLexer(new Lexer(source)),
      lexer(ownedLexer.get()),
      hasCurrentToken(false),
      hasLastToken(false),
      currentDeclarationType(ParserValueType::Int),
      lastDeclaredIndex(-1),
      pendingComparison(T_EOF),
      hasPendingComparison(false) {
}

Parser::Parser(Lexer& lexerRef)
    : ownedLexer(),
      lexer(&lexerRef),
      hasCurrentToken(false),
      hasLastToken(false),
      currentDeclarationType(ParserValueType::Int),
      lastDeclaredIndex(-1),
      pendingComparison(T_EOF),
      hasPendingComparison(false) {
}

bool Parser::parse() {
    rpnString.clear();
    variables.clear();
    constants.clear();
    labelStack.clear();
    currentDeclarationType = ParserValueType::Int;
    lastDeclaredIndex = -1;
    pendingComparison = T_EOF;
    hasPendingComparison = false;
    hasLastToken = false;

    advance();

    std::vector<StackEntry> stack;
    stack.push_back(t(T_EOF));
    stack.push_back(nt(NonTerminal::Program));

    while (!stack.empty()) {
        StackEntry entry = stack.back();
        stack.pop_back();

        if (entry.kind == EntryKind::Action) {
            executeAction(entry.action);
            continue;
        }

        if (entry.kind == EntryKind::Terminal) {
            if (currentToken.type != entry.terminal) {
                syntaxError(tokenTypeToString(entry.terminal));
            }

            lastToken = currentToken;
            hasLastToken = true;

            if (entry.terminal != T_EOF) {
                advance();
            }
            continue;
        }

        const ParseTable& table = parseTable();
        ParseTable::const_iterator ruleIt =
            table.find(TableKey(entry.nonTerminal, currentToken.type));
        if (ruleIt == table.end()) {
            syntaxError("valid syntax construct");
        }

        const Rule& rule = ruleIt->second;
        for (int actionCode : rule.onApplyActions) {
            executeAction(actionCode);
        }

        for (int i = static_cast<int>(rule.rhs.size()) - 1; i >= 0; --i) {
            if (rule.actions[i] != ACT_NONE) {
                stack.push_back(StackEntry::actionEntry(rule.actions[i]));
            }
            stack.push_back(rule.rhs[i]);
        }
    }

    return true;
}

const std::vector<RpnElement>& Parser::getRpn() const {
    return rpnString;
}

const std::vector<VariableInfo>& Parser::getVariables() const {
    return variables;
}

const std::vector<ConstantInfo>& Parser::getConstants() const {
    return constants;
}

std::string Parser::rpnToString() const {
    std::ostringstream out;
    for (std::size_t i = 0; i < rpnString.size(); ++i) {
        if (i != 0) {
            out << ' ';
        }
        out << rpnElementToString(rpnString[i]);
    }
    return out.str();
}

std::string Parser::rpnElementToString(const RpnElement& element) const {
    std::ostringstream out;

    switch (element.type) {
        case ElementType::Address:
            out << 'a' << element.value;
            if (element.value >= 0 &&
                element.value < static_cast<int>(variables.size())) {
                out << '(' << variables[element.value].name << ')';
            }
            break;
        case ElementType::Constant:
            out << 'k' << element.value;
            if (element.value >= 0 &&
                element.value < static_cast<int>(constants.size())) {
                out << '(' << constants[element.value].value << ')';
            }
            break;
        case ElementType::Operation:
            out << operationToString(element.value);
            break;
        case ElementType::Label:
            out << 'm';
            if (element.value < 0) {
                out << '?';
            } else {
                out << element.value;
            }
            break;
    }

    return out.str();
}

std::string Parser::operationToString(int operationCode) const {
    switch (operationCode) {
        case RPN_OP_ADD: return "+";
        case RPN_OP_SUB: return "-";
        case RPN_OP_MUL: return "*";
        case RPN_OP_DIV: return "/";
        case RPN_OP_ASSIGN: return ":=";
        case RPN_OP_EQ: return "==";
        case RPN_OP_NE: return "!=";
        case RPN_OP_LT: return "<";
        case RPN_OP_GT: return ">";
        case RPN_OP_LE: return "<=";
        case RPN_OP_GE: return ">=";
        case RPN_OP_SQRT: return "sqrt";
        case RPN_OP_EXP: return "exp";
        case RPN_OP_LOG: return "log";
        case RPN_OP_UNARY_MINUS: return "@";
        case RPN_OP_INDEX: return "i";
        case RPN_OP_READ: return "r";
        case RPN_OP_WRITE: return "w";
        case RPN_OP_JUMP: return "j";
        case RPN_OP_JUMP_FALSE: return "jf";
    }
    return "?";
}

void Parser::advance() {
    currentToken = lexer->getNextToken();
    hasCurrentToken = true;

    if (currentToken.type == T_ERROR) {
        throw ParserError(currentToken.line,
                          currentToken.col,
                          "lexical error near '" + currentToken.value + "'");
    }
}

void Parser::executeAction(int actionCode) {
    switch (actionCode) {
        case ACT_NONE:
            break;
        case ACT_SET_INT:
            currentDeclarationType = ParserValueType::Int;
            break;
        case ACT_SET_FLOAT:
            currentDeclarationType = ParserValueType::Float;
            break;
        case ACT_DECLARE_SCALAR:
            declareScalar(lastToken);
            break;
        case ACT_MARK_ARRAY:
            markLastDeclarationAsArray(lastToken);
            break;
        case ACT_EMIT_ADDRESS:
            emitAddress(lastToken.value, lastToken);
            break;
        case ACT_EMIT_LAST_DECLARED_ADDRESS:
            emitLastDeclaredAddress();
            break;
        case ACT_EMIT_CONSTANT:
            emitConstant(lastToken);
            break;
        case ACT_ADD:
            emitOperation(RPN_OP_ADD);
            break;
        case ACT_SUB:
            emitOperation(RPN_OP_SUB);
            break;
        case ACT_MUL:
            emitOperation(RPN_OP_MUL);
            break;
        case ACT_DIV:
            emitOperation(RPN_OP_DIV);
            break;
        case ACT_ASSIGN:
            emitOperation(RPN_OP_ASSIGN);
            break;
        case ACT_UNARY_MINUS:
            emitOperation(RPN_OP_UNARY_MINUS);
            break;
        case ACT_INDEX:
            emitOperation(RPN_OP_INDEX);
            break;
        case ACT_READ:
            emitOperation(RPN_OP_READ);
            break;
        case ACT_WRITE:
            emitOperation(RPN_OP_WRITE);
            break;
        case ACT_SQRT:
            emitOperation(RPN_OP_SQRT);
            break;
        case ACT_EXP:
            emitOperation(RPN_OP_EXP);
            break;
        case ACT_LOG:
            emitOperation(RPN_OP_LOG);
            break;
        case ACT_STORE_COMPARISON:
            pendingComparison = lastToken.type;
            hasPendingComparison = true;
            break;
        case ACT_EMIT_COMPARISON:
            if (!hasPendingComparison) {
                semanticError(lastToken, "missing comparison operation");
            }
            emitOperation(static_cast<int>(pendingComparison));
            hasPendingComparison = false;
            break;
        case ACT_FALSE_JUMP:
            programFalseJump();
            break;
        case ACT_ELSE_JUMP:
            programElseJump();
            break;
        case ACT_FINISH_IF:
            programFinishIf();
            break;
        case ACT_LOOP_START:
            programLoopStart();
            break;
        case ACT_FINISH_LOOP:
            programFinishLoop();
            break;
        default:
            semanticError(lastToken, "unknown semantic action");
            break;
    }
}

void Parser::emitAddress(const std::string& name, const Token& token) {
    int index = findVariable(name);
    if (index < 0) {
        semanticError(token, "undeclared variable '" + name + "'");
    }
    rpnString.push_back(RpnElement(ElementType::Address, index));
}

void Parser::emitLastDeclaredAddress() {
    if (lastDeclaredIndex < 0 ||
        lastDeclaredIndex >= static_cast<int>(variables.size())) {
        semanticError(lastToken, "missing declaration target");
    }
    if (variables[lastDeclaredIndex].form != VariableForm::Scalar) {
        semanticError(lastToken, "array declaration cannot be initialized as scalar");
    }
    rpnString.push_back(RpnElement(ElementType::Address, lastDeclaredIndex));
}

void Parser::emitConstant(const Token& token) {
    int index = findOrAddConstant(token);
    rpnString.push_back(RpnElement(ElementType::Constant, index));
}

void Parser::emitOperation(int operationCode) {
    rpnString.push_back(RpnElement(ElementType::Operation, operationCode));
}

int Parser::emitLabel(int value) {
    int index = static_cast<int>(rpnString.size());
    rpnString.push_back(RpnElement(ElementType::Label, value));
    return index;
}

void Parser::patchLabel(int rpnIndex, int target) {
    if (rpnIndex < 0 || rpnIndex >= static_cast<int>(rpnString.size()) ||
        rpnString[rpnIndex].type != ElementType::Label) {
        semanticError(lastToken, "invalid jump label");
    }
    rpnString[rpnIndex].value = target;
}

void Parser::declareScalar(const Token& token) {
    if (findVariable(token.value) >= 0) {
        semanticError(token, "duplicate variable declaration '" + token.value + "'");
    }

    VariableInfo variable;
    variable.name = token.value;
    variable.type = currentDeclarationType;
    variable.form = VariableForm::Scalar;
    variable.size = 1;

    variables.push_back(variable);
    lastDeclaredIndex = static_cast<int>(variables.size()) - 1;
}

void Parser::markLastDeclarationAsArray(const Token& token) {
    if (lastDeclaredIndex < 0 ||
        lastDeclaredIndex >= static_cast<int>(variables.size())) {
        semanticError(token, "array declaration without variable");
    }

    int size = std::atoi(token.value.c_str());
    if (size <= 0) {
        semanticError(token, "array size must be positive");
    }

    variables[lastDeclaredIndex].form = VariableForm::Array;
    variables[lastDeclaredIndex].size = size;
}

int Parser::findVariable(const std::string& name) const {
    for (std::size_t i = 0; i < variables.size(); ++i) {
        if (variables[i].name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int Parser::findOrAddConstant(const Token& token) {
    ParserValueType type =
        (token.type == T_NUM_FLOAT) ? ParserValueType::Float : ParserValueType::Int;

    for (std::size_t i = 0; i < constants.size(); ++i) {
        if (constants[i].value == token.value && constants[i].type == type) {
            return static_cast<int>(i);
        }
    }

    ConstantInfo constant;
    constant.value = token.value;
    constant.type = type;

    constants.push_back(constant);
    return static_cast<int>(constants.size()) - 1;
}

void Parser::programFalseJump() {
    int labelIndex = emitLabel(-1);
    emitOperation(RPN_OP_JUMP_FALSE);
    labelStack.push_back(labelIndex);
}

void Parser::programElseJump() {
    if (labelStack.empty()) {
        semanticError(lastToken, "else without pending if label");
    }

    int falseLabelIndex = labelStack.back();
    labelStack.pop_back();

    int endLabelIndex = emitLabel(-1);
    emitOperation(RPN_OP_JUMP);
    patchLabel(falseLabelIndex, static_cast<int>(rpnString.size()));
    labelStack.push_back(endLabelIndex);
}

void Parser::programFinishIf() {
    if (labelStack.empty()) {
        semanticError(lastToken, "if without pending label");
    }

    int labelIndex = labelStack.back();
    labelStack.pop_back();
    patchLabel(labelIndex, static_cast<int>(rpnString.size()));
}

void Parser::programLoopStart() {
    labelStack.push_back(static_cast<int>(rpnString.size()));
}

void Parser::programFinishLoop() {
    if (labelStack.size() < 2) {
        semanticError(lastToken, "while without pending labels");
    }

    int falseLabelIndex = labelStack.back();
    labelStack.pop_back();

    int loopStart = labelStack.back();
    labelStack.pop_back();

    emitLabel(loopStart);
    emitOperation(RPN_OP_JUMP);
    patchLabel(falseLabelIndex, static_cast<int>(rpnString.size()));
}

void Parser::syntaxError(const std::string& expected) const {
    std::ostringstream out;
    out << "expected " << expected << ", got "
        << tokenTypeToString(currentToken.type);
    if (!currentToken.value.empty()) {
        out << " ('" << currentToken.value << "')";
    }
    throw ParserError(currentToken.line, currentToken.col, out.str());
}

void Parser::semanticError(const Token& token, const std::string& message) const {
    throw ParserError(token.line, token.col, message);
}
