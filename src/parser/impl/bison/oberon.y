%code requires {
  #include <vector>
  #include <string>
  #include <util/Token.h>
  #include <util/ast/Ast.h>

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
  if (ctx) {
    ctx->lastError = msg;
    Token t = ctx->tz->peek();
    std::fprintf(stderr, "Parse error at %d:%d: %s. Found token: '%s'\n",
        t.line, t.col, msg, t.text.c_str());
  } else {
    std::fprintf(stderr, "Parse error: %s\n", msg);
  }
}

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

%union {
  Token* tok;

  LiteralExpr* literalExpr;
  
  IdentDef* identDef;
  QualIdent* qualIdent;
  
  ConstDecl* constDecl;
  
  Expr* expr;
  TypeDecl* typeDecl;
  TypeNode* type;
  std::vector<Expr*>* expr_list;
  
  std::vector<IdentDef*>* identDef_list;
  std::vector<FieldDecl*>* fieldDecl_list;
  
  ProcType* procType;
  ProcParams* procParams;
  std::vector<ProcParams*>* procParams_list;
  
  std::vector<std::string>* strings;
  
  VarDecl* varDecl;
  
  std::vector<Decl*>* decls_list;
  
  BinaryExpr::Op op;
  std::vector<SetElement>* setElement_list;
  SetElement* setElement;
  Selector* selector;
  std::vector<Selector*>* selector_list;
  
  DesignatorExpr* designatorExpr;
  
  Stmt* stmt;
  std::vector<Stmt*>* stmt_list;
  
  std::vector<Branch>* elif;
  
  CaseLabel* caseLabel;
  std::vector<CaseLabel*>* caseLabel_list;
  CaseAlternative* caseAlternative;
  std::vector<CaseAlternative*>* caseAlternative_list;

  ProcDecl* procDecl;
  
  Module* module;
  Import* import;
  std::vector<Import>* imports;
  FieldDecl* fieldDecl;
}

%destructor { delete $$; } <tok> <literalExpr> <identDef> <qualIdent>
%destructor { delete $$; } <constDecl> <expr> <typeDecl> <type> <varDecl>
%destructor { delete $$; } <procParams>
%destructor { delete $$; } <setElement> <selector> <designatorExpr>
%destructor { delete $$; } <stmt> <caseLabel> <caseAlternative> <procDecl>
%destructor { delete $$; } <module> <import> <fieldDecl>
%destructor { delete $$; } <strings> <elif> <imports> <setElement_list>
%destructor { deleteVec($$); } <expr_list> <identDef_list> <fieldDecl_list>
%destructor { deleteVec($$); } <procParams_list> <selector_list> <stmt_list>
%destructor { deleteVec($$); } <caseLabel_list> <caseAlternative_list> <decls_list>

%token <tok> TOK_IDENT TOK_INTEGER TOK_REAL TOK_STRING 

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

%type <literalExpr> number
%type <qualIdent> qualident FormalParameterROpt
%type <identDef> identdef
%type <constDecl> ConstDeclaration
%type <expr> expression ConstExpression SimpleExpression factor label term set
%type <typeDecl> TypeDeclaration
%type <type> type ArrayType RecordType BaseType BaseTypeOpt PointerType ProcedureType FormalParametersOpt FormalParameters FormalType
%type <expr_list> exprList
%type <identDef_list> IdentList
%type <fieldDecl_list> FieldListSequence FieldListSequenceOpt
%type <procParams> FPSection
%type <procParams_list> FPSectionList FormalParametersLOpt
%type <strings> identList
%type <varDecl> VariableDeclaration
%type <op> relation AddOperator MulOperator
%type <setElement> element
%type <setElement_list> elementOpt elements
%type <selector> selector
%type <selector_list> selectorList selectorOpt
%type <designatorExpr> designator
%type <stmt> statement WhileStatement IfStatement RepeatStatement ForStatement CaseStatement ReturnStatement
%type <stmt_list> StatementSequence
%type <elif> ElifOpt ElsifDo
%type <caseLabel> LabelRange
%type <caseLabel_list> CaseLabelList
%type <caseAlternative> case
%type <caseAlternative_list> caseList
%type <procDecl> ProcedureDeclaration ProcedureHeading ProcedureBody
%type <decls_list> DeclarationSequence ConstDeclarationOpt ConstDeclarationList TypeDeclarationOpt TypeDeclarationList VariableDeclarationOpt VariableDeclarationList ProcedureDeclarationList
%type <module> module
%type <import> import
%type <imports> imports ImportList
%type <fieldDecl> FieldList

