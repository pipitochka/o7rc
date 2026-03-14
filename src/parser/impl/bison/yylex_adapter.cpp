#include <util/Token.h>
#include "ParserContext.h"
#include "oberon.tab.h"

static int mapTokenType(TokenType tt) {
    using TT = TokenType;
    switch (tt) {
        case TT::Eof: return 0;

        case TT::Ident:   return TOK_IDENT;
        case TT::Integer: return TOK_INTEGER;
        case TT::Real:    return TOK_REAL;
        case TT::String:  return TOK_STRING;

        case TT::KW_MODULE: return TOK_KW_MODULE;
        case TT::KW_BEGIN:  return TOK_KW_BEGIN;
        case TT::KW_END:    return TOK_KW_END;
        case TT::KW_DIV:    return TOK_KW_DIV;
        case TT::KW_MOD:    return TOK_KW_MOD;
        case TT::KW_OR:     return TOK_KW_OR;

        case TT::Plus:   return TOK_PLUS;
        case TT::Minus:  return TOK_MINUS;
        case TT::Star:   return TOK_STAR;
        case TT::Slash:  return TOK_SLASH;

        case TT::Eq:     return TOK_EQ;
        case TT::Neq:    return TOK_NEQ;
        case TT::Lt:     return TOK_LT;
        case TT::Le:     return TOK_LE;
        case TT::Gt:     return TOK_GT;
        case TT::Ge:     return TOK_GE;

        case TT::Assign: return TOK_ASSIGN;
        case TT::Amp:    return TOK_AMP;
        case TT::Tilde:  return TOK_TILDE;
        case TT::Caret:  return TOK_CARET;

        case TT::LParen: return TOK_LPAREN;
        case TT::RParen: return TOK_RPAREN;
        case TT::LBrack: return TOK_LBRACK;
        case TT::RBrack: return TOK_RBRACK;

        case TT::Comma:     return TOK_COMMA;
        case TT::Semicolon: return TOK_SEMICOLON;
        case TT::Dot:       return TOK_DOT;

        default: return TOK_UNKNOWN;
    }
}

int yylex(YYSTYPE* yylval, ParserContext* ctx) {
    Token t = ctx->tz->next();
    int tok = mapTokenType(t.type);

    // значения нужны только для некоторых токенов
    if (tok == TOK_IDENT || tok == TOK_INTEGER || tok == TOK_REAL || tok == TOK_STRING) {
        yylval->tok = new Token(std::move(t));
    }
    return tok;
}
