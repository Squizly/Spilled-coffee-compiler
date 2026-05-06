#include "../include/lexer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

int main() {
    std::string filePath = "tests/test_calc.txt";
    
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл: " << filePath << std::endl;
        return 1;
    }

    // Читаем весь файл в строку
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceCode = buffer.str();

    Lexer lexer(sourceCode);
    Token t;

    std::cout << std::left << std::setw(15) << "TOKEN CODE" 
              << std::setw(20) << "VALUE" 
              << std::setw(10) << "LINE" 
              << std::setw(10) << "COL" << "\n";
    std::cout << std::string(55, '-') << "\n";

    // Цикл получения токенов
    do {
        t = lexer.getNextToken();
        
        std::cout << std::left << std::setw(15) << t.type 
                  << std::setw(20) << t.value 
                  << std::setw(10) << t.line 
                  << std::setw(10) << t.col << "\n";
        
        // Остановка и вывод диагностики при ошибке
        if (t.type == T_ERROR) {
            std::cerr << "\n>>> ЛЕКСИЧЕСКАЯ ОШИБКА <<<\n"
                      << "Строка: " << t.line << ", Символ: " << t.col 
                      << " -> Недопустимый символ или формат: '" << t.value << "'\n";
            break;
        }

    } while (t.type != T_EOF);

    return 0;
}