%nonassoc TOK_EQ TOK_NEQ TOK_LT TOK_LE TOK_GT TOK_GE TOK_KW_IN TOK_KW_IS
%left TOK_PLUS TOK_MINUS TOK_KW_OR
%left TOK_STAR TOK_SLASH TOK_KW_DIV TOK_KW_MOD TOK_AMP
%right UMINUS UPLUS TOK_TILDE TOK_CARET

%%

input : module { ctx->module.reset($1); } ;

/* number = integer | real. */
number : TOK_INTEGER {
	auto* lit = new LiteralExpr();
	lit->kind = LiteralExpr::Kind::Int;
	lit->intValue = $1->intValue;
	delete $1;
	$$ = lit;
} | TOK_REAL {
	auto* lit = new LiteralExpr();
	lit->kind = LiteralExpr::Kind::Real;
	lit->realValue = $1->realValue;
	delete $1;
	$$ = lit;
} ;

/* qualident = [ident "."] ident. */
qualident: TOK_IDENT {
	$$ = new QualIdent{"", $1->text};
	delete $1;
} | TOK_IDENT TOK_DOT TOK_IDENT {
	$$ = new QualIdent{$1->text, $3->text};
	delete $1;
	delete $3;
};

/* identdef = ident ["*"]. */
identdef: TOK_IDENT {
	$$ = new IdentDef{$1->text, false};
	delete $1;
} | TOK_IDENT TOK_STAR {
	$$ = new IdentDef{$1->text, true};
	delete $1;
};

/* ConstDeclaration = identdef "=" ConstExpression. */
ConstDeclaration : identdef TOK_EQ ConstExpression {
	auto* decl = new ConstDecl();
	decl->name = $1->name;
	decl->exported = $1->exported;
	decl->value.reset($3);
	delete $1;
	$$ = decl;
};

/* ConstExpression = expression. */
ConstExpression: expression {
	$$ = $1;
};

/* TypeDeclaration = identdef "=" type. */
TypeDeclaration: identdef TOK_EQ type {
	auto* typeDecl = new TypeDecl();
	typeDecl->name = $1->name;
	typeDecl->exported = $1->exported;
	typeDecl->type.reset($3);
	delete $1;
	$$ = typeDecl;
};

/* type = qualident | ArrayType | RecordType | PointerType | ProcedureType. */
type : qualident {
	auto* named = new NamedType();
	named->name = $1->module.empty() ? $1->name : ($1->module + "." + $1->name);
	delete $1;
	$$ = named;
}| ArrayType {
	$$ = $1;
}| RecordType {
	$$ = $1;
}| PointerType {
	$$ = $1;
}| ProcedureType {
	$$ = $1;
};

/* ArrayType = ARRAY length {"," length} OF type. */
ArrayType: TOK_KW_ARRAY exprList TOK_KW_OF type {
	auto* t = new ArrayType();
	t->elemType.reset($4);
	for (auto* e : *$2) {
		t->length.push_back(Ptr<Expr>(e));
	}
	delete $2;
	$$ = t;
};

exprList
  : expression {
	$$ = new std::vector<Expr*>();
	$$->push_back($1);
} | exprList TOK_COMMA expression {
	$1->push_back($3);
	$$ = $1;
};

/* RecordType = RECORD ["(" BaseType ")"] [FieldListSequence] END. */
RecordType: TOK_KW_RECORD BaseTypeOpt FieldListSequenceOpt TOK_KW_END {
	auto* t = new RecordType();
	t->baseType.reset($2);
	if ($3) {
		for (auto* e : *$3) {
			t->fields.push_back(Ptr<FieldDecl>(e));
		}
		delete $3;
	}
	$$ = t;
};

BaseTypeOpt: %empty {
	$$ = nullptr;
} | TOK_LPAREN BaseType TOK_RPAREN {
	$$ = $2;
};

BaseType: qualident {
	auto* named = new NamedType();
	named->name = $1->module.empty() ? $1->name : ($1->module + "." + $1->name);
	delete $1;
	$$ = named;
};

FieldListSequenceOpt: %empty {
	$$ = nullptr;
} | FieldListSequence {
	$$ = $1;
};

/* FieldListSequence = FieldList {";" FieldList}. */
FieldListSequence: FieldList {
	auto* result = new std::vector<FieldDecl*>();
	result->push_back($1);
	$$ = result;
} | FieldListSequence TOK_SEMICOLON FieldList {
	$1->push_back($3);
	$$ = $1;
};

