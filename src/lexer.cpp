#include "../include/lexer.h"
#include <cctype>

// Структура для ячейки матрицы
struct Transition {
    int nextState;
    int action;
};

// МАТРИЦА ПЕРЕХОДОВ (Строки: состояния S0-S6, Столбцы: классы символов)
const Transition StateTable[7][11] = {
    // S0: Let/Und, Dig,   Dot,     Delim,   Eq,      <>,      !,       Math,    Brack,   Spc,     Oth
    /*S0*/{{1, 0}, {2, 0}, {-1, 9}, {-1, 6}, {5, 0},  {5, 0},  {6, 0},  {-1, 6}, {-1, 6}, {0, 2},  {-1, 9}},
    /*S1*/{{1, 1}, {1, 1}, {-1, 3}, {-1, 3}, {-1, 3}, {-1, 3}, {-1, 3}, {-1, 3}, {-1, 3}, {-1, 3}, {-1, 9}},
    /*S2*/{{-1,9}, {2, 1}, {3, 1},  {-1, 4}, {-1, 4}, {-1, 4}, {-1, 4}, {-1, 4}, {-1, 4}, {-1, 4}, {-1, 9}},
    /*S3*/{{-1,9}, {4, 1}, {-1, 9}, {-1, 9}, {-1, 9}, {-1, 9}, {-1, 9}, {-1, 9}, {-1, 9}, {-1, 9}, {-1, 9}},
    /*S4*/{{-1,9}, {4, 1}, {-1, 9}, {-1, 5}, {-1, 5}, {-1, 5}, {-1, 5}, {-1, 5}, {-1, 5}, {-1, 5}, {-1, 9}},
    /*S5*/{{-1,8}, {-1,8}, {-1, 8}, {-1, 8}, {-1, 7}, {-1, 8}, {-1, 8}, {-1, 8}, {-1, 8}, {-1, 8}, {-1, 9}},
    /*S6*/{{-1,9}, {-1,9}, {-1, 9}, {-1, 9}, {-1, 7}, {-1, 9}, {-1, 9}, {-1, 9}, {-1, 9}, {-1, 9}, {-1, 9}}
};

Lexer::Lexer(const std::string& src) : source(src), pos(0), line(1), col(1) 
{
    // Ключевые слова
    keywords = {
        {"if", T_IF}, 
        {"else", T_ELSE}, 
        {"while", T_WHILE},
        {"input", T_INPUT}, 
        {"print", T_PRINT},
        {"int", T_INT}, 
        {"float", T_FLOAT},
        {"sqrt", T_SQRT}, 
        {"exp", T_EXP}, 
        {"log", T_LOG}
    };
}

CharClass Lexer::getCharClass(char c) {
    if (isalpha(c) || c == '_') return C_LETTER_UNDERSCORE;
    if (isdigit(c)) return C_DIGIT;
    if (c == '.') return C_DOT;
    if (c == ',' || c == ';') return C_DELIM;
    if (c == '=') return C_EQ;
    if (c == '<' || c == '>') return C_COMP;
    if (c == '!') return C_EXCL;
    if (c == '+' || c == '-' || c == '*' || c == '/') return C_MATH;
    if (c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}') return C_BRACKET;
    if (isspace(c) || c == '\0') return C_SPACE; 
    return C_OTHER;
}

char Lexer::getChar()
{
    if (pos >= source.length()) return '\0';
    
    char c = source[pos++];
    
    if (c == '\n') { 
        line++; 
        col = 1;
    }
    else col++;

    return c;
}

void Lexer::ungetChar(char c) {
    if (c == '\0') return;
    pos--;
    if (c == '\n') { line--; col = 1; }
    else { col--; }
}

TokenType Lexer::getSingleOpToken(char c) {
    switch (c) {
        case '+': return T_PLUS;    case '-': return T_MINUS;
        case '*': return T_MUL;     case '/': return T_DIV;
        case '(': return T_LPAR;    case ')': return T_RPAR;
        case '[': return T_LBRACKET;case ']': return T_RBRACKET;
        case '{': return T_LBRACE;  case '}': return T_RBRACE;
        case ',': return T_COMMA;   case ';': return T_SEMICOLON;
        case '=': return T_ASSIGN;  case '<': return T_LT;  case '>': return T_GT;
        default: return T_ERROR;
    }
}

TokenType Lexer::getCompoundOpToken(const std::string& op) {
    if (op == "==") return T_EQ;
    if (op == "!=") return T_NE;
    if (op == "<=") return T_LE;
    if (op == ">=") return T_GE;
    return T_ERROR;
}

Token Lexer::getNextToken() {
    if (pos >= source.length()) return { T_EOF, "EOF", line, col };

    int state = 0;             
    std::string buffer = "";   
    int startLine = line;
    int startCol = col;

    while (true) {
        char c = getChar();
        if (c == '\0' && state == 0) return { T_EOF, "EOF", line, col };

        CharClass charClass = getCharClass(c);
        Transition tr = StateTable[state][charClass];
        
        int action = tr.action;
        int nextState = tr.nextState;

        // Семантические действия
        switch (action) {
            case 0: // Очистить буфер
                buffer.clear(); buffer += c;
                startLine = line; startCol = (c == '\n') ? col : col - 1;
                break;
            case 1: // Накопление
                buffer += c; break;
            case 2: // Пропустить пробел
                startLine = line; startCol = col; break;
            case 3: // ID или Ключевое слово + ОТКАТ
                ungetChar(c);
                if (keywords.find(buffer) != keywords.end()) {
                    return { keywords[buffer], buffer, startLine, startCol };
                }
                return { T_ID, buffer, startLine, startCol };
            case 4: // Целое число + ОТКАТ
                ungetChar(c); return { T_NUM_INT, buffer, startLine, startCol };
            case 5: // Вещественное число + ОТКАТ
                ungetChar(c); return { T_NUM_FLOAT, buffer, startLine, startCol };
            case 6: // Одиночный оператор (без отката)
                buffer += c; return { getSingleOpToken(c), buffer, startLine, startCol };
            case 7: // Составной оператор (==, <=, >=, !=)
                buffer += c; return { getCompoundOpToken(buffer), buffer, startLine, startCol };
            case 8: // Одиночный =, <, > + ОТКАТ
                ungetChar(c); return { getSingleOpToken(buffer[0]), buffer, startLine, startCol };
            case 9: // ОШИБКА
                buffer += c; return { T_ERROR, buffer, startLine, startCol };
        }

        if (nextState != -1) state = nextState;
    }
}

