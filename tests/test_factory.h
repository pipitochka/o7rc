#pragma once
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <tokenizer/ITokenizer.h>
#include <parser/IParser.h>
#include <util/ast/Ast.h>

#ifdef USE_FLEX
#include <tokenizer/impl/flex/FlexTokenizer.h>
#endif

#ifdef USE_HAND_TOKENIZER
#include <tokenizer/impl/hand/HandTokenizer.h>
#endif

#ifdef USE_BISON
#include <parser/impl/bison/BisonParser.h>
#endif

#ifdef USE_HAND_PARSER
#include <parser/impl/hand/HandParser.h>
#endif

enum class TokenizerKind { Flex, Hand };
enum class ParserKind { Bison, Hand };

inline const char* tokenizerName(TokenizerKind k) {
    switch (k) {
    case TokenizerKind::Flex: return "Flex";
    case TokenizerKind::Hand: return "Hand";
    }
    return "?";
}

inline const char* parserName(ParserKind k) {
    switch (k) {
    case ParserKind::Bison: return "Bison";
    case ParserKind::Hand:  return "Hand";
    }
    return "?";
}

inline std::vector<TokenizerKind> availableTokenizers() {
    std::vector<TokenizerKind> v;
#ifdef USE_FLEX
    v.push_back(TokenizerKind::Flex);
#endif
#ifdef USE_HAND_TOKENIZER
    v.push_back(TokenizerKind::Hand);
#endif
    return v;
}

inline std::vector<ParserKind> availableParsers() {
    std::vector<ParserKind> v;
#ifdef USE_BISON
    v.push_back(ParserKind::Bison);
#endif
#ifdef USE_HAND_PARSER
    v.push_back(ParserKind::Hand);
#endif
    return v;
}

inline ITokenizerPtr makeTokenizer(TokenizerKind kind, std::istream& in) {
    switch (kind) {
#ifdef USE_FLEX
    case TokenizerKind::Flex:
        return std::make_shared<FlexTokenizer>(in);
#endif
#ifdef USE_HAND_TOKENIZER
    case TokenizerKind::Hand:
        return std::make_shared<HandTokenizer>(in);
#endif
    default:
        return nullptr;
    }
}

inline std::unique_ptr<IParser> makeParser(ParserKind kind) {
    switch (kind) {
#ifdef USE_BISON
    case ParserKind::Bison:
        return std::make_unique<BisonParser>();
#endif
#ifdef USE_HAND_PARSER
    case ParserKind::Hand:
        return std::make_unique<HandParser>();
#endif
    default:
        return nullptr;
    }
}