/* FieldList = IdentList ":" type. */
FieldList: IdentList TOK_COLON type {
	auto* result = new FieldDecl();
	for (auto* el : *$1){
		result->names.push_back(el->name);
		delete el;
	}
	delete $1;
	result->type.reset($3);
	$$ = result;
}

/* IdentList = identdef {"," identdef}. */
IdentList: identdef {
	$$ = new std::vector<IdentDef*>;
	$$->push_back($1);
} | IdentList TOK_COMMA identdef {
	$1->push_back($3);
	$$ = $1;
};

/* PointerType = POINTER TO type. */
PointerType: TOK_KW_POINTER TOK_KW_TO type {
	auto* t = new PointerType();
	t->baseType.reset($3);
	$$ = t;
};

/* ProcedureType = PROCEDURE [FormalParameters]. */
ProcedureType : TOK_KW_PROCEDURE FormalParametersOpt {
	$$ = $2;
};

FormalParametersOpt: %empty {
	$$ = new ProcType();
} | FormalParameters {
	$$ = $1;
};

/* FormalParameters = "(" [FPSection {";" FPSection}] ")" [":" qualident]. */
FormalParameters: TOK_LPAREN FormalParametersLOpt TOK_RPAREN FormalParameterROpt {
	auto* params = new ProcType();
	if ($4) {
		params->type = std::make_unique<NamedType>();
		params->type->name = $4->module.empty() ? $4->name : ($4->module + "." + $4->name);
		delete $4;
	}
	if ($2) {
		for (auto* el : *$2) {
			params->params.push_back(Ptr<ProcParams>(el));
		}
		delete $2;
	}
	$$ = params;
}

FormalParameterROpt: %empty {
	$$ = nullptr;
} | TOK_COLON qualident {
	$$ = $2;
}

FormalParametersLOpt: %empty {
	$$ = nullptr;
} | FPSectionList {
	$$ = $1;
};

FPSectionList: FPSection {
	auto* data = new std::vector<ProcParams*>();
	data->push_back($1);
	$$ = data;
} | FPSectionList TOK_SEMICOLON FPSection {
	$1->push_back($3);
	$$ = $1;
}

/* FPSection = [VAR] ident {"," ident} ":" FormalType. */
FPSection: TOK_KW_VAR identList TOK_COLON FormalType {
	auto* data = new ProcParams();
	data->isVar = true;
	for (auto& el: *$2){
		data->names.push_back(el);
	}
	data->type.reset($4);
	delete $2;
	$$ = data;
} | identList TOK_COLON FormalType {
	auto* data = new ProcParams();
	data->isVar = false;
	for (auto& el: *$1){
		data->names.push_back(el);
	}
	data->type.reset($3);
	delete $1;
	$$ = data;
};

identList: TOK_IDENT {
	auto* data = new std::vector<std::string>();
	data->push_back($1->text);
	delete $1;
	$$ = data;
} | identList TOK_COMMA TOK_IDENT {
	$1->push_back($3->text);
	delete $3;
	$$ = $1;
};

/* FormalType = {ARRAY OF} qualident. */
FormalType: qualident {
	auto* named = new NamedType();
	named->name = $1->module.empty() ? $1->name : ($1->module + "." + $1->name);
	delete $1;
	$$ = named;
} | TOK_KW_ARRAY TOK_KW_OF FormalType {
	auto* arr = new ArrayType();
	arr->elemType.reset($3);
	$$ = arr;
}

/* VariableDeclaration = IdentList ":" type. */
VariableDeclaration : IdentList TOK_COLON type {
	auto* decl = new VarDecl();
	decl->type.reset($3);
	for (auto* el : *$1) {
		decl->names.push_back(el->name);
		decl->exportedFlags.push_back(el->exported);
		delete el;
	}
	delete $1;
	$$ = decl;
};

/* expression = SimpleExpression [relation SimpleExpression]. */
expression: SimpleExpression {
	$$ = $1;
} | SimpleExpression relation SimpleExpression {
	auto* expr = new BinaryExpr();
	expr->lhs.reset($1);
	expr->rhs.reset($3);
	expr->op = $2;
	$$ = expr;
}

