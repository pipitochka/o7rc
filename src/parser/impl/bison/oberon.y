%{
#include "parser/impl/bison/ParserContext.h"
#include <cstdio>

void yyerror(ParserContext* /*ctx*/, const char* msg) {
  std::fprintf(stderr, "parse error: %s\n", msg);
}
%}

%define api.pure full
%parse-param { ParserContext* ctx }
%lex-param   { ParserContext* ctx }

%code provides {
  int yylex(YYSTYPE* yylval, ParserContext* ctx);
}

%token TOK_IDENT
%token TOK_KW_MODULE TOK_KW_BEGIN TOK_KW_END
%token TOK_SEMICOLON TOK_DOT
%token TOK_UNKNOWN

%%

input  : module ;

module
  : TOK_KW_MODULE TOK_IDENT TOK_SEMICOLON
    TOK_KW_BEGIN
    TOK_KW_END TOK_IDENT TOK_DOT
  ;

%%
