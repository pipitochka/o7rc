#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include <parser/IParser.h>
#include <tokenizer/ITokenizer.h>

#ifdef USE_FLEX
#include "tokenizer/impl/flex/FlexTokenizer.h"
#endif

#ifdef USE_BISON
#include "parser/impl/bison/BisonParser.h"
#endif

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

    std::unique_ptr<ITokenizer> tokenizer;

#ifdef USE_FLEX
    tokenizer = std::make_unique<FlexTokenizer>(*file);
#endif

    std::unique_ptr<IParser> parser;

#ifdef USE_BISON
    parser = std::make_unique<BisonParser>();
#endif

    if (!tokenizer) {
        std::cerr << "No tokenizer built/selected\n";
        return 1;
    }

    if (!parser) {
        std::cerr << "No parser built/selected\n";
        return 1;
    }

    const bool ok = parser->parse(*tokenizer);

    std::cout << (ok ? "Parse OK\n" : "Parse FAILED\n");
    return ok ? 0 : 1;
}
