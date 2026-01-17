%option c++
%option noyywrap
%option yylineno

%{
#include "util/Token.h"
%}

%%
[ \t\r\n]+                 ;

[0-9]+                     { return (int)TokenType::Integer; }

[a-zA-Z_][a-zA-Z0-9_]*     { return (int)TokenType::Ident; }

.                          { return (int)TokenType::Unknown; }
%%
