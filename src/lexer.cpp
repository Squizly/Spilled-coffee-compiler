#include <iostream>
#include <string>
#include <unordered_map>
#include <cctype>

// ============================================================
//  Коды лексем (TokenType)
// ============================================================
enum TokenType {
    T_EOF        =  0,
    T_ID         =  1,
    T_NUM_INT    =  2,
    T_NUM_FLOAT  =  3,
    T_IF         =  4,
    T_ELSE       =  5,
    T_WHILE      =  6,
    T_INPUT      =  7,
    T_PRINT      =  8,
    T_INT        =  9,
    T_FLOAT      = 10,
    T_PLUS       = 11,
    T_MINUS      = 12,
    T_MUL        = 13,
    T_DIV        = 14,
    T_ASSIGN     = 15,  // =
    T_EQ         = 16,  // ==
    T_NE         = 17,  // !=
    T_LT         = 18,  // <
    T_GT         = 19,  // >
    T_LE         = 20,  // <=
    T_GE         = 21,  // >=
    T_LPAR       = 22,  // (
    T_RPAR       = 23,  // )
    T_LBRACKET   = 24,  // [
    T_RBRACKET   = 25,  // ]
    T_LBRACE     = 26,  // {
    T_RBRACE     = 27,  // }
    T_COMMA      = 28,  // ,
    T_SEMICOLON  = 29,  // ;
    T_ERROR      = -1
};

// ============================================================
//  Структура токена
// ============================================================
struct Token {
    TokenType   type;
    std::string text;
    int         int_val;
    double      float_val;
    int         line;
    int         col;
};

// ============================================================
//  Классы входных символов (столбцы матрицы переходов)
// ============================================================
//  0  — буква или '_'
//  1  — цифра
//  2  — точка '.'
//  3  — разделители: , ;
//  4  — '='
//  5  — '<' или '>'
//  6  — '!'
//  7  — одиночные операторы: + - * /
//  8  — скобки: ( ) [ ] { }
//  9  — пробельные символы
//  10 — прочее (недопустимое)
//  11 — конец файла '\0'

static const int NUM_CLASSES = 12;

static int char_class(char c) {
    if (std::isalpha((unsigned char)c) || c == '_') return 0;
    if (std::isdigit((unsigned char)c))             return 1;
    if (c == '.')                                   return 2;
    if (c == ',' || c == ';')                       return 3;
    if (c == '=')                                   return 4;
    if (c == '<' || c == '>')                       return 5;
    if (c == '!')                                   return 6;
    if (c == '+' || c == '-' || c == '*' || c == '/') return 7;
    if (c == '(' || c == ')' || c == '[' || c == ']' ||
        c == '{' || c == '}')                       return 8;
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') return 9;
    if (c == '\0')                                  return 11;
    return 10;
}

// ============================================================
//  Состояния автомата
// ============================================================
//  0  — S0: начальное
//  1  — S1: идентификатор
//  2  — S2: целое число
//  3  — S3: после точки (ожидание цифры дробной части)
//  4  — S4: дробная часть числа
//  5  — S5: после = < >
//  6  — S6: после !
// -1  — финал (лексема готова)
// -2  — ошибка

static const int ST_FIN = -1;
static const int ST_ERR = -2;
static const int NUM_STATES = 7;

// ============================================================
//  Семантические действия
// ============================================================
//  0 — очистить буфер, записать текущий символ
//  1 — добавить текущий символ в буфер
//  2 — пропустить пробел
//  3 — выдать T_ID / ключевое слово + откат
//  4 — выдать T_NUM_INT + откат
//  5 — выдать T_NUM_FLOAT + откат
//  6 — выдать одиночный оператор / разделитель (без отката)
//  7 — выдать составной оператор (== <= >= !=)
//  8 — выдать одиночный = < > + откат
//  9 — ошибка T_ERROR

// ============================================================
//  Матрица переходов: transition[state][class] = {new_state, action}
// ============================================================
struct Cell { int next; int action; };

