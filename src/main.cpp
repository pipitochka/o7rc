#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include "tokenizer/impl/flex/FlexTokenizer.h"
#include "parser/impl/bison/BisonParser.h"

int main() {
    std::ios::sync_with_stdio(false);

    std::cout << "Enter path to source file: ";
    std::string path;
    if (!std::getline(std::cin, path) || path.empty()) {
        std::cerr << "No path provided\n";
        return 1;
    }

    auto file = std::make_unique<std::ifstream>(path);
    if (!*file) {
        std::cerr << "Cannot open file: " << path << "\n";
        return 1;
    }

    FlexTokenizer tz(*file);
    BisonParser parser;

    bool ok = parser.parse(tz);
    std::cout << (ok ? "Parse OK\n" : "Parse FAILED\n");
    return ok ? 0 : 1;
}
