#include "FlexTokenizer.h"

FlexTokenizer::FlexTokenizer(std::istream& in)
    : lexer(&in)   // yyFlexLexer умеет читать из istream*
{}

Token FlexTokenizer::readOne() {
    Token t;

    int rc = lexer.yylex(); // 0 => EOF
    if (rc == 0) {
        t.type = TokenType::Eof;
        t.text.clear();
        t.line = (std::uint32_t)lexer.lineno();
        t.col = 1;
        return t;
    }

    t.type = static_cast<TokenType>(rc);
    t.text = lexer.YYText();                  // копия лексемы
    t.line = (std::uint32_t)lexer.lineno();   // line (col пока упрощённо)
    t.col = 1;

    return t;
}

Token FlexTokenizer::peek() {
    if (!hasLA) {
        la = readOne();
        hasLA = true;
    }
    return la;
}

Token FlexTokenizer::next() {
    Token t = peek();
    hasLA = false;
    return t;
}
