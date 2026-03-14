%code requires {
  #include <vector>
  #include <string>
  #include <util/Token.h>
  #include <util/ast/Ast.h>

  // Вспомогательные структуры для парсера (чтобы прокидывать списки)
  struct IdentDef {
      std::string name;
      bool exported = false;
  };
  
  struct QualIdent {
      std::string module;
      std::string name;
  };
}

%{
#include "parser/impl/bison/ParserContext.h"
#include <cstdio>
#include <vector>

#include <util/Token.h>
#include <util/ast/Ast.h>

static void yyerror(ParserContext* ctx, const char* msg) {
  if (ctx) ctx->lastError = msg;
  std::fprintf(stderr, "parse error: %s\n", msg);
}

// Функции для очистки памяти при ошибках синтаксиса
template<typename T>
static void deleteVec(std::vector<T*>* v) {
  if (!v) return;
  for (auto* e : *v) delete e;
  delete v;
}
%}

%define api.pure full
%define parse.error verbose
%parse-param { ParserContext* ctx }
%lex-param   { ParserContext* ctx }

%code provides { int yylex(YYSTYPE* yylval, ParserContext* ctx); }

/* ===== sem values ===== */
%union {
  Token* tok;

  Module* module;
  Block*  block;

  Decl* decl;
  TypeNode* type;
  Stmt* stmt;
  Expr* expr;
  DesignatorExpr* des;

  IdentDef* identdef;
  QualIdent* qualident;

  std::vector<IdentDef>* ident_list;
  std::vector<Decl*>* decls;
  std::vector<FieldDecl*>* fields;
  std::vector<ParamDecl*>* params;
  
  std::vector<Stmt*>* stmts;
  std::vector<Expr*>* exprs;
  std::vector<CaseAlternative*>* cases;
  std::vector<CaseLabel*>* labels;
}

/* ===== tokens ===== */
%token <tok> TOK_IDENT TOK_INTEGER TOK_REAL TOK_STRING // TOK_CHAR если добавишь

%token TOK_KW_MODULE TOK_KW_BEGIN TOK_KW_END TOK_KW_IMPORT
%token TOK_KW_CONST TOK_KW_TYPE TOK_KW_VAR TOK_KW_PROCEDURE
%token TOK_KW_ARRAY TOK_KW_OF TOK_KW_RECORD TOK_KW_POINTER TOK_KW_TO
%token TOK_KW_IF TOK_KW_THEN TOK_KW_ELSIF TOK_KW_ELSE
%token TOK_KW_CASE TOK_KW_BAR TOK_KW_WHILE TOK_KW_DO
%token TOK_KW_REPEAT TOK_KW_UNTIL TOK_KW_FOR TOK_KW_BY
%token TOK_KW_RETURN TOK_KW_NIL TOK_KW_TRUE TOK_KW_FALSE
%token TOK_KW_IN TOK_KW_IS TOK_KW_DIV TOK_KW_MOD TOK_KW_OR

%token TOK_PLUS TOK_MINUS TOK_STAR TOK_SLASH
%token TOK_EQ TOK_NEQ TOK_LT TOK_LE TOK_GT TOK_GE
%token TOK_ASSIGN TOK_AMP TOK_TILDE TOK_CARET TOK_RANGE

%token TOK_LPAREN TOK_RPAREN TOK_LBRACK TOK_RBRACK TOK_LBRACE TOK_RBRACE
%token TOK_COMMA TOK_SEMICOLON TOK_COLON TOK_DOT
%token TOK_UNKNOWN

/* ===== types ===== */
%type <module>     module
%type <block>      block
%type <identdef>   identdef
%type <qualident>  qualident
%type <ident_list> ident_list

%type <decls>      import_list imports decl_seq const_decls type_decls var_decls
%type <decl>       import const_decl type_decl var_decl proc_decl
%type <params>     formal_pars fp_sections opt_fpars
%type <type>       type base_type fp_type 
%type <fields>     field_list_seq field_list

