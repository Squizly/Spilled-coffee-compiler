#pragma once
#include <string>

// Всевозможные токены
enum TokenType {
    // === === === Служебные === === === ///
    T_ERROR = -1, T_EOF = 0,

    // === === === Числа и идентификаторы === === === //
    T_ID = 1,        // abcd102_9j
    T_NUM_INT = 2,   // 10
    T_NUM_FLOAT = 3, // 5.5

    // === === === Ключевые слова и типы == === === //
    T_IF = 4, T_ELSE = 5,     // if else
    T_WHILE = 6,              // while
    T_INPUT = 7, T_PRINT = 8, // input print
    T_INT = 9, T_FLOAT = 10,  // int float
    
    // === === === Операторы и сравнения === === === //
    T_PLUS = 11, T_MINUS = 12, // + -
    T_MUL = 13, T_DIV = 14,    // * / 
    T_ASSIGN = 15,             // =
    T_EQ = 16, T_NE = 17,      // == != 
    T_LT = 18, T_GT = 19,      // < >
    T_LE = 20, T_GE = 21,      // <= >=

    // === === === Разделители === === === //
    T_LPAR = 22, T_RPAR = 23,         // ( )
    T_LBRACKET = 24, T_RBRACKET = 25, // [ ] 
    T_LBRACE = 26, T_RBRACE = 27,     // { }
    T_COMMA = 28, T_SEMICOLON = 29,   // , ;

    // === === === Стандартные функции
    T_SQRT = 30,  // sqrt
    T_EXP = 31,   // exp
    T_LOG = 32    // log
};

// Структура токена
struct Token {
    TokenType type;     // Код лексемы
    std::string value;  // Значение
    int line;           // Номер строки
    int col;            // Номер символа в строке
};