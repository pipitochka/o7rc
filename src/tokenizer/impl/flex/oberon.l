%option c++
%option noyywrap
%option yylineno

%{
#include "util/Token.h"
%}

%%
[ \t\r\n]+                 ;

"MODULE"                   { return (int)TokenType::KW_MODULE; }
"BEGIN"                    { return (int)TokenType::KW_BEGIN;  }
"END"                      { return (int)TokenType::KW_END;    }

";"                        { return (int)TokenType::Semicolon; }
"."                        { return (int)TokenType::Dot; }

[a-zA-Z_][a-zA-Z0-9_]*     { return (int)TokenType::Ident; }

.                          { return (int)TokenType::Unknown; }
%%
