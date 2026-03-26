#include "HandParser.h"
#include <cstdio>

ModulePtr HandParser::parse(ITokenizerPtr tz) {
    tz_ = std::move(tz);
    try {
        return parseModule();
    } catch (const std::runtime_error& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return nullptr;
    }
}

Token HandParser::peek() { return tz_->peek(); }

Token HandParser::advance() { return tz_->next(); }

Token HandParser::expect(TokenType tt) {
    Token t = advance();
    if (t.type != tt) {
        error("Expected " + std::string(toString(tt)) +
              " but found " + std::string(toString(t.type)) +
              " ('" + t.text + "')");
    }
    return t;
}

bool HandParser::check(TokenType tt) { return peek().type == tt; }

bool HandParser::match(TokenType tt) {
    if (check(tt)) { advance(); return true; }
    return false;
}

void HandParser::error(const std::string& msg) {
    Token t = peek();
    throw std::runtime_error("Parse error at " + std::to_string(t.line) +
                             ":" + std::to_string(t.col) + ": " + msg);
}

ModulePtr HandParser::parseModule() {
    auto mod = std::make_unique<Module>();

    expect(TokenType::KW_MODULE);
    mod->name = expect(TokenType::Ident).text;
    expect(TokenType::Semicolon);

    if (check(TokenType::KW_IMPORT)) {
        mod->imports = parseImportList();
    }

    mod->decls = parseDeclarationSequence();

    if (match(TokenType::KW_BEGIN)) {
        mod->block = parseStatementSequence();
    }

    expect(TokenType::KW_END);
    mod->endName = expect(TokenType::Ident).text;
    expect(TokenType::Dot);

    return mod;
}

std::vector<Import> HandParser::parseImportList() {
    expect(TokenType::KW_IMPORT);
    std::vector<Import> imports;
    imports.push_back(parseImport());
    while (match(TokenType::Comma)) {
        imports.push_back(parseImport());
    }
    expect(TokenType::Semicolon);
    return imports;
}

Import HandParser::parseImport() {
    Import imp;
    std::string first = expect(TokenType::Ident).text;
    if (match(TokenType::Assign)) {
        imp.alias = first;
        imp.name = expect(TokenType::Ident).text;
    } else {
        imp.name = first;
    }
    return imp;
}

std::vector<DeclPtr> HandParser::parseDeclarationSequence() {
    std::vector<DeclPtr> decls;

    if (match(TokenType::KW_CONST)) {
        while (check(TokenType::Ident)) {
            decls.push_back(parseConstDecl());
            expect(TokenType::Semicolon);
        }
    }

    if (match(TokenType::KW_TYPE)) {
        while (check(TokenType::Ident)) {
            decls.push_back(parseTypeDecl());
            expect(TokenType::Semicolon);
        }
    }

    if (match(TokenType::KW_VAR)) {
        while (check(TokenType::Ident)) {
            decls.push_back(parseVarDecl());
            expect(TokenType::Semicolon);
        }
    }

    while (check(TokenType::KW_PROCEDURE)) {
        decls.push_back(parseProcDecl());
        expect(TokenType::Semicolon);
    }

    return decls;
}

DeclPtr HandParser::parseConstDecl() {
    auto d = std::make_unique<ConstDecl>();
    auto id = parseIdentDef();
    d->name = id.name;
    d->exported = id.exported;
    expect(TokenType::Eq);
    d->value = parseExpression();
    return d;
}

DeclPtr HandParser::parseTypeDecl() {
    auto d = std::make_unique<TypeDecl>();
    auto id = parseIdentDef();
    d->name = id.name;
    d->exported = id.exported;
    expect(TokenType::Eq);
    d->type = parseType();
    return d;
}

DeclPtr HandParser::parseVarDecl() {
    auto d = std::make_unique<VarDecl>();
    auto ids = parseIdentList();
    for (auto& id : ids) {
        d->names.push_back(id.name);
        d->exportedFlags.push_back(id.exported);
    }
    expect(TokenType::Colon);
    d->type = parseType();
    return d;
}

