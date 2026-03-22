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
  if (ctx) ctx->lastError = msg;
    Token t = ctx->tz->peek();

  std::fprintf(stderr, "Parse error at %d:%d: %s. Found token: '%s'\n",
      t.line, t.col, msg, t.text.c_str());
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

//%destructor { delete $$; } <identDef> <qualIdent> <expr> <type> <decl> <stmt> <designatorExpr>
//%destructor { for(auto* e:*$$) delete e; delete $$; } <expr_list> <decl_list> <stmt_list> /* и т.д. для всех _list */

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
%type <expr> expression ConstExpression SimpleExpression Add_list_opt Add_list first termList factor label term set
%type <typeDecl> TypeDeclaration
%type <type> type ArrayType RecordType BaseType BaseTypeOpt PointerType ProcedureType FormalParametersOpt FormalParameters FormalType
%type <expr_list> exprList ActualParameters
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
%type <stmt> statement assignment ProcedureCall WhileStatement IfStatement RepeatStatement ForStatement CaseStatement
%type <stmt_list> StatementSequence
%type <elif> ElifOpt ElsifDo
%type <caseLabel> LabelRange
%type <caseLabel_list> CaseLabelList
%type <caseAlternative> case
%type <caseAlternative_list> caseList
%type <procDecl> ProcedureDeclaration ProcedureHeading ProcedureBody
%type <decls_list> DeclarationSequence ConstDeclarationOpt ConstDeclarationList TypeDeclarationOpt TypeDeclarationList VariableDeclarationOpt VariableDeclarationList  ProcedureDeclarationList
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

//number     = integer | real.
//возвращает  LiteralExpr*
number : TOK_INTEGER {
	auto* lit = new LiteralExpr();
	lit->kind = LiteralExpr::Kind::Int;
	lit->intValue = $1->intValue;
	$$ = lit;
} | TOK_REAL {
	auto* lit = new LiteralExpr();
	lit->kind = LiteralExpr::Kind::Real;
	lit->intValue = $1->realValue;
	$$ = lit;
} ;

//qualident  = [ident "."] ident.
//возвращает QualIdent*
qualident: TOK_IDENT {
	$$ = new QualIdent{"", $1->text}; 
} | TOK_IDENT TOK_DOT TOK_IDENT {
	$$ = new QualIdent{$1->text, $3->text}; 
};


//identdef   = ident ["*"].
//возвращает IdentDef*
identdef: TOK_IDENT {
	$$ = new IdentDef{$1->text, false};
} | TOK_IDENT TOK_STAR {
	$$ = new IdentDef{$1->text, true};
};

//ConstDeclaration = identdef "=" ConstExpression.
//возвращает ConstDecl*
ConstDeclaration : identdef TOK_EQ ConstExpression {
	auto* decl = new ConstDecl();
	decl->name = $1->name;
	decl->value.reset($3);
	$$ = decl;
	delete $1;
};

//ConstExpression  = expression.
//return Expr*
ConstExpression: expression {
	$$ = $1;
};

//TypeDeclaration   = identdef "=" type.
//return TypeDecl*
TypeDeclaration: identdef TOK_EQ type {
	auto* typeDecl = new TypeDecl();
	typeDecl->name = $1->name;
	typeDecl->type.reset($3);
	$$ = typeDecl;
	delete $1;
};

//type              = qualident | ArrayType | RecordType | PointerType | ProcedureType.
//return Type*
type : qualident {
	auto* named = new NamedType();
    named->name = $1->name;
    $$ = named;
    delete $1;
}| ArrayType {
	$$ = $1;
}| RecordType {
	$$ = $1;
}| PointerType {
	$$ = $1;
}| ProcedureType {
	$$ = $1;
};
	