%type <stmts>      stmt_seq elsifs opt_else
%type <stmt>       stmt assign_stmt call_stmt if_stmt while_stmt repeat_stmt for_stmt case_stmt return_stmt
%type <expr>       expr simple_expr term factor element set
%type <exprs>      opt_args arg_list expr_list set_elements
%type <des>        designator

/* ===== precedence ===== */
/* Приоритеты строго по стандарту Oberon-07 */
%nonassoc TOK_EQ TOK_NEQ TOK_LT TOK_LE TOK_GT TOK_GE TOK_KW_IN TOK_KW_IS
%left TOK_PLUS TOK_MINUS TOK_KW_OR
%left TOK_STAR TOK_SLASH TOK_KW_DIV TOK_KW_MOD TOK_AMP
%right UMINUS UPLUS TOK_TILDE TOK_CARET

%%

/* ================== ROOT ================== */
input : module { ctx->module.reset($1); } ;

/* ================== MODULE ================== */
module
  : TOK_KW_MODULE TOK_IDENT TOK_SEMICOLON
    import_list
    decl_seq
    block
    TOK_IDENT TOK_DOT
    {
      auto* m = new Module();
      m->name = $2->text;
      m->endName = $7->text;
      delete $2; delete $7;

      if (m->name != m->endName) {
        yyerror(ctx, "module name mismatch");
        YYERROR;
      }

      // Добавляем импорты и декларации (const/type/var/proc)
      // m->imports = ... (реализуй наполнение)
      // m->decls = ... 
      m->block.reset($6);
      $$ = m;
    }
  ;

import_list
  : /* empty */ { $$ = new std::vector<Decl*>(); }
  | TOK_KW_IMPORT imports TOK_SEMICOLON { $$ = $2; }
  ;

imports
  : import { $$ = new std::vector<Decl*>(); $$->push_back($1); }
  | imports TOK_COMMA import { $1->push_back($3); $$ = $1; }
  ;

import
  : TOK_IDENT
    { 
       // auto* i = new Import(); i->name = $1->text; $$ = i; 
       delete $1; $$ = nullptr; // TODO
    }
  | TOK_IDENT TOK_ASSIGN TOK_IDENT
    { 
       // alias := name
       delete $1; delete $3; $$ = nullptr; // TODO
    }
  ;

/* ================== DECLARATIONS ================== */
decl_seq
  : const_decls type_decls var_decls /* TODO: ProcDecls */
    { $$ = new std::vector<Decl*>(); /* Склеить все */ }
  ;

const_decls
  : /* empty */ { $$ = new std::vector<Decl*>(); }
  | TOK_KW_CONST const_decl_list { $$ = nullptr; }
  ;
const_decl_list
  : const_decl TOK_SEMICOLON
  | const_decl_list const_decl TOK_SEMICOLON
  ;
const_decl : identdef TOK_EQ expr { $$ = nullptr; /* TODO: ConstDecl */ } ;

type_decls
  : /* empty */ { $$ = new std::vector<Decl*>(); }
  | TOK_KW_TYPE type_decl_list { $$ = nullptr; }
  ;
type_decl_list
  : type_decl TOK_SEMICOLON
  | type_decl_list type_decl TOK_SEMICOLON
  ;
type_decl : identdef TOK_EQ type { $$ = nullptr; /* TODO: TypeDecl */ } ;

var_decls
  : /* empty */ { $$ = new std::vector<Decl*>(); }
  | TOK_KW_VAR var_decl_list { $$ = nullptr; }
  ;
var_decl_list
  : var_decl TOK_SEMICOLON
  | var_decl_list var_decl TOK_SEMICOLON
  ;
var_decl
  : ident_list TOK_COLON type 
    { $$ = nullptr; /* TODO: VarDecl со списком имен */ }
  ;

/* ================== TYPES ================== */
type
  : qualident { $$ = nullptr; /* NamedType */ }
  | TOK_KW_ARRAY expr_list TOK_KW_OF type { $$ = nullptr; /* ArrayType */ }
  | TOK_KW_RECORD base_type field_list_seq TOK_KW_END { $$ = nullptr; /* RecordType */ }
  | TOK_KW_POINTER TOK_KW_TO type { $$ = nullptr; /* PointerType */ }
  | TOK_KW_PROCEDURE opt_fpars { $$ = nullptr; /* ProcType */ }
  ;

