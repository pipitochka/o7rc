#pragma once
#include <parser/IParser.h>
#include <tokenizer/ITokenizer.h>
#include <util/ast/Ast.h>

#include <stdexcept>
#include <string>

class HandParser final : public IParser {
public:
    HandParser() = default;
    ModulePtr parse(ITokenizerPtr tz) override;
    ~HandParser() override = default;

private:
    ITokenizerPtr tz_;

    Token peek();
    Token advance();
    Token expect(TokenType tt);
    bool check(TokenType tt);
    bool match(TokenType tt);

    [[noreturn]] void error(const std::string& msg);

    // Grammar productions
    ModulePtr parseModule();
    std::vector<Import> parseImportList();
    Import parseImport();

    std::vector<DeclPtr> parseDeclarationSequence();
    DeclPtr parseConstDecl();
    DeclPtr parseTypeDecl();
    DeclPtr parseVarDecl();
    DeclPtr parseProcDecl();

    TypePtr parseType();
    TypePtr parseArrayType();
    TypePtr parseRecordType();
    TypePtr parsePointerType();
    TypePtr parseProcedureType();
    TypePtr parseFormalParameters();
    TypePtr parseFormalType();

    ExprPtr parseExpression();
    ExprPtr parseSimpleExpression();
    ExprPtr parseTerm();
    ExprPtr parseFactor();

    Ptr<DesignatorExpr> parseDesignator();
    std::vector<ExprPtr> parseExprList();

    StmtPtr parseStatement();
    std::vector<StmtPtr> parseStatementSequence();
    StmtPtr parseIfStatement();
    StmtPtr parseWhileStatement();
    StmtPtr parseRepeatStatement();
    StmtPtr parseForStatement();
    StmtPtr parseCaseStatement();

    struct IdentDef {
        std::string name;
        bool exported = false;
    };
    IdentDef parseIdentDef();
    std::vector<IdentDef> parseIdentList();
};