//ArrayType         = ARRAY length {"," length} OF type.
//return Type*
ArrayType: TOK_KW_ARRAY exprList TOK_KW_OF type {
	auto* type = new ArrayType();
	type->elemType.reset($4);
	for (auto* e : *$2) {
		type->length.push_back(Ptr<Expr>(e));
    }
	$$ = type;
};

//return std::vector<Expr*>*
exprList
  : expression { 
	$$ = new std::vector<Expr*>(); 
	$$->push_back($1); 
} | exprList TOK_COMMA expression { 
	$1->push_back($3); 
	$$ = $1; 
};

//RecordType        = RECORD ["(" BaseType ")"] [FieldListSequence] END.
//return Type*
RecordType: TOK_KW_RECORD BaseTypeOpt FieldListSequenceOpt TOK_KW_END {
	auto* type = new RecordType();
	type->baseType.reset($2);
	if ($3) {
        for (auto* e : *$3) {
            type->fields.push_back(Ptr<FieldDecl>(e));
        }
        delete $3;
    }
	$$ = type;
};

//return Type*
BaseTypeOpt: %empty { 
    $$ = nullptr; 
}  | TOK_LPAREN BaseType TOK_RPAREN {
	$$ = $2;
};

//return Type*
BaseType: qualident {
	auto* named = new NamedType();
	named->name = $1->module.empty() ? $1->name : ($1->module + "." + $1->name);
	delete $1;
	$$ = named;
};

//std::vector<FieldDecl*>*
FieldListSequenceOpt:  %empty {
	$$ = nullptr;
} | FieldListSequence {
	$$ = $1;
};

//IdentList ":" type
//return std::vector<FieldDecl*>*
FieldListSequence: FieldList {
	auto* result = new std::vector<FieldDecl*>();
	result->push_back($1);
	$$ = result;
} | FieldListSequence TOK_SEMICOLON FieldList {
    $1->push_back($3);
    $$=$1;
};

//IdentList ":" type.
//return FieldDecl*
FieldList: IdentList TOK_SEMICOLON type { 
    auto* result = new FieldDecl();
    for (auto* el : *$1){ 
        result->names.push_back(el->name);
        delete el;
    }
    delete $1;
    result->type.reset($3);
    $$=result;
}

//IdentList         = identdef {"," identdef}.
//return std::vector<IdentDef*>*
IdentList: identdef {
	$$ = new std::vector<IdentDef*>;
	$$->push_back($1); 
} | IdentList TOK_COMMA identdef {
	$1->push_back($3); 
    $$ = $1; 
};

//PointerType       = POINTER TO type.
//return Type*
PointerType: TOK_KW_POINTER TOK_KW_TO type {
	auto* type = new PointerType();
	type->baseType.reset($3);
	$$ = type;
};

//ProcedureType     = PROCEDURE [FormalParameters].
//return Type*
ProcedureType : TOK_KW_PROCEDURE FormalParametersOpt {
	$$ = $2;
};

//return Type*
FormalParametersOpt: %empty {
	$$ = nullptr;
} | FormalParameters {
	$$ = $1;
};

