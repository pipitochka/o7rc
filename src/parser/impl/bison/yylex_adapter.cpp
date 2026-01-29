#include <tokenizer/ITokenizer.h>
#include <util/Token.h>
#include "ParserContext.h"

#include "oberon.tab.h" // bison-generated

static int mapToken(const Token &t) {
    switch (t.type) {
        case TokenType::Eof:
            return 0;

        case TokenType::Ident:
            return TOK_IDENT;

        case TokenType::KW_MODULE:
            return TOK_KW_MODULE;
        case TokenType::KW_BEGIN:
            return TOK_KW_BEGIN;
        case TokenType::KW_END:
            return TOK_KW_END;

        case TokenType::Semicolon:
            return TOK_SEMICOLON;
        case TokenType::Dot:
            return TOK_DOT;

        default:
            return TOK_UNKNOWN;
    }
}

int yylex(YYSTYPE * /*yylval*/, ParserContext *ctx) {
    Token t = ctx->tz->next();
    return mapToken(t);
}