base_type
  : /* empty */ { $$ = nullptr; }
  | TOK_LPAREN qualident TOK_RPAREN { $$ = nullptr; /* Наследование */ }
  ;

field_list_seq
  : /* empty */ { $$ = new std::vector<FieldDecl*>(); }
  | field_list
  | field_list_seq TOK_SEMICOLON field_list
  ;

field_list
  : /* empty */ { $$ = new std::vector<FieldDecl*>(); }
  | ident_list TOK_COLON type { $$ = nullptr; }
  ;

opt_fpars
  : /* empty */ { $$ = new std::vector<ParamDecl*>(); }
  | formal_pars { $$ = $1; }
  ;

formal_pars
  : TOK_LPAREN fp_sections TOK_RPAREN { $$ = $2; }
  | TOK_LPAREN fp_sections TOK_RPAREN TOK_COLON qualident { $$ = $2; /* + return type */ }
  ;

fp_sections
  : /* empty */ { $$ = new std::vector<ParamDecl*>(); }
  | ident_list TOK_COLON fp_type { $$ = nullptr; }
  | TOK_KW_VAR ident_list TOK_COLON fp_type { $$ = nullptr; }
  | fp_sections TOK_SEMICOLON ident_list TOK_COLON fp_type { $$ = nullptr; }
  | fp_sections TOK_SEMICOLON TOK_KW_VAR ident_list TOK_COLON fp_type { $$ = nullptr; }
  ;

fp_type
  : qualident { $$ = nullptr; }
  | TOK_KW_ARRAY TOK_KW_OF fp_type { $$ = nullptr; }
  ;

/* ================== BLOCKS & STATEMENTS ================== */
block
  : /* empty */ { $$ = new Block(); }
  | TOK_KW_BEGIN stmt_seq TOK_KW_END
    {
      auto* b = new Block();
      for (auto* s : *$2) b->stmts.emplace_back(s);
      delete $2;
      $$ = b;
    }
  ;

stmt_seq
  : stmt 
    { 
      $$ = new std::vector<Stmt*>();
      if ($1) $$->push_back($1); 
    }
  | stmt_seq TOK_SEMICOLON stmt 
    { 
      if ($3) $1->push_back($3); 
      $$ = $1; 
    }
  ;

stmt
  : /* empty */   { $$ = nullptr; }
  | assign_stmt   { $$ = $1; }
  | call_stmt     { $$ = $1; }
  | if_stmt       { $$ = $1; }
  | while_stmt    { $$ = $1; }
  | repeat_stmt   { $$ = $1; }
  | for_stmt      { $$ = $1; }
  | return_stmt   { $$ = $1; }
  | case_stmt     { $$ = nullptr; /* TODO */ }
  ;

assign_stmt
  : designator TOK_ASSIGN expr
    {
      auto* s = new AssignStmt();
      s->lhs.reset($1);
      s->rhs.reset($3);
      $$ = s;
    }
  ;

call_stmt
  : designator TOK_LPAREN opt_args TOK_RPAREN
    {
      auto* c = new CallExpr();
      c->callee.reset($1);
      for (auto* e : *$3) c->args.emplace_back(e);
      delete $3;

      auto* s = new CallStmt();
      s->call.reset(c);
      $$ = s;
    }
  | designator /* Процедура без параметров в Oberon-07 вызывается без () */
    {
      auto* c = new CallExpr();
      c->callee.reset($1);
      auto* s = new CallStmt();
      s->call.reset(c);
      $$ = s;
    }
  ;

if_stmt
  : TOK_KW_IF expr TOK_KW_THEN stmt_seq elsifs opt_else TOK_KW_END
    {
      auto* s = new IfStmt();
      s->cond.reset($2);
      // s->thenBlock = $4; s->elsifs = $5; s->elseBlock = $6; 
      $$ = s;
    }
  ;