//FormalParameters   = "(" [FPSection {";" FPSection}] ")" [":" qualident].
//return Type*
FormalParameters: TOK_LPAREN FormalParametersLOpt TOK_RPAREN FormalParameterROpt {
	auto* params = new ProcType();
	params->type = std::make_unique<NamedType>();
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

//return qualident*
FormalParameterROpt: %empty {
	$$ = nullptr;
} | TOK_COLON qualident {
	$$ = $2;
}

//return std::vector<ProcParams*>
FormalParametersLOpt: %empty {
	$$ = nullptr;
} | FPSectionList {
	$$ = $1;
};

//return std::vector<ProcParams*>
FPSectionList: FPSection {
	auto* data = new std::vector<ProcParams*>();
	data->push_back($1);
} | FPSectionList TOK_SEMICOLON FPSection {
	$1->push_back($3);
	$$ = $1;
}

//FPSection          = [VAR] ident {"," ident} ":" FormalType.
//return ProcParams*
FPSection: TOK_KW_VAR identList TOK_COLON FormalType {
	auto* data = new ProcParams();
	for (auto el: *$2){ 
	    data->names.push_back(el);    
	}
	data->type.reset($4);
	$$ = data;
} | identList TOK_COLON FormalType { 
    auto* data = new ProcParams();
    for (auto el: *$1){ 
        data->names.push_back(el);    
    }
    data->type.reset($3);
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

//FormalType         = {ARRAY OF} qualident.
//return type*
FormalType: qualident {
	auto* named = new NamedType();
    named->name = $1->module.empty() ? $1->name : ($1->module + "." + $1->name);
    delete $1;
} | TOK_KW_ARRAY TOK_KW_OF FormalType {
	auto* arr = new ArrayType();
    arr->elemType.reset($3);   
    $$ = arr;
}

//VariableDeclaration = IdentList ":" type.
//return VarDecl;
VariableDeclaration : IdentList TOK_COLON type {
	auto* decl = new VarDecl();
	decl->type.reset($3);
	for (auto* el : *$1) {
        decl->names.push_back(el->name);
        delete el;
    }
	$$ = decl;
	delete $1;
};

//expression        = SimpleExpression [relation SimpleExpression].
//return Expr*
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

//SimpleExpression  = ["+" | "-"] term {AddOperator term}.
SimpleExpression: first {
	$$ = $1;
} | first AddOperator Add_list_opt {
	auto* op = new BinaryExpr();
	op->lhs.reset($1);
	op->rhs.reset($3);
	op->op = $2;
	$$ = op;
}

first: term {
	$$ = $1;
} | TOK_PLUS term {
	$$ = $2;
} | TOK_MINUS term {
	auto* expr = new UnaryExpr();
	expr->rhs.reset($2);
	$$ = expr;
}

Add_list_opt: %empty {
	$$ = nullptr;
} | Add_list {
	$$ = $1;
}

//return 
Add_list : term {
	$$ = $1;
} | term AddOperator Add_list {
	auto* expr = new BinaryExpr();
	expr->lhs.reset($1);
	expr->rhs.reset($3);
	expr->op = $2;
	$$ = expr;
};

AddOperator: TOK_PLUS {
	$$ = BinaryExpr::Op::Add;
} | TOK_MINUS {
	$$ = BinaryExpr::Op::Sub;
} | TOK_KW_OR {
	$$ = BinaryExpr::Op::Or;
};

//term              = factor {MulOperator factor}.
term : factor {$$ = $1;} | termList {$$ = $1;};

termList: factor {
	$$=$1;
} | factor MulOperator termList {
	auto* expr = new BinaryExpr();
	expr->lhs.reset($1);
	expr->rhs.reset($3);
	expr->op = $2;
	$$ = expr;
}

MulOperator: TOK_STAR {
	$$ = BinaryExpr::Op::Mul;
} | TOK_SLASH {
	$$ = BinaryExpr::Op::IDiv;
} | TOK_AMP {
	$$ = BinaryExpr::Op::And;
} | TOK_KW_DIV{ 
    $$ = BinaryExpr::Op::IDiv; 
} | TOK_KW_MOD{ $$ = BinaryExpr::Op::Mod; };

//factor            = number | string | NIL | TRUE | FALSE | set | designator [ActualParameters] | "(" expression ")" | "~" factor
//return expr*
factor: number {
	$$ = $1;
} | TOK_STRING {
	auto* val = new LiteralExpr();
	val->kind = LiteralExpr::Kind::String;
	val->strValue = $1->text;
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
} | designator ActualParameters{
	auto* call = new CallExpr();
	call->callee.reset($1);
	for (auto* e : *$2) {
		call->args.push_back(Ptr<Expr>(e));
	}
	delete $2;
	$$ = call;
} | TOK_LPAREN expression TOK_RPAREN {
	$$ = $2;
} | TOK_TILDE factor {
	auto* result = new UnaryExpr();
	result->rhs.reset($2);
	result->op = UnaryExpr::Op::Not;
	$$ = result;
}


//designator        = qualident {selector}.
//DesignatorExpr* 
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
	auto* list = new std::vector<Selector*>;
	$$ = list;
} | selectorList {
	$$ = $1;
}

//return std::vector<Selector*>*
selectorList: selector {
	auto* list = new std::vector<Selector*>;
	list->push_back($1);
	$$ = list;
} | selectorList selector  {
	$1->push_back($2);
	$$ = $1;
};

//selector          = "." ident | "[" ExpList "]" | "^" | "(" qualident ")".
//return Selector*
selector: TOK_DOT TOK_IDENT {
	auto* selector = new FieldSelector();
	selector->name = $2->text;
	$$ = selector;
} | TOK_LBRACK exprList TOK_RBRACK {
	auto* selector = new IndexSelector();
	for (auto* exp : *$2) { selector->index.push_back(Ptr<Expr>(exp)); }
	delete $2;
	$$ = selector;
} | TOK_CARET {
	$$ = new DerefSelector();
} | TOK_LPAREN qualident TOK_RPAREN {
	auto* selector = new TypeGuardSelector();
	selector->typeName = $2->module.empty() ? $2->name : ($2->module + "." + $2->name);
	$$ = selector;
};

//set               = "{" [element {"," element}] "}".
////  return Expr*
set: TOK_LBRACE elementOpt TOK_RBRACE {
	auto* set = new SetExpr();
	set->elements = std::move(*$2);
	$$ = set;
};

//  return std::vector<setElement>* setElement
elementOpt: %empty {
	auto* el = new std::vector<SetElement>();
	$$ = el;
} | elements {
	$$ = $1;
};

//  return std::vector<setElement>* setElement
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

//element           = expression [".." expression].
//return setElement*
element: expression{
	auto* setElem = new SetElement();
	setElem->low.reset($1);
	$$ = setElem;
} | expression TOK_RANGE expression {
	auto* setElem = new SetElement();
	setElem->low.reset($1);
	setElem->high.reset($3);
	$$ = setElem;
};


//ActualParameters  = "(" [ExpList] ")" .
//return std::vector<Expr*>*
ActualParameters: TOK_LPAREN exprList TOK_RPAREN {
	$$ = $2;
} | TOK_LPAREN TOK_RPAREN {
	$$ = new std::vector<Expr*>();
};

//statement         = [assignment | ProcedureCall | IfStatement | CaseStatement |WhileStatement | RepeatStatement | ForStatement].
//return Stmn*
statement: %empty { 
    $$ = nullptr; 
} | assignment {
	$$ = $1;
} | ProcedureCall {
	$$ = $1;
} | IfStatement {
	$$ = $1;
} | CaseStatement {
	$$ = $1;
} | WhileStatement {
	$$ = $1;
} | RepeatStatement{
	$$ = $1;
} | ForStatement {
	$$ = $1;
};

//assignment        = designator ":=" expression.
//return Stmt*
assignment: designator TOK_ASSIGN expression {
	auto* result = new AssignStmt();
	result->lhs.reset($1);
	result->rhs.reset($3);
	$$ = result;
};

//ProcedureCall     = designator [ActualParameters].
//return Stmt*
ProcedureCall: designator {
	auto* call = new CallExpr();
	call->callee.reset($1);
	auto* stm = new CallStmt();
	stm->call.reset(call);
	$$ = stm;
} | designator ActualParameters {
	auto* call = new CallExpr();
	call->callee.reset($1);
	for (auto* e : *$2) {
	  call->args.push_back(ExprPtr(e));
	}
	delete $2;
	auto* stm = new CallStmt();
	stm->call.reset(call);
	$$ = stm;
};

//StatementSequence = statement {";" statement}.
//return std::vector<Stmt*>
StatementSequence: statement {
	auto* vec = new std::vector<Stmt*>();
    if ($1) vec->push_back($1);
    $$ = vec;
} | StatementSequence TOK_SEMICOLON statement {
	if ($3) $1->push_back($3);
    $$ = $1;
}

//IfStatement       = IF expression THEN StatementSequence {ELSIF expression THEN StatementSequence} [ELSE StatementSequence] END.
//return Stmt*
IfStatement: TOK_KW_IF expression TOK_KW_THEN StatementSequence ElifOpt TOK_KW_ELSE StatementSequence TOK_KW_END {
	auto* stmt = new IfStmt();
	auto branch = Branch();
	branch.cond = Ptr<Expr>($2);
	for (auto* el : *$4){
        branch.body.push_back(Ptr<Stmt>(el));
    }
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
}|TOK_KW_IF expression TOK_KW_THEN StatementSequence ElifOpt TOK_KW_END {
	auto* stmt = new IfStmt();
	auto branch = Branch();
    branch.cond = Ptr<Expr>($2);
	for (auto* el : *$4){
		branch.body.push_back(Ptr<Stmt>(el));
	}
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
	$$ = new std::vector<Branch>();
} | TOK_KW_ELSIF expression TOK_KW_THEN StatementSequence {
	auto* branches = new std::vector<Branch>();
	auto branch = Branch();
	branch.cond = Ptr<Expr>($2);
	for (auto* el : *$4){
		branch.body.push_back(Ptr<Stmt>(el));
	}
	branches->push_back(std::move(branch));
	$$ = branches;
}| ElifOpt TOK_KW_ELSIF expression TOK_KW_THEN StatementSequence {
	auto branch = Branch();
	branch.cond = Ptr<Expr>($3);
	for (auto* el : *$5){
		branch.body.push_back(Ptr<Stmt>(el));
	}
	$1->push_back(std::move(branch));
    delete $5;
	$$ = $1;
};

//CaseStatement     = CASE expression OF case {"|" case} END.
CaseStatement: TOK_KW_CASE expression TOK_KW_OF caseList TOK_KW_END {
	auto* res = new CaseStmt();
	res->expr.reset($2);
	for (auto* el : *$4){
		res->alts.push_back(Ptr<CaseAlternative>(el));
	}
	$$ = res;
};

caseList: case {
	auto* v = new std::vector<CaseAlternative*>();
	v->push_back($1);
	$$ = v;
} | caseList TOK_KW_OR case {
	$1->push_back($3);
	$$ = $1;
};

//case              = [CaseLabelList ":" StatementSequence].
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
	$$ = res;
};

