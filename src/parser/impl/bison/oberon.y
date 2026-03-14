%code requires {
  #include <vector>
  #include <util/Token.h>
  #include <util/ast/Ast.h>
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

static void deleteExprVec(std::vector<Expr*>* v) {
  if (!v) return;
  for (auto* e : *v) delete e;
  delete v;
}
static void deleteStmtVec(std::vector<Stmt*>* v) {
  if (!v) return;
  for (auto* s : *v) delete s;
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

  Stmt* stmt;
  Expr* expr;
  DesignatorExpr* des;

  std::vector<Stmt*>* stmts;
  std::vector<Expr*>* exprs;
}

/* ===== tokens ===== */
%token <tok> TOK_IDENT TOK_INTEGER TOK_REAL TOK_STRING

%token TOK_KW_MODULE TOK_KW_BEGIN TOK_KW_END
%token TOK_KW_DIV TOK_KW_MOD TOK_KW_OR

%token TOK_PLUS TOK_MINUS TOK_STAR TOK_SLASH
%token TOK_EQ TOK_NEQ TOK_LT TOK_LE TOK_GT TOK_GE
%token TOK_ASSIGN
%token TOK_AMP TOK_TILDE
%token TOK_CARET

%token TOK_LPAREN TOK_RPAREN TOK_LBRACK TOK_RBRACK
%token TOK_COMMA TOK_SEMICOLON TOK_DOT
%token TOK_UNKNOWN

%type <module> module
%type <block>  block
%type <stmts>  stmt_list
%type <stmt>   stmt assign_stmt call_stmt
%type <expr>   expr primary
%type <exprs>  opt_args arg_list expr_list
%type <des>    designator

%destructor { delete $$; } <tok>
%destructor { delete $$; } <expr>
%destructor { delete $$; } <stmt>
%destructor { delete $$; } <des>
%destructor { delete $$; } <module>
%destructor { delete $$; } <block>
%destructor { deleteStmtVec($$); } <stmts>
%destructor { deleteExprVec($$); } <exprs>

/* ===== precedence ===== */
%left TOK_KW_OR
%left TOK_AMP
%nonassoc TOK_EQ TOK_NEQ TOK_LT TOK_LE TOK_GT TOK_GE
%left TOK_PLUS TOK_MINUS
%left TOK_STAR TOK_SLASH TOK_KW_DIV TOK_KW_MOD
%right TOK_TILDE
%right UMINUS

%%

input
  : module { ctx->module.reset($1); }
  ;

module
  : TOK_KW_MODULE TOK_IDENT TOK_SEMICOLON
    block
    TOK_KW_END TOK_IDENT TOK_DOT
    {
      auto* m = new Module();
      m->name = $2->text;
      m->endName = $6->text;

      delete $2;
      delete $6;

      if (m->name != m->endName) {
        yyerror(ctx, "module name mismatch");
        delete m;
        delete $4;
        YYERROR;
      }

      m->block.reset($4);
      $$ = m;
    }
  ;

block
  : TOK_KW_BEGIN stmt_list TOK_KW_END
    {
      auto* b = new Block();
      // порядок stmt сохранится как в исходнике
      for (auto* s : *$2) b->stmts.emplace_back(s);
      delete $2;
      $$ = b;
    }
  ;

stmt_list
  : /* empty */ { $$ = new std::vector<Stmt*>(); }
  | stmt_list stmt TOK_SEMICOLON
    {
      $1->push_back($2);
      $$ = $1;
    }
  ;

stmt
  : assign_stmt { $$ = $1; }
  | call_stmt   { $$ = $1; }
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
  ;

opt_args
  : /* empty */ { $$ = new std::vector<Expr*>(); }
  | arg_list    { $$ = $1; }
  ;

arg_list
  : expr_list { $$ = $1; }
  ;

expr_list
  : expr
    {
      $$ = new std::vector<Expr*>();
      $$->push_back($1);
    }
  | expr_list TOK_COMMA expr
    {
      $1->push_back($3);
      $$ = $1;
    }
  ;

/* ===== expressions ===== */
expr
  : expr TOK_KW_OR expr { auto* b = new BinaryExpr(); b->op=BinaryExpr::Op::Or;  b->lhs.reset($1); b->rhs.reset($3); $$=b; }
  | expr TOK_AMP  expr  { auto* b = new BinaryExpr(); b->op=BinaryExpr::Op::And; b->lhs.reset($1); b->rhs.reset($3); $$=b; }

  | expr TOK_EQ  expr   { auto* b = new BinaryExpr(); b->op=BinaryExpr::Op::Eq;  b->lhs.reset($1); b->rhs.reset($3); $$=b; }
  | expr TOK_NEQ expr   { auto* b = new BinaryExpr(); b->op=BinaryExpr::Op::Neq; b->lhs.reset($1); b->rhs.reset($3); $$=b; }
  | expr TOK_LT  expr   { auto* b = new BinaryExpr(); b->op=BinaryExpr::Op::Lt;  b->lhs.reset($1); b->rhs.reset($3); $$=b; }
  | expr TOK_LE  expr   { auto* b = new BinaryExpr(); b->op=BinaryExpr::Op::Le;  b->lhs.reset($1); b->rhs.reset($3); $$=b; }
  | expr TOK_GT  expr   { auto* b = new BinaryExpr(); b->op=BinaryExpr::Op::Gt;  b->lhs.reset($1); b->rhs.reset($3); $$=b; }
  | expr TOK_GE  expr   { auto* b = new BinaryExpr(); b->op=BinaryExpr::Op::Ge;  b->lhs.reset($1); b->rhs.reset($3); $$=b; }

  | expr TOK_PLUS  expr { auto* b = new BinaryExpr(); b->op=BinaryExpr::Op::Add; b->lhs.reset($1); b->rhs.reset($3); $$=b; }
  | expr TOK_MINUS expr { auto* b = new BinaryExpr(); b->op=BinaryExpr::Op::Sub; b->lhs.reset($1); b->rhs.reset($3); $$=b; }

  | expr TOK_STAR  expr { auto* b = new BinaryExpr(); b->op=BinaryExpr::Op::Mul; b->lhs.reset($1); b->rhs.reset($3); $$=b; }
  | expr TOK_SLASH expr { auto* b = new BinaryExpr(); b->op=BinaryExpr::Op::RDiv;b->lhs.reset($1); b->rhs.reset($3); $$=b; }
  | expr TOK_KW_DIV expr{ auto* b = new BinaryExpr(); b->op=BinaryExpr::Op::IDiv;b->lhs.reset($1); b->rhs.reset($3); $$=b; }
  | expr TOK_KW_MOD expr{ auto* b = new BinaryExpr(); b->op=BinaryExpr::Op::Mod; b->lhs.reset($1); b->rhs.reset($3); $$=b; }

  | TOK_TILDE expr      { auto* u = new UnaryExpr(); u->op=UnaryExpr::Op::Not; u->rhs.reset($2); $$=u; }
  | TOK_MINUS expr %prec UMINUS { auto* u = new UnaryExpr(); u->op=UnaryExpr::Op::Neg; u->rhs.reset($2); $$=u; }

  | primary             { $$ = $1; }
  ;

primary
  : TOK_INTEGER
    {
      auto* lit = new LiteralExpr();
      lit->kind = LiteralExpr::Kind::Int;
      lit->intValue = $1->intValue;
      delete $1;
      $$ = lit;
    }
  | TOK_IDENT
    {
      auto* d = new DesignatorExpr();
      d->baseName = $1->text;
      delete $1;
      $$ = d;
    }
  | TOK_LPAREN expr TOK_RPAREN { $$ = $2; }
  ;

/* ===== designator ===== */
designator
  : TOK_IDENT
    {
      auto* d = new DesignatorExpr();
      d->baseName = $1->text;
      delete $1;
      $$ = d;
    }
  | designator TOK_DOT TOK_IDENT
    {
      auto s = std::make_unique<FieldSelector>();
      s->name = $3->text;
      delete $3;

      $1->selectors.push_back(std::move(s));
      $$ = $1;
    }
  | designator TOK_LBRACK expr TOK_RBRACK
    {
      auto s = std::make_unique<IndexSelector>();
      s->index.reset($3);

      $1->selectors.push_back(std::move(s));
      $$ = $1;
    }
  | designator TOK_CARET
    {
      auto s = std::make_unique<DerefSelector>();
      $1->selectors.push_back(std::move(s));
      $$ = $1;
    }
  ;

%%