elsifs
  : /* empty */ { $$ = new std::vector<Stmt*>(); }
  | elsifs TOK_KW_ELSIF expr TOK_KW_THEN stmt_seq { $$ = $1; /* добавить elsif */ }
  ;

opt_else
  : /* empty */ { $$ = nullptr; }
  | TOK_KW_ELSE stmt_seq { $$ = $2; }
  ;

while_stmt
  : TOK_KW_WHILE expr TOK_KW_DO stmt_seq elsifs TOK_KW_END
    {
      auto* s = new WhileStmt();
      s->cond.reset($2);
      // s->body = $4;
      $$ = s;
    }
  ;

repeat_stmt
  : TOK_KW_REPEAT stmt_seq TOK_KW_UNTIL expr
    {
      auto* s = new RepeatStmt();
      // s->body = $2; s->cond = $4;
      $$ = s;
    }
  ;

for_stmt
  : TOK_KW_FOR TOK_IDENT TOK_ASSIGN expr TOK_KW_TO expr TOK_KW_DO stmt_seq TOK_KW_END
    { $$ = new ForStmt(); /* Без BY */ }
  | TOK_KW_FOR TOK_IDENT TOK_ASSIGN expr TOK_KW_TO expr TOK_KW_BY expr TOK_KW_DO stmt_seq TOK_KW_END
    { $$ = new ForStmt(); /* С BY */ }
  ;

case_stmt
  : TOK_KW_CASE expr TOK_KW_OF case_list opt_else TOK_KW_END { $$ = nullptr; }
  ;

case_list
  : /*...*/ { } ;

return_stmt
  : TOK_KW_RETURN expr
    {
      auto* s = new ReturnStmt();
      // s->expr = $2;
      $$ = s;
    }
  | TOK_KW_RETURN { $$ = new ReturnStmt(); }
  ;

/* ================== EXPRESSIONS ================== */
expr
  : simple_expr { $$ = $1; }
  | simple_expr TOK_EQ  simple_expr { $$ = new BinaryExpr(); /* Eq */ }
  | simple_expr TOK_NEQ simple_expr { $$ = new BinaryExpr(); /* Neq */ }
  | simple_expr TOK_LT  simple_expr { $$ = new BinaryExpr(); /* Lt */ }
  | simple_expr TOK_LE  simple_expr { $$ = new BinaryExpr(); /* Le */ }
  | simple_expr TOK_GT  simple_expr { $$ = new BinaryExpr(); /* Gt */ }
  | simple_expr TOK_GE  simple_expr { $$ = new BinaryExpr(); /* Ge */ }
  | simple_expr TOK_KW_IN simple_expr { $$ = new InExpr(); }
  | simple_expr TOK_KW_IS qualident { $$ = new IsExpr(); }
  ;

simple_expr
  : term { $$ = $1; }
  | TOK_PLUS term %prec UPLUS { $$ = $2; /* Унарный плюс (пустышка) */ }
  | TOK_MINUS term %prec UMINUS { auto* u = new UnaryExpr(); u->op = UnaryExpr::Op::Neg; u->rhs.reset($2); $$ = u; }
  | simple_expr TOK_PLUS term { $$ = new BinaryExpr(); /* Add */ }
  | simple_expr TOK_MINUS term { $$ = new BinaryExpr(); /* Sub */ }
  | simple_expr TOK_KW_OR term { $$ = new BinaryExpr(); /* Or */ }
  ;

term
  : factor { $$ = $1; }
  | term TOK_STAR factor { $$ = new BinaryExpr(); /* Mul */ }
  | term TOK_SLASH factor { $$ = new BinaryExpr(); /* RDiv */ }
  | term TOK_KW_DIV factor { $$ = new BinaryExpr(); /* IDiv */ }
  | term TOK_KW_MOD factor { $$ = new BinaryExpr(); /* Mod */ }
  | term TOK_AMP factor { $$ = new BinaryExpr(); /* And */ }
  ;