//CaseLabelList     = LabelRange {"," LabelRange}.
//return std::vector<CaseLabel*>*
CaseLabelList: LabelRange {
	auto* vec = new std::vector<CaseLabel*>();
	vec->push_back($1);
	$$ = vec;
} | CaseLabelList TOK_COMMA LabelRange {
	$1->push_back($3);
	$$ = $1;
};


//LabelRange        = label [".." label].
//return CaseLabel*
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

//label             = integer | string | qualident.
//return Expr*
label : TOK_INTEGER {
	auto* lit = new LiteralExpr();
	lit->kind = LiteralExpr::Kind::Int;
	lit->intValue = $1->intValue;
	$$ = lit;
} | TOK_STRING {
	auto* lit = new LiteralExpr();
	lit->kind = LiteralExpr::Kind::String;
	lit->intValue = $1->intValue;
	$$ = lit;
}| qualident {
	auto* des = new DesignatorExpr;
	if ($1->module.empty())
	  des->baseName = $1->name;
	else
	  des->baseName = $1->module + "." + $1->name;
	delete $1;
	$$ = des;
};


//WhileStatement    = WHILE expression DO StatementSequence {ELSIF expression DO StatementSequence} END.
WhileStatement : TOK_KW_WHILE expression TOK_KW_DO StatementSequence ElsifDo TOK_KW_END{
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
	$$ = new std::vector<Branch>();
} | TOK_KW_ELSIF expression TOK_KW_DO StatementSequence {
	auto* branches = new std::vector<Branch>();
	auto branch = Branch();
	branch.cond = Ptr<Expr>($2);
	for (auto* el : *$4){
		branch.body.push_back(Ptr<Stmt>(el));
	}
	branches->push_back(std::move(branch));
    delete $4;
	$$ = branches;
}| ElsifDo TOK_KW_ELSIF expression TOK_KW_DO StatementSequence {
	auto branch = Branch();
	branch.cond = Ptr<Expr>($3);
	for (auto* el : *$5){
		branch.body.push_back(Ptr<Stmt>(el));
	}
    $1->push_back(std::move(branch));
    delete $5;
    $$ = $1;
};

