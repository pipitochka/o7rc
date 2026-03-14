#pragma once
#include <memory>

struct IVisitor;

struct Node;

struct Module;
struct Import;
struct Block;

struct Decl;
struct ConstDecl;
struct TypeDecl;
struct VarDecl;
struct ProcDecl;
struct ParamDecl;

struct TypeNode;
struct NamedType;
struct ArrayType;
struct RecordType;
struct PointerType;
struct ProcType;
struct FieldDecl;

struct Stmt;
struct AssignStmt;
struct CallStmt;
struct IfStmt;
struct WhileStmt;
struct RepeatStmt;
struct ForStmt;
struct ReturnStmt;
struct CaseStmt;

struct Expr;
struct LiteralExpr;
struct UnaryExpr;
struct BinaryExpr;
struct CallExpr;
struct DesignatorExpr;
struct IsExpr;
struct InExpr;

struct Selector;
struct FieldSelector;
struct IndexSelector;
struct DerefSelector;

struct CaseAlternative;
struct CaseLabel;

template<class T>
using Ptr = std::unique_ptr<T>;

using NodePtr = Ptr<Node>;
using ExprPtr = Ptr<Expr>;
using StmtPtr = Ptr<Stmt>;
using DeclPtr = Ptr<Decl>;
using TypePtr = Ptr<TypeNode>;
using SelectorPtr = Ptr<Selector>;

using ModulePtr = Ptr<Module>;