relation: TOK_EQ {
	$$ = BinaryExpr::Op::Eq;
}| TOK_NEQ {
	$$ = BinaryExpr::Op::Neq;
}| TOK_LT {
	$$ = BinaryExpr::Op::Lt;
}| TOK_LE {
	$$ = BinaryExpr::Op::Le;
}| TOK_GT {
	$$ = BinaryExpr::Op::Gt;
}| TOK_GE {
	$$ = BinaryExpr::Op::Ge;
}| TOK_KW_IN {
	$$ = BinaryExpr::Op::In;
}| TOK_KW_IS {
	$$ = BinaryExpr::Op::Is;
};

/* SimpleExpression = ["+" | "-"] term {AddOperator term}. */
SimpleExpression: term {
	$$ = $1;
} | TOK_PLUS term %prec UPLUS {
	$$ = $2;
} | TOK_MINUS term %prec UMINUS {
	auto* expr = new UnaryExpr();
	expr->rhs.reset($2);
	expr->op = UnaryExpr::Op::Neg;
	$$ = expr;
} | SimpleExpression AddOperator term {
	auto* op = new BinaryExpr();
	op->lhs.reset($1);
	op->rhs.reset($3);
	op->op = $2;
	$$ = op;
}

AddOperator: TOK_PLUS {
	$$ = BinaryExpr::Op::Add;
} | TOK_MINUS {
	$$ = BinaryExpr::Op::Sub;
} | TOK_KW_OR {
	$$ = BinaryExpr::Op::Or;
};

/* term = factor {MulOperator factor}. */
term: factor {
	$$ = $1;
} | term MulOperator factor {
	auto* expr = new BinaryExpr();
	expr->lhs.reset($1);
	expr->rhs.reset($3);
	expr->op = $2;
	$$ = expr;
};

MulOperator: TOK_STAR {
	$$ = BinaryExpr::Op::Mul;
} | TOK_SLASH {
	$$ = BinaryExpr::Op::RDiv;
} | TOK_AMP {
	$$ = BinaryExpr::Op::And;
} | TOK_KW_DIV {
	$$ = BinaryExpr::Op::IDiv;
} | TOK_KW_MOD {
	$$ = BinaryExpr::Op::Mod;
};

/* factor = number | string | NIL | TRUE | FALSE | set | designator | "(" expression ")" | "~" factor */
factor: number {
	$$ = $1;
} | TOK_STRING {
	auto* val = new LiteralExpr();
	val->kind = LiteralExpr::Kind::String;
	val->strValue = $1->text;
	delete $1;
	$$ = val;
} | TOK_KW_NIL {
	auto* val = new LiteralExpr();
	val->kind = LiteralExpr::Kind::Nil;
	$$ = val;
} | TOK_KW_TRUE {
	auto* val = new LiteralExpr();
	val->kind = LiteralExpr::Kind::Bool;
	val->boolValue = true;
	$$ = val;
} | TOK_KW_FALSE {
	auto* val = new LiteralExpr();
	val->kind = LiteralExpr::Kind::Bool;
	val->boolValue = false;
	$$ = val;
} | set {
	$$ = $1;
} | designator {
	$$ = $1;
} | TOK_LPAREN expression TOK_RPAREN {
	$$ = $2;
} | TOK_TILDE factor {
	auto* result = new UnaryExpr();
	result->rhs.reset($2);
	result->op = UnaryExpr::Op::Not;
	$$ = result;
}

/* designator = qualident {selector}. */
/* Selectors now include argument lists (calls / type guards). */
designator: qualident selectorOpt {
	auto* result = new DesignatorExpr();
	result->baseName = $1->module.empty() ? $1->name : ($1->module + "." + $1->name);
	delete $1;
	for (auto* el : *$2) {
		result->selectors.push_back(Ptr<Selector>(el));
	}
	delete $2;
	$$ = result;
};

selectorOpt: %empty {
	$$ = new std::vector<Selector*>;
} | selectorList {
	$$ = $1;
}

selectorList: selector {
	auto* list = new std::vector<Selector*>;
	list->push_back($1);
	$$ = list;
} | selectorList selector {
	$1->push_back($2);
	$$ = $1;
};

/* selector = "." ident | "[" ExpList "]" | "^" | "(" ExpList ")" | "(" ")". */
/* "(" ... ")" covers both ActualParameters and TypeGuard — resolved in semantic analysis. */
selector: TOK_DOT TOK_IDENT {
	auto* s = new FieldSelector();
	s->name = $2->text;
	delete $2;
	$$ = s;
} | TOK_LBRACK exprList TOK_RBRACK {
	auto* s = new IndexSelector();
	for (auto* exp : *$2) { s->index.push_back(Ptr<Expr>(exp)); }
	delete $2;
	$$ = s;
} | TOK_CARET {
	$$ = new DerefSelector();
} | TOK_LPAREN exprList TOK_RPAREN {
	auto* s = new ArgsSelector();
	for (auto* e : *$2) {
		s->args.push_back(Ptr<Expr>(e));
	}
	delete $2;
	$$ = s;
} | TOK_LPAREN TOK_RPAREN {
	$$ = new ArgsSelector();
};