RepeatStatement: TOK_KW_REPEAT StatementSequence TOK_KW_UNTIL expression {
	auto* result = new RepeatStmt();
	result->untilCond.reset($4);
	for (auto* el : *$2){
		result->body.push_back(Ptr<Stmt>(el));
	}
	$$ = result;
};

//ForStatement      = FOR ident ":=" expression TO expression [BY ConstExpression] DO StatementSequence END.
ForStatement: TOK_KW_FOR TOK_IDENT TOK_ASSIGN expression TOK_KW_TO expression TOK_KW_BY ConstExpression TOK_KW_DO StatementSequence TOK_KW_END {
	auto* result = new ForStmt();
	result->varName = $2->text;
	result->from = Ptr<Expr>($4);
	result->to = Ptr<Expr>($6);
	result->by = Ptr<Expr>($8);
	for (auto* el : *$10){
		result->body.push_back(Ptr<Stmt>(el));
	}
	$$ = result;
	
} | TOK_KW_FOR TOK_IDENT TOK_ASSIGN expression TOK_KW_TO expression TOK_KW_DO StatementSequence TOK_KW_END {
	auto* result = new ForStmt();
	result->varName = $2->text;
	result->from = Ptr<Expr>($4);
	result->to = Ptr<Expr>($6);
	for (auto* el : *$8){
		result->body.push_back(Ptr<Stmt>(el));
	}
	$$ = result;
}