factor
  : TOK_INTEGER 
    { 
       auto* lit = new LiteralExpr(); lit->kind = LiteralExpr::Kind::Int;
       lit->intValue = $1->intValue; delete $1; $$ = lit; 
    }
  | TOK_REAL    { $$ = new LiteralExpr(); /* Реализуй RealLiteral */ delete $1; }
  | TOK_STRING  { $$ = new LiteralExpr(); /* Реализуй StringLiteral */ delete $1; }
  | TOK_KW_NIL  { $$ = new LiteralExpr(); /* NilLiteral */ }
  | TOK_KW_TRUE { $$ = new LiteralExpr(); /* Bool(true) */ }
  | TOK_KW_FALSE{ $$ = new LiteralExpr(); /* Bool(false) */ }
  | set         { $$ = $1; }
  | designator  { $$ = $1; } /* Вызов ф-ии или переменная */
  | designator TOK_LPAREN opt_args TOK_RPAREN { $$ = new CallExpr(); /* Вызов ф-ии в выражении */ }
  | TOK_LPAREN expr TOK_RPAREN { $$ = $2; }
  | TOK_TILDE factor { auto* u = new UnaryExpr(); u->op = UnaryExpr::Op::Not; u->rhs.reset($2); $$ = u; }
  ;

/* ================== SETS ================== */
set : TOK_LBRACE set_elements TOK_RBRACE { $$ = nullptr; /* SetExpr */ } ;

set_elements
  : /* empty */ { $$ = new std::vector<Expr*>(); }
  | element { $$ = new std::vector<Expr*>(); $$->push_back($1); }
  | set_elements TOK_COMMA element { $1->push_back($3); $$ = $1; }
  ;

element
  : expr { $$ = $1; }
  | expr TOK_RANGE expr { $$ = nullptr; /* SetRangeExpr 1..5 */ }
  ;

/* ================== DESIGNATOR ================== */
designator
  : qualident
    {
      auto* d = new DesignatorExpr();
      d->baseName = $1->name; /* Поддержка module.ident делается тут */
      delete $1;
      $$ = d;
    }
  | designator TOK_DOT TOK_IDENT
    {
      auto s = std::make_unique<FieldSelector>();
      s->name = $3->text; delete $3;
      $1->selectors.push_back(std::move(s)); $$ = $1;
    }
  | designator TOK_LBRACK expr_list TOK_RBRACK
    {
      for(auto* e : *$3) {
         auto s = std::make_unique<IndexSelector>(); s->index.reset(e);
         $1->selectors.push_back(std::move(s));
      }
      delete $3; $$ = $1;
    }
  | designator TOK_CARET
    {
      $1->selectors.push_back(std::make_unique<DerefSelector>());
      $$ = $1;
    }
  /* Type Guard: p(NodeType) парсится как селектор в Обероне! */
  | designator TOK_LPAREN qualident TOK_RPAREN { $$ = $1; /* Добавь TypeGuardSelector */ }
  ;

/* ================== IDENTIFIERS & LISTS ================== */
identdef
  : TOK_IDENT { $$ = new IdentDef{$1->text, false}; delete $1; }
  | TOK_IDENT TOK_STAR { $$ = new IdentDef{$1->text, true}; delete $1; }
  ;

qualident
  : TOK_IDENT { $$ = new QualIdent{"", $1->text}; delete $1; }
  | TOK_IDENT TOK_DOT TOK_IDENT { $$ = new QualIdent{$1->text, $3->text}; delete $1; delete $3; }
  ;

ident_list
  : identdef { $$ = new std::vector<IdentDef>(); $$->push_back(*$1); delete $1; }
  | ident_list TOK_COMMA identdef { $1->push_back(*$3); delete $3; $$ = $1; }
  ;

opt_args
  : /* empty */ { $$ = new std::vector<Expr*>(); }
  | expr_list   { $$ = $1; }
  ;

expr_list
  : expr { $$ = new std::vector<Expr*>(); $$->push_back($1); }
  | expr_list TOK_COMMA expr { $1->push_back($3); $$ = $1; }
  ;

%%