DeclPtr HandParser::parseProcDecl() {
    auto proc = std::make_unique<ProcDecl>();

    // ProcedureHeading
    expect(TokenType::KW_PROCEDURE);
    auto id = parseIdentDef();
    proc->name = id.name;
    proc->exported = id.exported;

    if (check(TokenType::LParen)) {
        proc->type = parseFormalParameters();
    }

    expect(TokenType::Semicolon);

    // ProcedureBody
    proc->decls = parseDeclarationSequence();

    if (match(TokenType::KW_BEGIN)) {
        proc->body = parseStatementSequence();
    }

    if (match(TokenType::KW_RETURN)) {
        proc->returnValue = parseExpression();
    }

    expect(TokenType::KW_END);
    Token endName = expect(TokenType::Ident);
    if (endName.text != proc->name) {
        error("Procedure name mismatch: expected '" + proc->name +
              "', but found '" + endName.text + "'");
    }

    return proc;
}

HandParser::IdentDef HandParser::parseIdentDef() {
    IdentDef id;
    id.name = expect(TokenType::Ident).text;
    id.exported = match(TokenType::Star);
    return id;
}

std::vector<HandParser::IdentDef> HandParser::parseIdentList() {
    std::vector<IdentDef> list;
    list.push_back(parseIdentDef());
    while (match(TokenType::Comma)) {
        list.push_back(parseIdentDef());
    }
    return list;
}

TypePtr HandParser::parseType() {
    if (check(TokenType::KW_ARRAY))   return parseArrayType();
    if (check(TokenType::KW_RECORD))  return parseRecordType();
    if (check(TokenType::KW_POINTER)) return parsePointerType();
    if (check(TokenType::KW_PROCEDURE)) return parseProcedureType();

    // qualident
    auto named = std::make_unique<NamedType>();
    std::string first = expect(TokenType::Ident).text;
    if (match(TokenType::Dot)) {
        named->name = first + "." + expect(TokenType::Ident).text;
    } else {
        named->name = first;
    }
    return named;
}

TypePtr HandParser::parseArrayType() {
    expect(TokenType::KW_ARRAY);
    auto arr = std::make_unique<ArrayType>();

    if (!check(TokenType::KW_OF)) {
        arr->length.push_back(parseExpression());
        while (match(TokenType::Comma)) {
            arr->length.push_back(parseExpression());
        }
    }

    expect(TokenType::KW_OF);
    arr->elemType = parseType();
    return arr;
}

TypePtr HandParser::parseRecordType() {
    expect(TokenType::KW_RECORD);
    auto rec = std::make_unique<RecordType>();

    if (match(TokenType::LParen)) {
        auto base = std::make_unique<NamedType>();
        std::string first = expect(TokenType::Ident).text;
        if (match(TokenType::Dot)) {
            base->name = first + "." + expect(TokenType::Ident).text;
        } else {
            base->name = first;
        }
        rec->baseType = std::move(base);
        expect(TokenType::RParen);
    }

    if (check(TokenType::Ident)) {
        // FieldListSequence
        auto parseFieldList = [this]() -> Ptr<FieldDecl> {
            auto field = std::make_unique<FieldDecl>();
            auto ids = parseIdentList();
            for (auto& id : ids) {
                field->names.push_back(id.name);
            }
            expect(TokenType::Colon);
            field->type = parseType();
            return field;
        };

        rec->fields.push_back(parseFieldList());
        while (match(TokenType::Semicolon)) {
            if (check(TokenType::KW_END)) break;
            rec->fields.push_back(parseFieldList());
        }
    }

    expect(TokenType::KW_END);
    return rec;
}

TypePtr HandParser::parsePointerType() {
    expect(TokenType::KW_POINTER);
    expect(TokenType::KW_TO);
    auto ptr = std::make_unique<PointerType>();
    ptr->baseType = parseType();
    return ptr;
}