ProcedureDeclaration: ProcedureHeading TOK_SEMICOLON ProcedureBody TOK_IDENT {
    if ($1->name != $4->text) {
        std::string msg = "Procedure name mismatch: expected '" + $1->name +
                          "', but found '" + $4->text + "'";

        std::string fullMsg = msg + " at line " + std::to_string($4->line);
        yyerror(ctx, fullMsg.c_str());
        YYERROR;
    }
	$3->name = $1->name;
	$3->type = std::move($1->type);
	delete $1;
	$$ = $3;
};

//ProcedureHeading   = PROCEDURE identdef [FormalParameters].
ProcedureHeading: TOK_KW_PROCEDURE identdef FormalParameters {
	auto* result = new ProcDecl();
	result->name = $2->name;
	result->type.reset($3);
	$$ = result;
} | TOK_KW_PROCEDURE identdef {
	auto* result = new ProcDecl();
	result->name = $2->name;
	$$ = result;
};


//ProcedureBody      = DeclarationSequence [BEGIN StatementSequence] [RETURN expression] END.
//ProcedureBody      = DeclarationSequence [BEGIN StatementSequence] [RETURN expression] END.
ProcedureBody: DeclarationSequence TOK_KW_BEGIN StatementSequence TOK_KW_RETURN expression TOK_KW_END {
	auto* result = new ProcDecl();
	for (auto* el : *$1){
		result->params.push_back(Ptr<Decl>(el));
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
		result->params.push_back(Ptr<Decl>(el));
	}
	result->returnValue.reset($3);
    delete $1;
	$$ = result;	
} | DeclarationSequence TOK_KW_BEGIN StatementSequence TOK_KW_END {
	auto* result = new ProcDecl();
	for (auto* el : *$1){
		result->params.push_back(Ptr<Decl>(el));
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
		result->params.push_back(Ptr<Decl>(el));
	}
    delete $1;
	$$ = result;	
};

