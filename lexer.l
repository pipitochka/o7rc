%option c++
%option noyywrap
%option yylineno

%{
#include <string>
#include <vector>
#include <iostream>

// Объявляем внешний вектор для сбора токенов
extern std::vector<std::pair<std::string, std::string>> tokens;
%}

%%

[0-9]+          { tokens.emplace_back("NUMBER", yytext); }
[a-zA-Z_][a-zA-Z0-9_]*  { tokens.emplace_back("IDENT", yytext); }
[ \t\n]         ;  // skip whitespace
.               { tokens.emplace_back("UNKNOWN", std::string(1, *yytext)); }

%%