TypePtr HandParser::parseProcedureType() {
    expect(TokenType::KW_PROCEDURE);
    if (check(TokenType::LParen)) {
        return parseFormalParameters();
    }
    return std::make_unique<ProcType>();
}

TypePtr HandParser::parseFormalParameters() {
    expect(TokenType::LParen);
    auto pt = std::make_unique<ProcType>();

    if (!check(TokenType::RParen)) {
        auto parseFPSection = [this]() -> Ptr<ProcParams> {
            auto pp = std::make_unique<ProcParams>();
            pp->isVar = match(TokenType::KW_VAR);

            pp->names.push_back(expect(TokenType::Ident).text);
            while (match(TokenType::Comma)) {
                pp->names.push_back(expect(TokenType::Ident).text);
            }

            expect(TokenType::Colon);
            pp->type = parseFormalType();
            return pp;
        };

        pt->params.push_back(parseFPSection());
        while (match(TokenType::Semicolon)) {
            pt->params.push_back(parseFPSection());
        }
    }

    expect(TokenType::RParen);

    if (match(TokenType::Colon)) {
        pt->type = std::make_unique<NamedType>();
        std::string first = expect(TokenType::Ident).text;
        if (match(TokenType::Dot)) {
            pt->type->name = first + "." + expect(TokenType::Ident).text;
        } else {
            pt->type->name = first;
        }
    }

    return pt;
}

TypePtr HandParser::parseFormalType() {
    if (match(TokenType::KW_ARRAY)) {
        expect(TokenType::KW_OF);
        auto arr = std::make_unique<ArrayType>();
        arr->elemType = parseFormalType();
        return arr;
    }
    auto named = std::make_unique<NamedType>();
    std::string first = expect(TokenType::Ident).text;
    if (match(TokenType::Dot)) {
        named->name = first + "." + expect(TokenType::Ident).text;
    } else {
        named->name = first;
    }
    return named;
}

// ─────────────────── expressions ───────────────────

ExprPtr HandParser::parseExpression() {
    auto lhs = parseSimpleExpression();

    BinaryExpr::Op op = BinaryExpr::Op::None;
    switch (peek().type) {
    case TokenType::Eq:    op = BinaryExpr::Op::Eq;  break;
    case TokenType::Neq:   op = BinaryExpr::Op::Neq; break;
    case TokenType::Lt:    op = BinaryExpr::Op::Lt;  break;
    case TokenType::Le:    op = BinaryExpr::Op::Le;  break;
    case TokenType::Gt:    op = BinaryExpr::Op::Gt;  break;
    case TokenType::Ge:    op = BinaryExpr::Op::Ge;  break;
    case TokenType::KW_IN: op = BinaryExpr::Op::In;  break;
    case TokenType::KW_IS: op = BinaryExpr::Op::Is;  break;
    default: return lhs;
    }

    advance();
    auto rhs = parseSimpleExpression();
    auto bin = std::make_unique<BinaryExpr>();
    bin->op = op;
    bin->lhs = std::move(lhs);
    bin->rhs = std::move(rhs);
    return bin;
}

ExprPtr HandParser::parseSimpleExpression() {
    bool neg = false;
    if (check(TokenType::Plus)) {
        advance();
    } else if (check(TokenType::Minus)) {
        advance();
        neg = true;
    }

    ExprPtr result = parseTerm();

    if (neg) {
        auto u = std::make_unique<UnaryExpr>();
        u->op = UnaryExpr::Op::Neg;
        u->rhs = std::move(result);
        result = std::move(u);
    }

    while (true) {
        BinaryExpr::Op op;
        switch (peek().type) {
        case TokenType::Plus:  op = BinaryExpr::Op::Add; break;
        case TokenType::Minus: op = BinaryExpr::Op::Sub; break;
        case TokenType::KW_OR: op = BinaryExpr::Op::Or;  break;
        default: return result;
        }
        advance();
        auto rhs = parseTerm();
        auto bin = std::make_unique<BinaryExpr>();
        bin->op = op;
        bin->lhs = std::move(result);
        bin->rhs = std::move(rhs);
        result = std::move(bin);
    }
}

