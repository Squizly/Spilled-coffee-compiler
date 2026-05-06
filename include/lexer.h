#pragma once
#include "common.h"
#include <string>
#include <map>

// Группы символов (столбцы матрицы FSM)
enum CharClass {
    C_LETTER_UNDERSCORE = 0,
    C_DIGIT = 1,
    C_DOT = 2,
    C_DELIM = 3,
    C_EQ = 4,
    C_COMP = 5,
    C_EXCL = 6,
    C_MATH = 7,
    C_BRACKET = 8,
    C_SPACE = 9,
    C_OTHER = 10
};

// Класс лексического анализатора
class Lexer {
private:
    std::string source; // исходный код
    int pos;            // текущая позиция в исходнике
    int line;           // строка
    int col;            // номер символа в строке

    // Таблица ключевых слов
    std::map<std::string, TokenType> keywords;

    // Вспомогательные методы
    CharClass getCharClass(char c);
    char getChar();
    void ungetChar(char c);
    TokenType getSingleOpToken(char c);
    TokenType getCompoundOpToken(const std::string& op);
    
public:
    // Конструктор принимает исходный код в виде строки
    Lexer(const std::string& src);

    Token getNextToken();
};