static const Cell transition[NUM_STATES][NUM_CLASSES] = {
//        буква/_      цифра        .            , ;          =            < >          !            + - * /      ( ) [ ] {}   пробел       прочее       EOF
/* S0 */ {{1,  0},   {2,  0},   {ST_ERR,9},{ST_FIN,6},{5,  0},   {5,  0},   {6,  0},   {ST_FIN,6},{ST_FIN,6},{0,  2},   {ST_ERR,9},{ST_FIN,6}},
/* S1 */ {{1,  1},   {1,  1},   {ST_FIN,3},{ST_FIN,3},{ST_FIN,3},{ST_FIN,3},{ST_FIN,3},{ST_FIN,3},{ST_FIN,3},{ST_FIN,3},{ST_ERR,9},{ST_FIN,3}},
/* S2 */ {{ST_ERR,9},{2,  1},   {3,  1},   {ST_FIN,4},{ST_FIN,4},{ST_FIN,4},{ST_FIN,4},{ST_FIN,4},{ST_FIN,4},{ST_FIN,4},{ST_ERR,9},{ST_FIN,4}},
/* S3 */ {{ST_ERR,9},{4,  1},   {ST_ERR,9},{ST_ERR,9},{ST_ERR,9},{ST_ERR,9},{ST_ERR,9},{ST_ERR,9},{ST_ERR,9},{ST_ERR,9},{ST_ERR,9},{ST_ERR,9}},
/* S4 */ {{ST_ERR,9},{4,  1},   {ST_ERR,9},{ST_FIN,5},{ST_FIN,5},{ST_FIN,5},{ST_FIN,5},{ST_FIN,5},{ST_FIN,5},{ST_FIN,5},{ST_ERR,9},{ST_FIN,5}},
/* S5 */ {{ST_FIN,8},{ST_FIN,8},{ST_FIN,8},{ST_FIN,8},{ST_FIN,7},{ST_FIN,8},{ST_FIN,8},{ST_FIN,8},{ST_FIN,8},{ST_FIN,8},{ST_ERR,9},{ST_FIN,8}},
/* S6 */ {{ST_ERR,9},{ST_ERR,9},{ST_ERR,9},{ST_ERR,9},{ST_FIN,7},{ST_ERR,9},{ST_ERR,9},{ST_ERR,9},{ST_ERR,9},{ST_ERR,9},{ST_ERR,9},{ST_ERR,9}},
};

// ============================================================
//  Вспомогательные таблицы
// ============================================================
static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"if",    T_IF},   {"else",  T_ELSE}, {"while", T_WHILE},
    {"input", T_INPUT},{"print", T_PRINT},{"int",   T_INT},
    {"float", T_FLOAT},
};

static TokenType single_char_token(char c) {
    switch (c) {
        case '+':  return T_PLUS;      case '-':  return T_MINUS;
        case '*':  return T_MUL;       case '/':  return T_DIV;
        case '(':  return T_LPAR;      case ')':  return T_RPAR;
        case '[':  return T_LBRACKET;  case ']':  return T_RBRACKET;
        case '{':  return T_LBRACE;    case '}':  return T_RBRACE;
        case ',':  return T_COMMA;     case ';':  return T_SEMICOLON;
        case '\0': return T_EOF;
        default:   return T_ERROR;
    }
}

static TokenType compound_token(const std::string& s) {
    if (s == "==") return T_EQ;
    if (s == "!=") return T_NE;
    if (s == "<=") return T_LE;
    if (s == ">=") return T_GE;
    return T_ERROR;
}

static TokenType single_rel_token(char c) {
    if (c == '=') return T_ASSIGN;
    if (c == '<') return T_LT;
    if (c == '>') return T_GT;
    return T_ERROR;
}

// ============================================================
//  Лексический анализатор
// ============================================================
class Lexer {
public:
    explicit Lexer(const std::string& source) : src(source), pos(0), line(1), col(1) {
        if (src.empty() || src.back() != '\0')
            src.push_back('\0');
    }

    Token next_token() {
        int         state;
        std::string buf;
        int         tok_line, tok_col;

    restart:
        state    = 0;
        buf.clear();
        tok_line = line;
        tok_col  = col;

        while (true) {
            char c      = advance();
            int  cls    = char_class(c);
            Cell cell   = transition[state][cls];
            int  action = cell.action;

            switch (action) {
                case 0: buf.clear(); buf += c; break;
                case 1: buf += c;               break;
                case 2: goto restart;           // пробел — перезапуск (tok_line/col обновятся)
                case 3: // ID или ключевое слово + откат
                    retract();
                    return make_token(keyword_or_id(buf), buf, tok_line, tok_col);
                case 4: // целое + откат
                    retract();
                    return make_token(T_NUM_INT, buf, tok_line, tok_col);
                case 5: // вещественное + откат
                    retract();
                    return make_token(T_NUM_FLOAT, buf, tok_line, tok_col);
                case 6: // одиночный символ, без отката
                    return make_token(single_char_token(c), std::string(1, c), tok_line, tok_col);
                case 7: // составной оператор
                    buf += c;
                    return make_token(compound_token(buf), buf, tok_line, tok_col);
                case 8: // одиночный = < > + откат
                    retract();
                    return make_token(single_rel_token(buf[0]), buf, tok_line, tok_col);
                case 9: // ошибка
                    return make_error(c, tok_line, tok_col);
            }

            state = cell.next;
        }
    }

private:
    std::string src;
    int pos, line, col;