/* set = "{" [element {"," element}] "}". */
set: TOK_LBRACE elementOpt TOK_RBRACE {
	auto* s = new SetExpr();
	s->elements = std::move(*$2);
	delete $2;
	$$ = s;
};

elementOpt: %empty {
	$$ = new std::vector<SetElement>();
} | elements {
	$$ = $1;
};

elements: element {
	auto* el = new std::vector<SetElement>();
	el->push_back(std::move(*$1));
	delete $1;
	$$ = el;
} | elements TOK_COMMA element {
	$1->push_back(std::move(*$3));
	delete $3;
	$$ = $1;
};

/* element = expression [".." expression]. */
element: expression {
	auto* setElem = new SetElement();
	setElem->low.reset($1);
	$$ = setElem;
} | expression TOK_RANGE expression {
	auto* setElem = new SetElement();
	setElem->low.reset($1);
	setElem->high.reset($3);
	$$ = setElem;
};

/* statement = [assignment | ProcedureCall | IfStatement | CaseStatement | WhileStatement | RepeatStatement | ForStatement]. */
/* assignment and ProcedureCall are merged: both start with designator. */
statement: %empty {
	$$ = nullptr;
} | designator TOK_ASSIGN expression {
	auto* result = new AssignStmt();
	result->lhs.reset($1);
	result->rhs.reset($3);
	$$ = result;
} | designator {
	auto* stm = new CallStmt();
	stm->designator.reset($1);
	$$ = stm;
} | IfStatement {
	$$ = $1;
} | CaseStatement {
	$$ = $1;
} | WhileStatement {
	$$ = $1;
} | RepeatStatement {
	$$ = $1;
} | ForStatement {
	$$ = $1;
} | ReturnStatement {
	$$ = $1;
};

ReturnStatement: TOK_KW_RETURN expression {
	auto* result = new ReturnStmt();
	result->value.reset($2);
	$$ = result;
} | TOK_KW_RETURN {
	$$ = new ReturnStmt();
};

/* StatementSequence = statement {";" statement}. */
StatementSequence: statement {
	auto* vec = new std::vector<Stmt*>();
	if ($1) vec->push_back($1);
	$$ = vec;
} | StatementSequence TOK_SEMICOLON statement {
	if ($3) $1->push_back($3);
	$$ = $1;
}

/* IfStatement = IF expression THEN StatementSequence {ELSIF expression THEN StatementSequence} [ELSE StatementSequence] END. */
IfStatement: TOK_KW_IF expression TOK_KW_THEN StatementSequence ElifOpt TOK_KW_ELSE StatementSequence TOK_KW_END {
	auto* stmt = new IfStmt();
	Branch branch;
	branch.cond = Ptr<Expr>($2);
	for (auto* el : *$4){
		branch.body.push_back(Ptr<Stmt>(el));
	}
	stmt->branches.push_back(std::move(branch));
	if ($5) {
		for (auto& b : *$5) {
			stmt->branches.push_back(std::move(b));
		}
		delete $5;
	}
	for (auto* el : *$7){
		stmt->elseBody.push_back(Ptr<Stmt>(el));
	}
	delete $4;
	delete $7;
	$$ = stmt;
} | TOK_KW_IF expression TOK_KW_THEN StatementSequence ElifOpt TOK_KW_END {
	auto* stmt = new IfStmt();
	Branch branch;
	branch.cond = Ptr<Expr>($2);
	for (auto* el : *$4){
		branch.body.push_back(Ptr<Stmt>(el));
	}
	stmt->branches.push_back(std::move(branch));
	if ($5) {
		for (auto& b : *$5) {
			stmt->branches.push_back(std::move(b));
		}
		delete $5;
	}
	delete $4;
	$$ = stmt;
};

ElifOpt: %empty {
	$$ = nullptr;
} | ElifOpt TOK_KW_ELSIF expression TOK_KW_THEN StatementSequence {
	if (!$1) $1 = new std::vector<Branch>();
	Branch branch;
	branch.cond = Ptr<Expr>($3);
	for (auto* el : *$5){
		branch.body.push_back(Ptr<Stmt>(el));
	}
	delete $5;
	$1->push_back(std::move(branch));
	$$ = $1;
};