//DeclarationSequence = [CONST {ConstDeclaration ";"}] [TYPE {TypeDeclaration ";"}] [VAR {VariableDeclaration ";"}] {ProcedureDeclaration ";"}.
//return std::vector<Decl*>*
DeclarationSequence: ConstDeclarationOpt TypeDeclarationOpt VariableDeclarationOpt ProcedureDeclarationList {
	auto* result = new std::vector<Decl*>;
	result->insert(result->end(), $1->begin(), $1->end());
	result->insert(result->end(), $2->begin(), $2->end());
	result->insert(result->end(), $3->begin(), $3->end());
	result->insert(result->end(), $4->begin(), $4->end());
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

//module = MODULE ident ";" [ImportList] DeclarationSequence [BEGIN StatementSequence] END ident "." .
module: TOK_KW_MODULE TOK_IDENT TOK_SEMICOLON ImportList DeclarationSequence TOK_KW_BEGIN StatementSequence TOK_KW_END TOK_IDENT TOK_DOT {
	auto* res = new Module();
	res->name = $2->text;
	res->imports = std::move(*$4);
	for (auto* el : *$5){
		res->decls.push_back(Ptr<Decl>(el));
	}
	for (auto* el : *$7){
		res->block.push_back(Ptr<Stmt>(el));
	}
	res->endName = $9->text;
    delete $4; delete $5; delete $7;
	$$ = res;
} | TOK_KW_MODULE TOK_IDENT TOK_SEMICOLON  DeclarationSequence TOK_KW_BEGIN StatementSequence TOK_KW_END TOK_IDENT TOK_DOT {
	auto* res = new Module();
	res->name = $2->text;
	for (auto* el : *$4){
		res->decls.push_back(Ptr<Decl>(el));
	}
	for (auto* el : *$6){
		res->block.push_back(Ptr<Stmt>(el));
	}
	res->endName = $8->text;
    delete $4; delete $6;
	$$ = res;
} | TOK_KW_MODULE TOK_IDENT TOK_SEMICOLON ImportList DeclarationSequence TOK_KW_END TOK_IDENT TOK_DOT {
	auto* res = new Module();
	res->name = $2->text;
	res->imports = std::move(*$4);
	for (auto* el : *$5){
		res->decls.push_back(Ptr<Decl>(el));
	}
	res->endName = $7->text;
    delete $4; delete $5;
	$$ = res;
} | TOK_KW_MODULE TOK_IDENT TOK_SEMICOLON  DeclarationSequence  TOK_KW_END TOK_IDENT TOK_DOT {
	auto* res = new Module();
	res->name = $2->text;
	for (auto* el : *$4){
		res->decls.push_back(Ptr<Decl>(el));
	}
	res->endName = $6->text;
    delete $4;
	$$ = res;
};


//ImportList         = IMPORT import {"," import} ";".
ImportList: TOK_KW_IMPORT imports TOK_SEMICOLON {
	$$ = $2;
};

imports : import {
	auto* res = new std::vector<Import>;
	res->push_back(std::move(*$1)); 
    delete $1;
	$$ = res;
} | imports TOK_COMMA import  {
	$1->push_back(std::move(*$3));
	$$ = $1;
};

//import             = ident [":=" ident].
import: TOK_IDENT {
	auto* res = new Import();
	res->name = $1->text;
	$$ = res;
} | TOK_IDENT TOK_ASSIGN TOK_IDENT {
	auto* res = new Import();
	res->name = $1->text;
	res->alias = $3->text;
	$$ = res;
};