    char advance() {
        char c = src[pos++];
        if (c == '\n') { line++; col = 1; } else { col++; }
        return c;
    }

    void retract() {
        if (pos > 0) {
            pos--;
            if (src[pos] == '\n') { line--; col = 1; } else { col--; }
        }
    }

    static TokenType keyword_or_id(const std::string& text) {
        auto it = KEYWORDS.find(text);
        return (it != KEYWORDS.end()) ? it->second : T_ID;
    }

    static Token make_token(TokenType type, const std::string& text, int l, int c) {
        Token t;
        t.type      = type;
        t.text      = text;
        t.int_val   = 0;
        t.float_val = 0.0;
        t.line      = l;
        t.col       = c;
        if (type == T_NUM_INT)   t.int_val   = std::stoi(text);
        if (type == T_NUM_FLOAT) t.float_val = std::stod(text);
        return t;
    }

    static Token make_error(char c, int l, int col) {
        Token t;
        t.type      = T_ERROR;
        t.text      = std::string(1, c);
        t.int_val   = 0;
        t.float_val = 0.0;
        t.line      = l;
        t.col       = col;
        return t;
    }
};

// ============================================================
//  Тесты
// ============================================================
static std::string token_name(TokenType t) {
    switch (t) {
        case T_EOF:       return "T_EOF";       case T_ERROR:     return "T_ERROR";
        case T_ID:        return "T_ID";         case T_NUM_INT:   return "T_NUM_INT";
        case T_NUM_FLOAT: return "T_NUM_FLOAT";  case T_IF:        return "T_IF";
        case T_ELSE:      return "T_ELSE";       case T_WHILE:     return "T_WHILE";
        case T_INPUT:     return "T_INPUT";      case T_PRINT:     return "T_PRINT";
        case T_INT:       return "T_INT";        case T_FLOAT:     return "T_FLOAT";
        case T_PLUS:      return "T_PLUS";       case T_MINUS:     return "T_MINUS";
        case T_MUL:       return "T_MUL";        case T_DIV:       return "T_DIV";
        case T_ASSIGN:    return "T_ASSIGN";     case T_EQ:        return "T_EQ";
        case T_NE:        return "T_NE";         case T_LT:        return "T_LT";
        case T_GT:        return "T_GT";         case T_LE:        return "T_LE";
        case T_GE:        return "T_GE";         case T_LPAR:      return "T_LPAR";
        case T_RPAR:      return "T_RPAR";       case T_LBRACKET:  return "T_LBRACKET";
        case T_RBRACKET:  return "T_RBRACKET";   case T_LBRACE:    return "T_LBRACE";
        case T_RBRACE:    return "T_RBRACE";     case T_COMMA:     return "T_COMMA";
        case T_SEMICOLON: return "T_SEMICOLON";
        default:          return "UNKNOWN";
    }
}

static void run_test(const std::string& title, const std::string& src) {
    std::cout << "\n=== " << title << " ===\n";
    std::cout << "Вход: " << src << "\n\n";
    Lexer lex(src);
    while (true) {
        Token tok = lex.next_token();
        std::cout << "[" << tok.line << ":" << tok.col << "] "
                  << token_name(tok.type)
                  << "\t\"" << tok.text << "\"";
        if (tok.type == T_NUM_INT)   std::cout << "  => int=" << tok.int_val;
        if (tok.type == T_NUM_FLOAT) std::cout << "  => float=" << tok.float_val;
        if (tok.type == T_ERROR)     std::cout << "  <-- ОШИБКА (строка " << tok.line << ", позиция " << tok.col << ")";
        std::cout << "\n";
        if (tok.type == T_EOF || tok.type == T_ERROR) break;
    }
}

int main() {
    run_test("Арифметика",
        "x = (a + b) * 3.14 - 2;");

    run_test("Условный оператор",
        "if (x >= 10) { y = x * 2; } else { y = 0; }");

    run_test("Цикл и ввод/вывод",
        "int n;\ninput(n);\nwhile (n != 0) {\n  print(n);\n  n = n - 1;\n}");

    run_test("Массив",
        "int a[10];\na[i] = a[i-1] + 1;");

    run_test("Вещественные числа",
        "float x;\nx = 3.14159;\ny = 0.5 + 1.0;");

    run_test("Операторы сравнения",
        "a == b; a != b; a <= b; a >= b;");

    run_test("Ошибочный символ",
        "x = @y + 5;");

    run_test("Неполное вещественное (5.)",
        "x = 5. + 1;");

    return 0;
}