/* CaseStatement = CASE expression OF case {"|" case} END. */
CaseStatement: TOK_KW_CASE expression TOK_KW_OF caseList TOK_KW_END {
	auto* res = new CaseStmt();
	res->expr.reset($2);
	for (auto* el : *$4){
		res->alts.push_back(Ptr<CaseAlternative>(el));
	}
	delete $4;
	$$ = res;
};

caseList: case {
	auto* v = new std::vector<CaseAlternative*>();
	if ($1) v->push_back($1);
	$$ = v;
} | caseList TOK_KW_BAR case {
	if ($3) $1->push_back($3);
	$$ = $1;
};

/* case = [CaseLabelList ":" StatementSequence]. */
case: %empty {
	$$ = nullptr;
} | CaseLabelList TOK_COLON StatementSequence {
	auto* res = new CaseAlternative();
	for (auto* el : *$1){
		res->labels.push_back(Ptr<CaseLabel>(el));
	}
	for (auto* el : *$3){
		res->body.push_back(Ptr<Stmt>(el));
	}
	delete $1;
	delete $3;
	$$ = res;
};

/* CaseLabelList = LabelRange {"," LabelRange}. */
CaseLabelList: LabelRange {
	auto* vec = new std::vector<CaseLabel*>();
	vec->push_back($1);
	$$ = vec;
} | CaseLabelList TOK_COMMA LabelRange {
	$1->push_back($3);
	$$ = $1;
};

/* LabelRange = label [".." label]. */
LabelRange: label {
	auto* result = new CaseLabel();
	result->from.reset($1);
	$$ = result;
} | label TOK_RANGE label {
	auto* result = new CaseLabel();
	result->from.reset($1);
	result->to.reset($3);
	$$ = result;
};

/* label = integer | string | qualident. */
label : TOK_INTEGER {
	auto* lit = new LiteralExpr();
	lit->kind = LiteralExpr::Kind::Int;
	lit->intValue = $1->intValue;
	delete $1;
	$$ = lit;
} | TOK_STRING {
	auto* lit = new LiteralExpr();
	lit->kind = LiteralExpr::Kind::String;
	lit->strValue = $1->text;
	delete $1;
	$$ = lit;
} | qualident {
	auto* des = new DesignatorExpr;
	des->baseName = $1->module.empty() ? $1->name : ($1->module + "." + $1->name);
	delete $1;
	$$ = des;
};

/* WhileStatement = WHILE expression DO StatementSequence {ELSIF expression DO StatementSequence} END. */
WhileStatement : TOK_KW_WHILE expression TOK_KW_DO StatementSequence ElsifDo TOK_KW_END {
	auto* result = new WhileStmt();
	result->cond.reset($2);
	if ($5) {
		result->branches = std::move(*$5);
		delete $5;
	}
	for (auto* el : *$4){
		result->body.push_back(Ptr<Stmt>(el));
	}
	delete $4;
	$$ = result;
}

ElsifDo: %empty {
	$$ = nullptr;
} | ElsifDo TOK_KW_ELSIF expression TOK_KW_DO StatementSequence {
	if (!$1) $1 = new std::vector<Branch>();
	Branch branch;
	branch.cond = Ptr<Expr>($3);
	for (auto* el : *$5){
		branch.body.push_back(Ptr<Stmt>(el));
	}
	delete $5;
	$1->push_back(std::move(branch));
	$$ = $1;
};

/* RepeatStatement = REPEAT StatementSequence UNTIL expression. */
RepeatStatement: TOK_KW_REPEAT StatementSequence TOK_KW_UNTIL expression {
	auto* result = new RepeatStmt();
	result->untilCond.reset($4);
	for (auto* el : *$2){
		result->body.push_back(Ptr<Stmt>(el));
	}
	delete $2;
	$$ = result;
};

