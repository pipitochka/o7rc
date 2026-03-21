#include "FlexTokenizer.h"
#include "Positions.h"

#include <cerrno>
#include <cstdlib>
#include <string>

FlexTokenizer::FlexTokenizer(std::istream &in) :
    lexer(&in) // yyFlexLexer умеет читать из istream*
{
    oberon_reset_positions();
}

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

    if (t.type == TokenType::Real) {
        t.realValue = std::stod(t.text);
    } else if (t.type == TokenType::Integer) {
        if (t.text.back() == 'H') {
            std::string hexStr = t.text.substr(0, t.text.size() - 1);
            t.intValue = std::stoll(hexStr, nullptr, 16);
        } else {
            t.intValue = std::stoll(t.text);
        }
    } else if (t.type == TokenType::String) {
        if (t.text.back() == 'X') {
            std::string hexStr = t.text.substr(0, t.text.size() - 1); // Отрезаем 'X'
            char ch = static_cast<char>(std::stoi(hexStr, nullptr, 16));
            t.text = std::string(1, ch);
        } else {
            if (t.text.size() >= 2) {
                t.text = t.text.substr(1, t.text.size() - 2);
            }
        }
    }

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
