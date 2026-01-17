#include <iostream>
#include "tokenizer/impl/flex/FlexTokenizer.h"
#include <fstream>
#include <memory>

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);

    // Ввод: либо файл из argv[1], либо stdin
    std::unique_ptr<std::istream> file;
    std::istream* in = &std::cin;

    if (argc >= 2) {
        file = std::make_unique<std::ifstream>(argv[1]);
        if (!*file) {
            std::cerr << "Cannot open file: " << argv[1] << "\n";
            return 1;
        }
        in = file.get();
    }

    // Выбираем токенайзер (пока только Flex)
    std::unique_ptr<ITokenizer> tz = std::make_unique<FlexTokenizer>(*in);

    // Печатаем токены
    while (true) {
        Token t = tz->next();
        std::cout << toString(t.type) << "  '" << t.text << "'"
                  << "  (line " << t.line << ", col " << t.col << ")\n";

        if (t.type == TokenType::Eof) break;
    }

    return 0;
}