#include "FlexTokenizer.h"
#include "Positions.h"

#include <cerrno>
#include <cstdlib>
#include <string>

FlexTokenizer::FlexTokenizer(std::istream &in) :
    lexer(&in) // yyFlexLexer умеет читать из istream*
{}

Token FlexTokenizer::readOne() {
    Token t;

    int rc = lexer.yylex(); // 0 => EOF
    if (rc == 0) {
        t.type = TokenType::Eof;
        t.text.clear();
        t.line = oberon_cur_line();
        t.col = oberon_cur_col();
        return t;
    }

    t.type = static_cast<TokenType>(rc);
    t.text = lexer.YYText();
    t.line = oberon_tok_line(); // начало лексемы
    t.col = oberon_tok_col();

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
