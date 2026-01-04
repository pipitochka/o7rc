#include <iostream>
#include <vector>
#include <string>

#include <FlexLexer.h>

// Глобальный вектор токенов — совместно используется лексером и main
std::vector<std::pair<std::string, std::string>> tokens;

int main() {
    yyFlexLexer lexer;
    while (lexer.yylex() != 0) {
        // токены уже добавляются в `tokens` из правил в lexer.l
    }

    std::cout << "Parsed tokens:\n";
    for (const auto& [type, value] : tokens) {
        std::cout << type << " -> '" << value << "'\n";
    }

    return 0;
}