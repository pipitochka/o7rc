%option c++
%option noyywrap
%option nodefault

%{
#include "util/Token.h"
#include "tokenizer/impl/flex/Positions.h"
#include <cstdint>

static std::uint32_t g_line = 1;
static std::uint32_t g_col  = 1;

static std::uint32_t g_tokLine = 1;
static std::uint32_t g_tokCol  = 1;

static int commentDepth = 0;

std::uint32_t oberon_tok_line() { return g_tokLine; }
std::uint32_t oberon_tok_col()  { return g_tokCol;  }
std::uint32_t oberon_cur_line() { return g_line;    }
std::uint32_t oberon_cur_col()  { return g_col;     }

#define YY_USER_ACTION                                            \
    {                                                             \
        g_tokLine = g_line;                                       \
        g_tokCol  = g_col;                                        \
        for (int i = 0; i < yyleng; ++i) {                        \
            if (yytext[i] == '\n') { ++g_line; g_col = 1; }       \
            else { ++g_col; }                                     \
        }                                                         \
    }
%}

%x COMMENT

DIGIT     [0-9]
HEXDIGIT  [0-9A-F]
LETTER    [A-Za-z_]
IDREST    [A-Za-z0-9_]

%%

[ \t\r]+                    ;  /* skip */
\n+                         ;  /* skip */

"(*"                        { commentDepth = 1; BEGIN(COMMENT); }
<COMMENT>"(*"               { ++commentDepth; }
<COMMENT>"*)"               { if (--commentDepth == 0) BEGIN(INITIAL); }
<COMMENT>\n+                ;  /* skip */
<COMMENT>.                  ;  /* skip */

"ARRAY"                     { return (int)TokenType::KW_ARRAY; }
"BEGIN"                     { return (int)TokenType::KW_BEGIN; }
"BY"                        { return (int)TokenType::KW_BY; }
"CASE"                      { return (int)TokenType::KW_CASE; }
"CONST"                     { return (int)TokenType::KW_CONST; }
"DIV"                       { return (int)TokenType::KW_DIV; }
"DO"                        { return (int)TokenType::KW_DO; }
"ELSE"                      { return (int)TokenType::KW_ELSE; }
"ELSIF"                     { return (int)TokenType::KW_ELSIF; }
"END"                       { return (int)TokenType::KW_END; }
"FALSE"                     { return (int)TokenType::KW_FALSE; }
"FOR"                       { return (int)TokenType::KW_FOR; }
"IF"                        { return (int)TokenType::KW_IF; }
"IMPORT"                    { return (int)TokenType::KW_IMPORT; }
"IN"                        { return (int)TokenType::KW_IN; }
"IS"                        { return (int)TokenType::KW_IS; }
"MOD"                       { return (int)TokenType::KW_MOD; }
"MODULE"                    { return (int)TokenType::KW_MODULE; }
"NIL"                       { return (int)TokenType::KW_NIL; }
"OF"                        { return (int)TokenType::KW_OF; }
"OR"                        { return (int)TokenType::KW_OR; }
"POINTER"                   { return (int)TokenType::KW_POINTER; }
"PROCEDURE"                 { return (int)TokenType::KW_PROCEDURE; }
"RECORD"                    { return (int)TokenType::KW_RECORD; }
"REPEAT"                    { return (int)TokenType::KW_REPEAT; }
"RETURN"                    { return (int)TokenType::KW_RETURN; }
"THEN"                      { return (int)TokenType::KW_THEN; }
"TO"                        { return (int)TokenType::KW_TO; }
"TRUE"                      { return (int)TokenType::KW_TRUE; }
"TYPE"                      { return (int)TokenType::KW_TYPE; }
"UNTIL"                     { return (int)TokenType::KW_UNTIL; }
"VAR"                       { return (int)TokenType::KW_VAR; }
"WHILE"                     { return (int)TokenType::KW_WHILE; }

":="                        { return (int)TokenType::Assign; }
".."                        { return (int)TokenType::Range; }

"<="                        { return (int)TokenType::Le; }
">="                        { return (int)TokenType::Ge; }
"#"                         { return (int)TokenType::Neq; }
"="                         { return (int)TokenType::Eq; }
"<"                         { return (int)TokenType::Lt; }
">"                         { return (int)TokenType::Gt; }

"+"                         { return (int)TokenType::Plus; }
"-"                         { return (int)TokenType::Minus; }
"*"                         { return (int)TokenType::Star; }
"/"                         { return (int)TokenType::Slash; }

"^"                         { return (int)TokenType::Caret; }
"&"                         { return (int)TokenType::Amp; }
"~"                         { return (int)TokenType::Tilde; }
"|"                         { return (int)TokenType::Bar; }

"("                         { return (int)TokenType::LParen; }
")"                         { return (int)TokenType::RParen; }
"["                         { return (int)TokenType::LBrack; }
"]"                         { return (int)TokenType::RBrack; }
"{"                         { return (int)TokenType::LBrace; }
"}"                         { return (int)TokenType::RBrace; }

","                         { return (int)TokenType::Comma; }
";"                         { return (int)TokenType::Semicolon; }
":"                         { return (int)TokenType::Colon; }
"."                         { return (int)TokenType::Dot; }


{DIGIT}+\.{DIGIT}*([Ee][+-]?{DIGIT}+)?  { return (int)TokenType::Real; }

{DIGIT}{HEXDIGIT}*"H"       { return (int)TokenType::Integer; }

{DIGIT}+                    { return (int)TokenType::Integer; }

\"[^\"\n]*\"                { return (int)TokenType::String; }

{DIGIT}{HEXDIGIT}*"X"       { return (int)TokenType::String; }

{LETTER}{IDREST}*           { return (int)TokenType::Ident; }

<<EOF>>                     { return 0; }

.                           { return (int)TokenType::Unknown; }

%%