ExprPtr HandParser::parseTerm() {
    auto result = parseFactor();

    while (true) {
        BinaryExpr::Op op;
        switch (peek().type) {
        case TokenType::Star:   op = BinaryExpr::Op::Mul;  break;
        case TokenType::Slash:  op = BinaryExpr::Op::RDiv; break;
        case TokenType::KW_DIV: op = BinaryExpr::Op::IDiv; break;
        case TokenType::KW_MOD: op = BinaryExpr::Op::Mod;  break;
        case TokenType::Amp:    op = BinaryExpr::Op::And;  break;
        default: return result;
        }
        advance();
        auto rhs = parseFactor();
        auto bin = std::make_unique<BinaryExpr>();
        bin->op = op;
        bin->lhs = std::move(result);
        bin->rhs = std::move(rhs);
        result = std::move(bin);
    }
}

ExprPtr HandParser::parseFactor() {
    Token t = peek();

    switch (t.type) {
    case TokenType::Integer: {
        advance();
        auto lit = std::make_unique<LiteralExpr>();
        lit->kind = LiteralExpr::Kind::Int;
        lit->intValue = t.intValue;
        return lit;
    }
    case TokenType::Real: {
        advance();
        auto lit = std::make_unique<LiteralExpr>();
        lit->kind = LiteralExpr::Kind::Real;
        lit->realValue = t.realValue;
        return lit;
    }
    case TokenType::String: {
        advance();
        auto lit = std::make_unique<LiteralExpr>();
        lit->kind = LiteralExpr::Kind::String;
        lit->strValue = t.text;
        return lit;
    }
    case TokenType::KW_NIL: {
        advance();
        auto lit = std::make_unique<LiteralExpr>();
        lit->kind = LiteralExpr::Kind::Nil;
        return lit;
    }
    case TokenType::KW_TRUE: {
        advance();
        auto lit = std::make_unique<LiteralExpr>();
        lit->kind = LiteralExpr::Kind::Bool;
        lit->boolValue = true;
        return lit;
    }
    case TokenType::KW_FALSE: {
        advance();
        auto lit = std::make_unique<LiteralExpr>();
        lit->kind = LiteralExpr::Kind::Bool;
        lit->boolValue = false;
        return lit;
    }
    case TokenType::LBrace: {
        advance();
        auto s = std::make_unique<SetExpr>();
        if (!check(TokenType::RBrace)) {
            auto parseElement = [this]() -> SetElement {
                SetElement el;
                el.low = parseExpression();
                if (match(TokenType::Range)) {
                    el.high = parseExpression();
                }
                return el;
            };
            s->elements.push_back(parseElement());
            while (match(TokenType::Comma)) {
                s->elements.push_back(parseElement());
            }
        }
        expect(TokenType::RBrace);
        return s;
    }
    case TokenType::LParen: {
        advance();
        auto e = parseExpression();
        expect(TokenType::RParen);
        return e;
    }
    case TokenType::Tilde: {
        advance();
        auto u = std::make_unique<UnaryExpr>();
        u->op = UnaryExpr::Op::Not;
        u->rhs = parseFactor();
        return u;
    }
    case TokenType::Ident: {
        return parseDesignator();
    }
    default:
        error("Expected expression, found " + std::string(toString(t.type)));
    }
}

// ─────────────────── designator ───────────────────