/* ForStatement = FOR ident ":=" expression TO expression [BY ConstExpression] DO StatementSequence END. */
ForStatement: TOK_KW_FOR TOK_IDENT TOK_ASSIGN expression TOK_KW_TO expression TOK_KW_BY ConstExpression TOK_KW_DO StatementSequence TOK_KW_END {
	auto* result = new ForStmt();
	result->varName = $2->text;
	delete $2;
	result->from = Ptr<Expr>($4);
	result->to = Ptr<Expr>($6);
	result->by = Ptr<Expr>($8);
	for (auto* el : *$10){
		result->body.push_back(Ptr<Stmt>(el));
	}
	delete $10;
	$$ = result;
} | TOK_KW_FOR TOK_IDENT TOK_ASSIGN expression TOK_KW_TO expression TOK_KW_DO StatementSequence TOK_KW_END {
	auto* result = new ForStmt();
	result->varName = $2->text;
	delete $2;
	result->from = Ptr<Expr>($4);
	result->to = Ptr<Expr>($6);
	for (auto* el : *$8){
		result->body.push_back(Ptr<Stmt>(el));
	}
	delete $8;
	$$ = result;
}

/* ProcedureDeclaration = ProcedureHeading ";" ProcedureBody ident. */
ProcedureDeclaration: ProcedureHeading TOK_SEMICOLON ProcedureBody TOK_IDENT {
	if ($1->name != $4->text) {
		std::string msg = "Procedure name mismatch: expected '" + $1->name +
		                  "', but found '" + $4->text + "'";
		std::string fullMsg = msg + " at line " + std::to_string($4->line);
		yyerror(ctx, fullMsg.c_str());
		delete $4;
		delete $1;
		delete $3;
		YYERROR;
	}
	delete $4;
	$3->name = $1->name;
	$3->exported = $1->exported;
	$3->type = std::move($1->type);
	delete $1;
	$$ = $3;
};

/* ProcedureHeading = PROCEDURE identdef [FormalParameters]. */
ProcedureHeading: TOK_KW_PROCEDURE identdef FormalParameters {
	auto* result = new ProcDecl();
	result->name = $2->name;
	result->exported = $2->exported;
	result->type.reset($3);
	delete $2;
	$$ = result;
} | TOK_KW_PROCEDURE identdef {
	auto* result = new ProcDecl();
	result->name = $2->name;
	result->exported = $2->exported;
	delete $2;
	$$ = result;
};

/* ProcedureBody = DeclarationSequence [BEGIN StatementSequence] [RETURN expression] END. */
ProcedureBody: DeclarationSequence TOK_KW_BEGIN StatementSequence TOK_KW_RETURN expression TOK_KW_END {
	auto* result = new ProcDecl();
	for (auto* el : *$1){
		result->decls.push_back(Ptr<Decl>(el));
	}
	for (auto* el : *$3){
		result->body.push_back(Ptr<Stmt>(el));
	}
	result->returnValue.reset($5);
	delete $1;
	delete $3;
	$$ = result;
} | DeclarationSequence TOK_KW_RETURN expression TOK_KW_END {
	auto* result = new ProcDecl();
	for (auto* el : *$1){
		result->decls.push_back(Ptr<Decl>(el));
	}
	result->returnValue.reset($3);
	delete $1;
	$$ = result;
} | DeclarationSequence TOK_KW_BEGIN StatementSequence TOK_KW_END {
	auto* result = new ProcDecl();
	for (auto* el : *$1){
		result->decls.push_back(Ptr<Decl>(el));
	}
	for (auto* el : *$3){
		result->body.push_back(Ptr<Stmt>(el));
	}
	delete $1;
	delete $3;
	$$ = result;
} | DeclarationSequence TOK_KW_END {
	auto* result = new ProcDecl();
	for (auto* el : *$1){
		result->decls.push_back(Ptr<Decl>(el));
	}
	delete $1;
	$$ = result;
};

/* DeclarationSequence = [CONST {ConstDeclaration ";"}] [TYPE {TypeDeclaration ";"}] [VAR {VariableDeclaration ";"}] {ProcedureDeclaration ";"}. */
DeclarationSequence: ConstDeclarationOpt TypeDeclarationOpt VariableDeclarationOpt ProcedureDeclarationList {
	auto* result = new std::vector<Decl*>;
	result->insert(result->end(), $1->begin(), $1->end());
	result->insert(result->end(), $2->begin(), $2->end());
	result->insert(result->end(), $3->begin(), $3->end());
	result->insert(result->end(), $4->begin(), $4->end());
	delete $1;
	delete $2;
	delete $3;
	delete $4;
	$$ = result;
};

ConstDeclarationOpt: %empty {
	$$ = new std::vector<Decl*>;
} | TOK_KW_CONST ConstDeclarationList {
	$$ = $2;
}

