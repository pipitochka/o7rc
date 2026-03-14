#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include <parser/IParser.h>
#include <tokenizer/ITokenizer.h>
#include <util/ast/Ast.h>

#ifdef USE_FLEX
#include <tokenizer/impl/flex/FlexTokenizer.h>
#endif

#ifdef USE_BISON
#include <parser/impl/bison/BisonParser.h>
#endif

#ifdef USE_DEBUG
#include <tokenizer/impl/debug/BufferedTokenizer.h>
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

    ITokenizerPtr tokenizer;

#ifdef USE_FLEX
    tokenizer = std::make_unique<FlexTokenizer>(*file);
#endif

#ifdef USE_DEBUG
    auto buffered = std::make_unique<BufferedTokenizer>(std::move(tokenizer));

    try {
        buffered->check();
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\n";
        return 1;
    }
    tokenizer = std::move(buffered);

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

    auto result = parser->parse(tokenizer);
}