Ptr<DesignatorExpr> HandParser::parseDesignator() {
    auto des = std::make_unique<DesignatorExpr>();
    des->baseName = expect(TokenType::Ident).text;

    if (check(TokenType::Dot)) {
        Token dotTok = advance(); // consume '.'
        if (check(TokenType::Ident)) {
            Token secondIdent = advance();
            des->baseName += "." + secondIdent.text;
        } else {
            error("Expected identifier after '.'");
        }
    }

    while (true) {
        if (check(TokenType::Dot)) {
            advance();
            auto sel = std::make_unique<FieldSelector>();
            sel->name = expect(TokenType::Ident).text;
            des->selectors.push_back(std::move(sel));
        } else if (check(TokenType::LBrack)) {
            advance();
            auto sel = std::make_unique<IndexSelector>();
            sel->index.push_back(parseExpression());
            while (match(TokenType::Comma)) {
                sel->index.push_back(parseExpression());
            }
            expect(TokenType::RBrack);
            des->selectors.push_back(std::move(sel));
        } else if (check(TokenType::Caret)) {
            advance();
            des->selectors.push_back(std::make_unique<DerefSelector>());
        } else if (check(TokenType::LParen)) {
            advance();
            auto sel = std::make_unique<ArgsSelector>();
            if (!check(TokenType::RParen)) {
                sel->args.push_back(parseExpression());
                while (match(TokenType::Comma)) {
                    sel->args.push_back(parseExpression());
                }
            }
            expect(TokenType::RParen);
            des->selectors.push_back(std::move(sel));
        } else {
            break;
        }
    }

    return des;
}

std::vector<ExprPtr> HandParser::parseExprList() {
    std::vector<ExprPtr> list;
    list.push_back(parseExpression());
    while (match(TokenType::Comma)) {
        list.push_back(parseExpression());
    }
    return list;
}


std::vector<StmtPtr> HandParser::parseStatementSequence() {
    std::vector<StmtPtr> stmts;
    auto s = parseStatement();
    if (s) stmts.push_back(std::move(s));
    while (match(TokenType::Semicolon)) {
        s = parseStatement();
        if (s) stmts.push_back(std::move(s));
    }
    return stmts;
}

StmtPtr HandParser::parseStatement() {
    switch (peek().type) {
    case TokenType::KW_IF:     return parseIfStatement();
    case TokenType::KW_WHILE:  return parseWhileStatement();
    case TokenType::KW_REPEAT: return parseRepeatStatement();
    case TokenType::KW_FOR:    return parseForStatement();
    case TokenType::KW_CASE:   return parseCaseStatement();
    case TokenType::KW_RETURN: {
        advance();
        auto ret = std::make_unique<ReturnStmt>();
        if (!check(TokenType::Semicolon) && !check(TokenType::KW_END) &&
            !check(TokenType::KW_ELSE) && !check(TokenType::KW_ELSIF) &&
            !check(TokenType::KW_UNTIL) && !check(TokenType::Eof) &&
            !check(TokenType::Bar)) {
            ret->value = parseExpression();
        }
        return ret;
    }
    case TokenType::Ident: {
        auto des = parseDesignator();
        if (match(TokenType::Assign)) {
            auto stmt = std::make_unique<AssignStmt>();
            stmt->lhs = std::move(des);
            stmt->rhs = parseExpression();
            return stmt;
        } else {
            auto stmt = std::make_unique<CallStmt>();
            stmt->designator = std::move(des);
            return stmt;
        }
    }
    default:
        return nullptr;
    }
}

StmtPtr HandParser::parseIfStatement() {
    expect(TokenType::KW_IF);
    auto stmt = std::make_unique<IfStmt>();

    Branch first;
    first.cond = parseExpression();
    expect(TokenType::KW_THEN);
    first.body = parseStatementSequence();
    stmt->branches.push_back(std::move(first));

    while (match(TokenType::KW_ELSIF)) {
        Branch br;
        br.cond = parseExpression();
        expect(TokenType::KW_THEN);
        br.body = parseStatementSequence();
        stmt->branches.push_back(std::move(br));
    }

    if (match(TokenType::KW_ELSE)) {
        stmt->elseBody = parseStatementSequence();
    }

    expect(TokenType::KW_END);
    return stmt;
}