ConstDeclarationList: ConstDeclaration TOK_SEMICOLON {
	auto* ptr = new std::vector<Decl*>;
	ptr->push_back($1);
	$$ = ptr;
} | ConstDeclarationList ConstDeclaration TOK_SEMICOLON {
	$1->push_back($2);
	$$ = $1;
};

TypeDeclarationOpt: %empty {
	$$ = new std::vector<Decl*>;
} | TOK_KW_TYPE TypeDeclarationList {
	$$ = $2;
}

TypeDeclarationList: TypeDeclaration TOK_SEMICOLON {
	auto* ptr = new std::vector<Decl*>;
	ptr->push_back($1);
	$$ = ptr;
} | TypeDeclarationList TypeDeclaration TOK_SEMICOLON {
	$1->push_back($2);
	$$ = $1;
};

VariableDeclarationOpt: %empty {
	$$ = new std::vector<Decl*>;
} | TOK_KW_VAR VariableDeclarationList {
	$$ = $2;
};

VariableDeclarationList: VariableDeclaration TOK_SEMICOLON {
	auto* ptr = new std::vector<Decl*>;
	ptr->push_back($1);
	$$ = ptr;
} | VariableDeclarationList VariableDeclaration TOK_SEMICOLON {
	$1->push_back($2);
	$$ = $1;
};

ProcedureDeclarationList: %empty {
	$$ = new std::vector<Decl*>();
} | ProcedureDeclarationList ProcedureDeclaration TOK_SEMICOLON {
	$1->push_back($2);
	$$ = $1;
};

/* module = MODULE ident ";" [ImportList] DeclarationSequence [BEGIN StatementSequence] END ident ".". */
module: TOK_KW_MODULE TOK_IDENT TOK_SEMICOLON ImportList DeclarationSequence TOK_KW_BEGIN StatementSequence TOK_KW_END TOK_IDENT TOK_DOT {
	auto* res = new Module();
	res->name = $2->text;
	delete $2;
	res->imports = std::move(*$4);
	for (auto* el : *$5){
		res->decls.push_back(Ptr<Decl>(el));
	}
	for (auto* el : *$7){
		res->block.push_back(Ptr<Stmt>(el));
	}
	res->endName = $9->text;
	delete $9;
	delete $4; delete $5; delete $7;
	$$ = res;
} | TOK_KW_MODULE TOK_IDENT TOK_SEMICOLON DeclarationSequence TOK_KW_BEGIN StatementSequence TOK_KW_END TOK_IDENT TOK_DOT {
	auto* res = new Module();
	res->name = $2->text;
	delete $2;
	for (auto* el : *$4){
		res->decls.push_back(Ptr<Decl>(el));
	}
	for (auto* el : *$6){
		res->block.push_back(Ptr<Stmt>(el));
	}
	res->endName = $8->text;
	delete $8;
	delete $4; delete $6;
	$$ = res;
} | TOK_KW_MODULE TOK_IDENT TOK_SEMICOLON ImportList DeclarationSequence TOK_KW_END TOK_IDENT TOK_DOT {
	auto* res = new Module();
	res->name = $2->text;
	delete $2;
	res->imports = std::move(*$4);
	for (auto* el : *$5){
		res->decls.push_back(Ptr<Decl>(el));
	}
	res->endName = $7->text;
	delete $7;
	delete $4; delete $5;
	$$ = res;
} | TOK_KW_MODULE TOK_IDENT TOK_SEMICOLON DeclarationSequence TOK_KW_END TOK_IDENT TOK_DOT {
	auto* res = new Module();
	res->name = $2->text;
	delete $2;
	for (auto* el : *$4){
		res->decls.push_back(Ptr<Decl>(el));
	}
	res->endName = $6->text;
	delete $6;
	delete $4;
	$$ = res;
};

/* ImportList = IMPORT import {"," import} ";". */
ImportList: TOK_KW_IMPORT imports TOK_SEMICOLON {
	$$ = $2;
};

imports : import {
	auto* res = new std::vector<Import>;
	res->push_back(std::move(*$1));
	delete $1;
	$$ = res;
} | imports TOK_COMMA import {
	$1->push_back(std::move(*$3));
	delete $3;
	$$ = $1;
};

/* import = ident [":=" ident].  First ident = alias, second = real module name. */
import: TOK_IDENT {
	auto* res = new Import();
	res->name = $1->text;
	delete $1;
	$$ = res;
} | TOK_IDENT TOK_ASSIGN TOK_IDENT {
	auto* res = new Import();
	res->alias = $1->text;
	res->name = $3->text;
	delete $1;
	delete $3;
	$$ = res;
};
