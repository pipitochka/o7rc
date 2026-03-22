#include <util/Token.h>

#include "ParserContext.h"
#include "oberon.tab.h"

static int mapTokenType(TokenType tt) {
    // Сокращение для удобства
    using TT = TokenType;

    switch (tt) {
        case TT::Eof:
            return 0; // Bison требует 0 для конца файла
        case TT::Unknown:
            return TOK_UNKNOWN;

        // --- Литералы ---
        case TT::Ident:
            return TOK_IDENT;
        case TT::Integer:
            return TOK_INTEGER;
        case TT::Real:
            return TOK_REAL;
        case TT::String:
            return TOK_STRING;

        // --- Операторы ---
        case TT::Plus:
            return TOK_PLUS;
        case TT::Minus:
            return TOK_MINUS;
        case TT::Star:
            return TOK_STAR;
        case TT::Slash:
            return TOK_SLASH;

        case TT::Eq:
            return TOK_EQ;
        case TT::Neq:
            return TOK_NEQ;
        case TT::Lt:
            return TOK_LT;
        case TT::Le:
            return TOK_LE;
        case TT::Gt:
            return TOK_GT;
        case TT::Ge:
            return TOK_GE;

        case TT::Assign:
            return TOK_ASSIGN; // :=
        case TT::Caret:
            return TOK_CARET; // ^
        case TT::Amp:
            return TOK_AMP; // &
        case TT::Tilde:
            return TOK_TILDE; // ~

        // ВАЖНОЕ ИСПРАВЛЕНИЕ:
        case TT::Bar:
            return TOK_KW_BAR; // Символ |
        case TT::Range:
            return TOK_RANGE; // Символ ..

        // --- Пунктуация ---
        case TT::LParen:
            return TOK_LPAREN;
        case TT::RParen:
            return TOK_RPAREN;
        case TT::LBrack:
            return TOK_LBRACK;
        case TT::RBrack:
            return TOK_RBRACK;
        case TT::LBrace:
            return TOK_LBRACE; // {
        case TT::RBrace:
            return TOK_RBRACE; // }
        case TT::Comma:
            return TOK_COMMA;
        case TT::Semicolon:
            return TOK_SEMICOLON;
        case TT::Colon:
            return TOK_COLON; // :
        case TT::Dot:
            return TOK_DOT;

        // --- Ключевые слова (строго по Token.h) ---
        case TT::KW_ARRAY:
            return TOK_KW_ARRAY;
        case TT::KW_BEGIN:
            return TOK_KW_BEGIN;
        case TT::KW_BY:
            return TOK_KW_BY;
        case TT::KW_CASE:
            return TOK_KW_CASE;
        case TT::KW_CONST:
            return TOK_KW_CONST;
        case TT::KW_DIV:
            return TOK_KW_DIV;
        case TT::KW_DO:
            return TOK_KW_DO;
        case TT::KW_ELSE:
            return TOK_KW_ELSE;
        case TT::KW_ELSIF:
            return TOK_KW_ELSIF;
        case TT::KW_END:
            return TOK_KW_END;
        case TT::KW_FALSE:
            return TOK_KW_FALSE;
        case TT::KW_FOR:
            return TOK_KW_FOR;
        case TT::KW_IF:
            return TOK_KW_IF;
        case TT::KW_IMPORT:
            return TOK_KW_IMPORT;
        case TT::KW_IN:
            return TOK_KW_IN;
        case TT::KW_IS:
            return TOK_KW_IS;
        case TT::KW_MOD:
            return TOK_KW_MOD;
        case TT::KW_MODULE:
            return TOK_KW_MODULE;
        case TT::KW_NIL:
            return TOK_KW_NIL;
        case TT::KW_OF:
            return TOK_KW_OF;
        case TT::KW_OR:
            return TOK_KW_OR;
        case TT::KW_POINTER:
            return TOK_KW_POINTER;
        case TT::KW_PROCEDURE:
            return TOK_KW_PROCEDURE;
        case TT::KW_RECORD:
            return TOK_KW_RECORD;
        case TT::KW_REPEAT:
            return TOK_KW_REPEAT;
        case TT::KW_RETURN:
            return TOK_KW_RETURN;
        case TT::KW_THEN:
            return TOK_KW_THEN;
        case TT::KW_TO:
            return TOK_KW_TO;
        case TT::KW_TRUE:
            return TOK_KW_TRUE;
        case TT::KW_TYPE:
            return TOK_KW_TYPE;
        case TT::KW_UNTIL:
            return TOK_KW_UNTIL;
        case TT::KW_VAR:
            return TOK_KW_VAR;
        case TT::KW_WHILE:
            return TOK_KW_WHILE;

        default:
            return TOK_UNKNOWN;
    }
}

int yylex(YYSTYPE *yylval, ParserContext *ctx) {
    Token t = ctx->tz->next();
    int tok = mapTokenType(t.type);

    // значения нужны только для некоторых токенов
    if (tok == TOK_IDENT || tok == TOK_INTEGER || tok == TOK_REAL || tok == TOK_STRING) {
        yylval->tok = new Token(std::move(t));
    }
    return tok;
}