StmtPtr HandParser::parseWhileStatement() {
    expect(TokenType::KW_WHILE);
    auto stmt = std::make_unique<WhileStmt>();

    stmt->cond = parseExpression();
    expect(TokenType::KW_DO);
    stmt->body = parseStatementSequence();

    while (match(TokenType::KW_ELSIF)) {
        Branch br;
        br.cond = parseExpression();
        expect(TokenType::KW_DO);
        br.body = parseStatementSequence();
        stmt->branches.push_back(std::move(br));
    }

    expect(TokenType::KW_END);
    return stmt;
}

StmtPtr HandParser::parseRepeatStatement() {
    expect(TokenType::KW_REPEAT);
    auto stmt = std::make_unique<RepeatStmt>();
    stmt->body = parseStatementSequence();
    expect(TokenType::KW_UNTIL);
    stmt->untilCond = parseExpression();
    return stmt;
}

StmtPtr HandParser::parseForStatement() {
    expect(TokenType::KW_FOR);
    auto stmt = std::make_unique<ForStmt>();
    stmt->varName = expect(TokenType::Ident).text;
    expect(TokenType::Assign);
    stmt->from = parseExpression();
    expect(TokenType::KW_TO);
    stmt->to = parseExpression();
    if (match(TokenType::KW_BY)) {
        stmt->by = parseExpression();
    }
    expect(TokenType::KW_DO);
    stmt->body = parseStatementSequence();
    expect(TokenType::KW_END);
    return stmt;
}

StmtPtr HandParser::parseCaseStatement() {
    expect(TokenType::KW_CASE);
    auto stmt = std::make_unique<CaseStmt>();
    stmt->expr = parseExpression();
    expect(TokenType::KW_OF);

    auto parseCase = [this]() -> Ptr<CaseAlternative> {
        if (check(TokenType::KW_END) || check(TokenType::Bar))
            return nullptr;
        auto alt = std::make_unique<CaseAlternative>();

        auto parseLabel = [this]() -> Ptr<CaseLabel> {
            auto cl = std::make_unique<CaseLabel>();
            // label = integer | string | qualident
            if (check(TokenType::Integer)) {
                Token t = advance();
                auto lit = std::make_unique<LiteralExpr>();
                lit->kind = LiteralExpr::Kind::Int;
                lit->intValue = t.intValue;
                cl->from = std::move(lit);
            } else if (check(TokenType::String)) {
                Token t = advance();
                auto lit = std::make_unique<LiteralExpr>();
                lit->kind = LiteralExpr::Kind::String;
                lit->strValue = t.text;
                cl->from = std::move(lit);
            } else {
                auto des = std::make_unique<DesignatorExpr>();
                des->baseName = expect(TokenType::Ident).text;
                if (match(TokenType::Dot)) {
                    des->baseName += "." + expect(TokenType::Ident).text;
                }
                cl->from = std::move(des);
            }
            if (match(TokenType::Range)) {
                if (check(TokenType::Integer)) {
                    Token t = advance();
                    auto lit = std::make_unique<LiteralExpr>();
                    lit->kind = LiteralExpr::Kind::Int;
                    lit->intValue = t.intValue;
                    cl->to = std::move(lit);
                } else if (check(TokenType::String)) {
                    Token t = advance();
                    auto lit = std::make_unique<LiteralExpr>();
                    lit->kind = LiteralExpr::Kind::String;
                    lit->strValue = t.text;
                    cl->to = std::move(lit);
                } else {
                    auto des = std::make_unique<DesignatorExpr>();
                    des->baseName = expect(TokenType::Ident).text;
                    cl->to = std::move(des);
                }
            }
            return cl;
        };

        alt->labels.push_back(parseLabel());
        while (match(TokenType::Comma)) {
            alt->labels.push_back(parseLabel());
        }

        expect(TokenType::Colon);
        auto body = parseStatementSequence();
        for (auto& s : body) {
            alt->body.push_back(std::move(s));
        }
        return alt;
    };

    auto c = parseCase();
    if (c) stmt->alts.push_back(std::move(c));

    while (match(TokenType::Bar)) {
        c = parseCase();
        if (c) stmt->alts.push_back(std::move(c));
    }

    expect(TokenType::KW_END);
    return stmt;
}
