#include "IRBuilder.h"
#include <stdexcept>

IRModule IRBuilder::build(Module& module) {
    module_ = IRModule{};
    module.accept(*this);
    return std::move(module_);
}

// ── Scope management ──

void IRBuilder::pushScope() {
    scopes_.emplace_back();
}

void IRBuilder::popScope() {
    if (!scopes_.empty()) scopes_.pop_back();
}

IRBuilder::VarInfo* IRBuilder::lookupVar(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

// ── Helpers ──

void IRBuilder::emit(IRInstr instr) {
    if (curBlock_) curBlock_->instrs.push_back(std::move(instr));
}

void IRBuilder::setBlock(BasicBlock* bb) {
    curBlock_ = bb;
}

void IRBuilder::finishBlock(IRInstr terminator) {
    if (!curBlock_) return;
    if (curBlock_->hasTerminator()) return;
    emit(std::move(terminator));
}

IRValue IRBuilder::emitExpr(Expr& e) {
    e.accept(*this);
    return lastVal_;
}

void IRBuilder::emitStatements(const std::vector<StmtPtr>& stmts) {
    for (auto& s : stmts)
        if (s) s->accept(*this);
}

int IRBuilder::typeSize(TypeNode* t) {
    if (!t) return 4;
    if (auto* named = dynamic_cast<NamedType*>(t)) {
        auto it = typeLayouts_.find(named->name);
        if (it != typeLayouts_.end()) return it->second.size;
        return 4;
    }
    if (auto* arr = dynamic_cast<ArrayType*>(t)) {
        int len = arrayLength(t);
        return len * typeSize(arr->elemType.get());
    }
    if (auto* rec = dynamic_cast<RecordType*>(t)) {
        int total = 0;
        for (auto& fd : rec->fields) {
            int fSize = typeSize(fd->type.get());
            total += static_cast<int>(fd->names.size()) * fSize;
        }
        return total > 0 ? total : 4;
    }
    if (dynamic_cast<PointerType*>(t)) return 4;
    return 4;
}

std::string IRBuilder::resolveTypeName(TypeNode* t) {
    if (!t) return "";
    if (auto* named = dynamic_cast<NamedType*>(t))
        return named->name;
    if (auto* ptr = dynamic_cast<PointerType*>(t)) {
        if (auto* base = dynamic_cast<NamedType*>(ptr->baseType.get()))
            return "PTR:" + base->name;
    }
    return "";
}

void IRBuilder::registerTypeLayout(const std::string& name, TypeNode* t) {
    if (auto* rec = dynamic_cast<RecordType*>(t)) {
        TypeLayout layout;
        layout.kind = TypeLayout::Record;
        int offset = 0;
        for (auto& fd : rec->fields) {
            int fSize = typeSize(fd->type.get());
            std::string fTypeName = resolveTypeName(fd->type.get());
            for (auto& fname : fd->names) {
                layout.fields.push_back({fname, offset, fSize, fTypeName});
                offset += fSize;
            }
        }
        layout.size = offset > 0 ? offset : 4;
        typeLayouts_[name] = layout;
    } else if (auto* ptr = dynamic_cast<PointerType*>(t)) {
        TypeLayout layout;
        layout.kind = TypeLayout::Pointer;
        layout.size = 4;
        if (auto* base = dynamic_cast<NamedType*>(ptr->baseType.get()))
            layout.pointeeName = base->name;
        typeLayouts_[name] = layout;
        typeLayouts_["PTR:" + layout.pointeeName] = layout;
    }
}

int IRBuilder::arrayLength(TypeNode* t) {
    if (auto* arr = dynamic_cast<ArrayType*>(t)) {
        if (!arr->length.empty()) {
            if (auto* lit = dynamic_cast<LiteralExpr*>(arr->length[0].get()))
                return static_cast<int>(lit->intValue);
        }
    }
    return 1;
}

// ── Module ──

void IRBuilder::visit(Module& mod) {
    moduleName_ = mod.name;
    module_.name = mod.name;

    pushScope();
    inGlobalScope_ = true;

    for (auto& d : mod.decls)
        d->accept(*this);

    inGlobalScope_ = false;

    module_.mainBody.name = "main";
    curFunc_ = &module_.mainBody;
    auto* entry = curFunc_->createBlock("entry");
    setBlock(entry);

    emitStatements(mod.block);

    IRInstr ret;
    ret.op = IROp::Ret;
    ret.src1 = IRValue::voidVal();
    finishBlock(std::move(ret));

    curFunc_ = nullptr;
    popScope();
}

void IRBuilder::visit(Import&) {}
void IRBuilder::visit(Block&) {}

// ── Declarations ──

void IRBuilder::visit(ConstDecl& d) {
    int64_t val = 0;
    if (auto* lit = dynamic_cast<LiteralExpr*>(d.value.get()))
        val = lit->intValue;
    constants_[d.name] = {val};
}

void IRBuilder::visit(TypeDecl& d) {
    registerTypeLayout(d.name, d.type.get());
}

void IRBuilder::visit(VarDecl& d) {
    int size = typeSize(d.type.get());
    int arrLen = arrayLength(d.type.get());
    std::string tName = resolveTypeName(d.type.get());

    if (auto* ptr = dynamic_cast<PointerType*>(d.type.get())) {
        if (typeLayouts_.find(tName) == typeLayouts_.end())
            registerTypeLayout(tName, d.type.get());
    }

    for (auto& name : d.names) {
        if (inGlobalScope_) {
            std::string label = "_" + moduleName_ + "_" + name;
            IRGlobal g;
            g.name = name;
            g.label = label;
            g.size = size;
            g.isArray = (dynamic_cast<ArrayType*>(d.type.get()) != nullptr) ||
                        (dynamic_cast<RecordType*>(d.type.get()) != nullptr) ||
                        (size > 4);
            module_.globals.push_back(g);

            VarInfo vi;
            vi.isGlobal = true;
            vi.globalLabel = label;
            vi.arrayLen = arrLen;
            vi.typeName = tName;
            scopes_.back()[name] = vi;
        } else {
            IRValue addr = curFunc_->freshTemp();

            IRInstr alloca;
            alloca.op = IROp::Alloca;
            alloca.dst = addr;
            alloca.name = name;
            alloca.src1 = IRValue::constant(size);
            emit(alloca);

            VarInfo vi;
            vi.addr = addr;
            vi.isGlobal = false;
            vi.arrayLen = arrLen;
            vi.typeName = tName;
            curFunc_->locals[name] = {addr.id, size, false};
            scopes_.back()[name] = vi;
        }
    }
}

void IRBuilder::visit(ProcDecl& proc) {
    {
        VarInfo vi;
        vi.isGlobal = true;
        vi.globalLabel = "proc_" + moduleName_ + "_" + proc.name;
        scopes_.back()[proc.name] = vi;
    }

    IRFunction fn;
    fn.name = proc.name;

    auto* sig = dynamic_cast<ProcType*>(proc.type.get());
    fn.hasReturn = (sig && sig->type && !sig->type->name.empty());

    auto* savedFunc = curFunc_;
    auto* savedBlock = curBlock_;
    bool savedGlobal = inGlobalScope_;

    module_.functions.push_back(std::move(fn));
    curFunc_ = &module_.functions.back();
    inGlobalScope_ = false;

    auto* entry = curFunc_->createBlock("entry");
    setBlock(entry);

    pushScope();

    if (sig) {
        int paramIdx = 0;
        for (auto& section : sig->params) {
            for (auto& pname : section->names) {
                curFunc_->params.push_back(pname);
                curFunc_->varParams.push_back(section->isVar);

                IRValue paramVal = IRValue::param(paramIdx);
                IRValue addr = curFunc_->freshTemp();

                IRInstr alloca;
                alloca.op = IROp::Alloca;
                alloca.dst = addr;
                alloca.name = pname;
                alloca.src1 = IRValue::constant(4);
                emit(alloca);

                IRInstr store;
                store.op = IROp::StoreLocal;
                store.src1 = addr;
                store.src2 = paramVal;
                emit(store);

                VarInfo vi;
                vi.addr = addr;
                vi.isGlobal = false;
                vi.isVarParam = section->isVar;
                vi.typeName = resolveTypeName(section->type.get());
                scopes_.back()[pname] = vi;
                curFunc_->locals[pname] = {addr.id, 4, section->isVar};

                paramIdx++;
            }
        }
    }

    for (auto& d : proc.decls)
        d->accept(*this);

    emitStatements(proc.body);

    if (proc.returnValue) {
        IRValue rv = emitExpr(*proc.returnValue);
        IRInstr ret;
        ret.op = IROp::Ret;
        ret.src1 = rv;
        finishBlock(std::move(ret));
    } else {
        IRInstr ret;
        ret.op = IROp::Ret;
        ret.src1 = IRValue::voidVal();
        finishBlock(std::move(ret));
    }

    popScope();

    curFunc_ = savedFunc;
    curBlock_ = savedBlock;
    inGlobalScope_ = savedGlobal;
}

void IRBuilder::visit(ParamDecl&) {}

// ── Types (no-op at IR level) ──

void IRBuilder::visit(NamedType&) {}
void IRBuilder::visit(ArrayType&) {}
void IRBuilder::visit(RecordType&) {}
void IRBuilder::visit(PointerType&) {}
void IRBuilder::visit(ProcParams&) {}
void IRBuilder::visit(ProcType&) {}
void IRBuilder::visit(FieldDecl&) {}

// ── Expressions ──

void IRBuilder::visit(LiteralExpr& e) {
    switch (e.kind) {
        case LiteralExpr::Kind::Int:
            lastVal_ = IRValue::constant(e.intValue);
            break;
        case LiteralExpr::Kind::Bool:
            lastVal_ = IRValue::constant(e.boolValue ? 1 : 0);
            break;
        case LiteralExpr::Kind::Nil:
            lastVal_ = IRValue::constant(0);
            break;
        case LiteralExpr::Kind::String: {
            std::string lbl = "_str_" + std::to_string(module_.globals.size());
            IRGlobal g;
            g.name = lbl;
            g.label = lbl;
            g.isString = true;
            g.stringLiteral = e.strValue;
            module_.globals.push_back(g);

            IRValue dst = curFunc_->freshTemp();
            IRInstr instr;
            instr.op = IROp::AddrGlobal;
            instr.dst = dst;
            instr.name = lbl;
            emit(instr);
            lastVal_ = dst;
            break;
        }
        case LiteralExpr::Kind::Real: {
            float f = static_cast<float>(e.realValue);
            int32_t bits;
            std::memcpy(&bits, &f, sizeof(bits));
            lastVal_ = IRValue::constant(bits);
            break;
        }
    }
}

void IRBuilder::visit(UnaryExpr& e) {
    IRValue rhs = emitExpr(*e.rhs);
    IRValue dst = curFunc_->freshTemp();

    IRInstr instr;
    instr.dst = dst;
    instr.src1 = rhs;

    switch (e.op) {
        case UnaryExpr::Op::Neg: instr.op = IROp::Neg; break;
        case UnaryExpr::Op::Not: instr.op = IROp::Not; break;
    }

    emit(instr);
    lastVal_ = dst;
}

void IRBuilder::visit(BinaryExpr& e) {
    IRValue lhs = emitExpr(*e.lhs);
    IRValue rhs = emitExpr(*e.rhs);
    IRValue dst = curFunc_->freshTemp();

    IRInstr instr;
    instr.dst = dst;
    instr.src1 = lhs;
    instr.src2 = rhs;

    switch (e.op) {
        case BinaryExpr::Op::Add:  instr.op = IROp::Add; break;
        case BinaryExpr::Op::Sub:  instr.op = IROp::Sub; break;
        case BinaryExpr::Op::Mul:  instr.op = IROp::Mul; break;
        case BinaryExpr::Op::IDiv: instr.op = IROp::Div; break;
        case BinaryExpr::Op::RDiv: instr.op = IROp::Div; break;
        case BinaryExpr::Op::Mod:  instr.op = IROp::Mod; break;
        case BinaryExpr::Op::And:  instr.op = IROp::And; break;
        case BinaryExpr::Op::Or:   instr.op = IROp::Or;  break;
        case BinaryExpr::Op::Eq:   instr.op = IROp::Eq;  break;
        case BinaryExpr::Op::Neq:  instr.op = IROp::Neq; break;
        case BinaryExpr::Op::Lt:   instr.op = IROp::Lt;  break;
        case BinaryExpr::Op::Le:   instr.op = IROp::Le;  break;
        case BinaryExpr::Op::Gt:   instr.op = IROp::Gt;  break;
        case BinaryExpr::Op::Ge:   instr.op = IROp::Ge;  break;
        case BinaryExpr::Op::In:   instr.op = IROp::And; break;
        default:                   instr.op = IROp::Add; break;
    }

    emit(instr);
    lastVal_ = dst;
}

void IRBuilder::visit(CallExpr&) {
    lastVal_ = IRValue::constant(0);
}

void IRBuilder::visit(SetExpr&) {
    lastVal_ = IRValue::constant(0);
}

// ── Designator ──

IRValue IRBuilder::emitAddress(DesignatorExpr& des) {
    std::string baseName = des.baseName;
    std::string::size_type dotPos = baseName.find('.');
    std::string lookupName = (dotPos != std::string::npos)
        ? baseName.substr(dotPos + 1) : baseName;

    auto* vi = lookupVar(lookupName);
    if (!vi) vi = lookupVar(baseName);

    std::string implicitField;
    if (!vi && dotPos != std::string::npos) {
        std::string firstPart = baseName.substr(0, dotPos);
        vi = lookupVar(firstPart);
        if (vi) implicitField = baseName.substr(dotPos + 1);
    }

    if (!vi) {
        IRValue dst = curFunc_->freshTemp();
        IRInstr instr;
        instr.op = IROp::AddrGlobal;
        instr.dst = dst;
        instr.name = baseName;
        emit(instr);
        return dst;
    }

    IRValue addr;
    if (vi->isGlobal) {
        addr = curFunc_->freshTemp();
        IRInstr instr;
        instr.op = IROp::AddrGlobal;
        instr.dst = addr;
        instr.name = vi->globalLabel;
        emit(instr);
    } else if (vi->isVarParam) {
        addr = curFunc_->freshTemp();
        IRInstr load;
        load.op = IROp::LoadLocal;
        load.dst = addr;
        load.src1 = vi->addr;
        emit(load);
    } else {
        addr = vi->addr;
    }

    std::string curTypeName = vi ? vi->typeName : "";

    if (!implicitField.empty()) {
        auto tit = typeLayouts_.find(curTypeName);
        if (tit != typeLayouts_.end() && tit->second.kind == TypeLayout::Pointer) {
            IRValue newAddr = curFunc_->freshTemp();
            IRInstr load;
            load.op = IROp::Load;
            load.dst = newAddr;
            load.src1 = addr;
            emit(load);
            addr = newAddr;
            curTypeName = tit->second.pointeeName;
            tit = typeLayouts_.find(curTypeName);
        }
        if (tit != typeLayouts_.end() && tit->second.kind == TypeLayout::Record) {
            for (auto& f : tit->second.fields) {
                if (f.name == implicitField) {
                    if (f.offset != 0) {
                        IRValue offConst = IRValue::constant(f.offset);
                        IRValue newAddr = curFunc_->freshTemp();
                        IRInstr add;
                        add.op = IROp::Add;
                        add.dst = newAddr;
                        add.src1 = addr;
                        add.src2 = offConst;
                        emit(add);
                        addr = newAddr;
                    }
                    curTypeName = f.typeName;
                    break;
                }
            }
        }
    }

    for (auto& sel : des.selectors) {
        if (dynamic_cast<ArgsSelector*>(sel.get())) break;

        if (auto* field = dynamic_cast<FieldSelector*>(sel.get())) {
            auto tit = typeLayouts_.find(curTypeName);
            if (tit != typeLayouts_.end() && tit->second.kind == TypeLayout::Pointer) {
                IRValue newAddr = curFunc_->freshTemp();
                IRInstr load;
                load.op = IROp::Load;
                load.dst = newAddr;
                load.src1 = addr;
                emit(load);
                addr = newAddr;
                curTypeName = tit->second.pointeeName;
                tit = typeLayouts_.find(curTypeName);
            }
            if (tit != typeLayouts_.end() && tit->second.kind == TypeLayout::Record) {
                for (auto& f : tit->second.fields) {
                    if (f.name == field->name) {
                        if (f.offset != 0) {
                            IRValue offConst = IRValue::constant(f.offset);
                            IRValue newAddr = curFunc_->freshTemp();
                            IRInstr add;
                            add.op = IROp::Add;
                            add.dst = newAddr;
                            add.src1 = addr;
                            add.src2 = offConst;
                            emit(add);
                            addr = newAddr;
                        }
                        curTypeName = f.typeName;
                        break;
                    }
                }
            }
        } else if (auto* idx = dynamic_cast<IndexSelector*>(sel.get())) {
            if (!idx->index.empty()) {
                IRValue index = emitExpr(*idx->index[0]);
                IRValue newAddr = curFunc_->freshTemp();
                IRInstr instr;
                instr.op = IROp::Index;
                instr.dst = newAddr;
                instr.src1 = addr;
                instr.src2 = index;
                emit(instr);
                addr = newAddr;
            }
        } else if (dynamic_cast<DerefSelector*>(sel.get())) {
            IRValue newAddr = curFunc_->freshTemp();
            IRInstr load;
            load.op = IROp::Load;
            load.dst = newAddr;
            load.src1 = addr;
            emit(load);
            addr = newAddr;
            auto tit = typeLayouts_.find(curTypeName);
            if (tit != typeLayouts_.end() && tit->second.kind == TypeLayout::Pointer)
                curTypeName = tit->second.pointeeName;
        }
    }

    return addr;
}

IRValue IRBuilder::emitLoad(DesignatorExpr& des) {
    std::string baseName = des.baseName;
    std::string::size_type dotPos = baseName.find('.');
    std::string lookupName = (dotPos != std::string::npos)
        ? baseName.substr(dotPos + 1) : baseName;

    auto cit = constants_.find(lookupName);
    if (cit == constants_.end()) cit = constants_.find(baseName);

    bool isFieldAccess = false;
    if (dotPos != std::string::npos && cit == constants_.end()) {
        auto* vi = lookupVar(baseName.substr(0, dotPos));
        if (vi) isFieldAccess = true;
    }

    if (cit != constants_.end() && des.selectors.empty() && !isFieldAccess) {
        lastVal_ = IRValue::constant(cit->second.value);
        return lastVal_;
    }

    IRValue addr = emitAddress(des);
    IRValue dst = curFunc_->freshTemp();
    IRInstr load;
    load.op = IROp::Load;
    load.dst = dst;
    load.src1 = addr;
    emit(load);
    return dst;
}

void IRBuilder::visit(DesignatorExpr& des) {
    if (tryEmitBuiltin(des)) return;

    if (!des.selectors.empty()) {
        if (auto* args = dynamic_cast<ArgsSelector*>(des.selectors.back().get())) {
            std::string baseName = des.baseName;
            std::string::size_type dotPos = baseName.find('.');
            std::string procName = (dotPos != std::string::npos)
                ? baseName.substr(dotPos + 1) : baseName;

            std::vector<IRValue> callArgs;
            for (auto& a : args->args)
                callArgs.push_back(emitExpr(*a));

            IRValue dst = curFunc_->freshTemp();
            IRInstr call;
            call.op = IROp::Call;
            call.dst = dst;
            call.name = procName;
            call.args = std::move(callArgs);
            emit(call);
            lastVal_ = dst;
            return;
        }
    }

    lastVal_ = emitLoad(des);
}

bool IRBuilder::tryEmitBuiltin(DesignatorExpr& des) {
    if (des.selectors.empty()) return false;
    auto* args = dynamic_cast<ArgsSelector*>(des.selectors.back().get());
    if (!args) return false;

    std::string name = des.baseName;
    std::string::size_type dotPos = name.find('.');
    std::string mod, proc;
    if (dotPos != std::string::npos) {
        mod = name.substr(0, dotPos);
        proc = name.substr(dotPos + 1);
    } else {
        proc = name;
    }

    if (mod == "Out" && proc == "Int") {
        if (!args->args.empty()) {
            IRValue v = emitExpr(*args->args[0]);
            IRInstr sc;
            sc.op = IROp::Syscall;
            sc.syscallNum = 1;
            sc.args = {v};
            emit(sc);
        }
        lastVal_ = IRValue::voidVal();
        return true;
    }

    if (mod == "Out" && proc == "Ln") {
        IRInstr sc;
        sc.op = IROp::Syscall;
        sc.syscallNum = 11;
        sc.args = {IRValue::constant(10)};
        emit(sc);
        lastVal_ = IRValue::voidVal();
        return true;
    }

    if (mod == "Out" && (proc == "String" || proc == "Char")) {
        int scNum = (proc == "String") ? 4 : 11;
        if (!args->args.empty()) {
            IRValue v = emitExpr(*args->args[0]);
            IRInstr sc;
            sc.op = IROp::Syscall;
            sc.syscallNum = scNum;
            sc.args = {v};
            emit(sc);
        }
        lastVal_ = IRValue::voidVal();
        return true;
    }

    if (mod == "In" && proc == "Int") {
        IRValue dst = curFunc_->freshTemp();
        IRInstr sc;
        sc.op = IROp::Syscall;
        sc.syscallNum = 5;
        sc.dst = dst;
        emit(sc);
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                IRValue addr = emitAddress(*d);
                IRInstr store;
                store.op = IROp::Store;
                store.src1 = addr;
                store.src2 = dst;
                emit(store);
            }
        }
        lastVal_ = dst;
        return true;
    }

    if (proc == "INC" && mod.empty()) {
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                IRValue addr = emitAddress(*d);
                IRValue cur = curFunc_->freshTemp();
                IRInstr load;
                load.op = IROp::Load;
                load.dst = cur;
                load.src1 = addr;
                emit(load);

                IRValue inc = (args->args.size() > 1) ? emitExpr(*args->args[1]) : IRValue::constant(1);
                IRValue result = curFunc_->freshTemp();
                IRInstr add;
                add.op = IROp::Add;
                add.dst = result;
                add.src1 = cur;
                add.src2 = inc;
                emit(add);

                IRInstr store;
                store.op = IROp::Store;
                store.src1 = addr;
                store.src2 = result;
                emit(store);
            }
        }
        lastVal_ = IRValue::voidVal();
        return true;
    }

    if (proc == "DEC" && mod.empty()) {
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                IRValue addr = emitAddress(*d);
                IRValue cur = curFunc_->freshTemp();
                IRInstr load;
                load.op = IROp::Load;
                load.dst = cur;
                load.src1 = addr;
                emit(load);

                IRValue dec = (args->args.size() > 1) ? emitExpr(*args->args[1]) : IRValue::constant(1);
                IRValue result = curFunc_->freshTemp();
                IRInstr sub;
                sub.op = IROp::Sub;
                sub.dst = result;
                sub.src1 = cur;
                sub.src2 = dec;
                emit(sub);

                IRInstr store;
                store.op = IROp::Store;
                store.src1 = addr;
                store.src2 = result;
                emit(store);
            }
        }
        lastVal_ = IRValue::voidVal();
        return true;
    }

    if (proc == "NEW" && mod.empty()) {
        int allocSize = 4;
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                auto* vi = lookupVar(d->baseName);
                if (vi) {
                    auto tit = typeLayouts_.find(vi->typeName);
                    if (tit != typeLayouts_.end() && tit->second.kind == TypeLayout::Pointer) {
                        auto pit = typeLayouts_.find(tit->second.pointeeName);
                        if (pit != typeLayouts_.end())
                            allocSize = pit->second.size;
                    }
                }
            }
        }
        if (allocSize < 4) allocSize = 4;
        IRValue dst = curFunc_->freshTemp();
        IRInstr sc;
        sc.op = IROp::Syscall;
        sc.syscallNum = 9;
        sc.args = {IRValue::constant(allocSize)};
        sc.dst = dst;
        emit(sc);
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                IRValue addr = emitAddress(*d);
                IRInstr store;
                store.op = IROp::Store;
                store.src1 = addr;
                store.src2 = dst;
                emit(store);
            }
        }
        lastVal_ = dst;
        return true;
    }

    if (proc == "ODD" && mod.empty()) {
        if (!args->args.empty()) {
            IRValue v = emitExpr(*args->args[0]);
            IRValue dst = curFunc_->freshTemp();
            IRInstr instr;
            instr.op = IROp::And;
            instr.dst = dst;
            instr.src1 = v;
            instr.src2 = IRValue::constant(1);
            emit(instr);
            lastVal_ = dst;
        }
        return true;
    }

    if (proc == "ABS" && mod.empty()) {
        if (!args->args.empty()) {
            IRValue v = emitExpr(*args->args[0]);
            IRValue neg = curFunc_->freshTemp();
            IRInstr n;
            n.op = IROp::Neg;
            n.dst = neg;
            n.src1 = v;
            emit(n);

            IRValue cmp = curFunc_->freshTemp();
            IRInstr c;
            c.op = IROp::Lt;
            c.dst = cmp;
            c.src1 = v;
            c.src2 = IRValue::constant(0);
            emit(c);

            auto* thenBB = curFunc_->createBlock();
            auto* elseBB = curFunc_->createBlock();
            auto* mergeBB = curFunc_->createBlock();

            IRInstr br;
            br.op = IROp::Branch;
            br.src1 = cmp;
            br.targetBlock = thenBB->id;
            br.falseBlock = elseBB->id;
            finishBlock(std::move(br));
            curFunc_->linkBlocks(curBlock_->id, thenBB->id);
            curFunc_->linkBlocks(curBlock_->id, elseBB->id);

            IRValue result = curFunc_->freshTemp();

            setBlock(thenBB);
            IRInstr copyNeg;
            copyNeg.op = IROp::Copy;
            copyNeg.dst = result;
            copyNeg.src1 = neg;
            emit(copyNeg);
            IRInstr j1;
            j1.op = IROp::Jump;
            j1.targetBlock = mergeBB->id;
            finishBlock(std::move(j1));
            curFunc_->linkBlocks(thenBB->id, mergeBB->id);

            setBlock(elseBB);
            IRInstr copyPos;
            copyPos.op = IROp::Copy;
            copyPos.dst = result;
            copyPos.src1 = v;
            emit(copyPos);
            IRInstr j2;
            j2.op = IROp::Jump;
            j2.targetBlock = mergeBB->id;
            finishBlock(std::move(j2));
            curFunc_->linkBlocks(elseBB->id, mergeBB->id);

            setBlock(mergeBB);
            lastVal_ = result;
        }
        return true;
    }

    if ((proc == "ORD" || proc == "CHR") && mod.empty()) {
        if (!args->args.empty())
            lastVal_ = emitExpr(*args->args[0]);
        return true;
    }

    if (proc == "LEN" && mod.empty()) {
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                auto* vi = lookupVar(d->baseName);
                if (vi && vi->arrayLen > 0) {
                    lastVal_ = IRValue::constant(vi->arrayLen);
                    return true;
                }
            }
        }
        lastVal_ = IRValue::constant(0);
        return true;
    }

    return false;
}

// ── Selectors (handled inside emitAddress) ──

void IRBuilder::visit(FieldSelector&) {}
void IRBuilder::visit(IndexSelector&) {}
void IRBuilder::visit(DerefSelector&) {}
void IRBuilder::visit(TypeGuardSelector&) {}
void IRBuilder::visit(ArgsSelector&) {}
void IRBuilder::visit(CaseAlternative&) {}
void IRBuilder::visit(CaseLabel&) {}

// ── Statements ──

void IRBuilder::visit(AssignStmt& s) {
    IRValue val = emitExpr(*s.rhs);
    IRValue addr = emitAddress(*s.lhs);

    IRInstr store;
    store.op = IROp::Store;
    store.src1 = addr;
    store.src2 = val;
    emit(store);
}

void IRBuilder::visit(CallStmt& s) {
    auto& des = *s.designator;

    std::string name = des.baseName;
    std::string::size_type dotPos = name.find('.');
    std::string mod, proc;
    if (dotPos != std::string::npos) {
        mod = name.substr(0, dotPos);
        proc = name.substr(dotPos + 1);
    } else {
        proc = name;
    }

    if (mod == "Out" && proc == "Ln") {
        IRInstr sc;
        sc.op = IROp::Syscall;
        sc.syscallNum = 11;
        sc.args = {IRValue::constant(10)};
        emit(sc);
        return;
    }

    des.accept(*this);
}

void IRBuilder::visit(IfStmt& s) {
    auto* mergeBB = curFunc_->createBlock();

    for (size_t i = 0; i < s.branches.size(); ++i) {
        auto& br = s.branches[i];
        IRValue cond = emitExpr(*br.cond);

        auto* thenBB = curFunc_->createBlock();
        auto* elseBB = curFunc_->createBlock();

        IRInstr branch;
        branch.op = IROp::Branch;
        branch.src1 = cond;
        branch.targetBlock = thenBB->id;
        branch.falseBlock = elseBB->id;
        finishBlock(std::move(branch));
        curFunc_->linkBlocks(curBlock_->id, thenBB->id);
        curFunc_->linkBlocks(curBlock_->id, elseBB->id);

        setBlock(thenBB);
        emitStatements(br.body);
        IRInstr jmp;
        jmp.op = IROp::Jump;
        jmp.targetBlock = mergeBB->id;
        finishBlock(std::move(jmp));
        curFunc_->linkBlocks(curBlock_->id, mergeBB->id);

        setBlock(elseBB);
    }

    if (!s.elseBody.empty()) {
        emitStatements(s.elseBody);
    }

    IRInstr jmp;
    jmp.op = IROp::Jump;
    jmp.targetBlock = mergeBB->id;
    finishBlock(std::move(jmp));
    curFunc_->linkBlocks(curBlock_->id, mergeBB->id);

    setBlock(mergeBB);
}

void IRBuilder::visit(WhileStmt& s) {
    auto* headerBB = curFunc_->createBlock();
    auto* bodyBB = curFunc_->createBlock();
    auto* exitBB = curFunc_->createBlock();

    IRInstr jmpHeader;
    jmpHeader.op = IROp::Jump;
    jmpHeader.targetBlock = headerBB->id;
    finishBlock(std::move(jmpHeader));
    curFunc_->linkBlocks(curBlock_->id, headerBB->id);

    setBlock(headerBB);
    IRValue cond = emitExpr(*s.cond);

    IRInstr branch;
    branch.op = IROp::Branch;
    branch.src1 = cond;
    branch.targetBlock = bodyBB->id;
    branch.falseBlock = exitBB->id;
    finishBlock(std::move(branch));
    curFunc_->linkBlocks(headerBB->id, bodyBB->id);
    curFunc_->linkBlocks(headerBB->id, exitBB->id);

    setBlock(bodyBB);
    emitStatements(s.body);

    IRInstr jmpBack;
    jmpBack.op = IROp::Jump;
    jmpBack.targetBlock = headerBB->id;
    finishBlock(std::move(jmpBack));
    curFunc_->linkBlocks(curBlock_->id, headerBB->id);

    setBlock(exitBB);
}

void IRBuilder::visit(RepeatStmt& s) {
    auto* bodyBB = curFunc_->createBlock();
    auto* exitBB = curFunc_->createBlock();

    IRInstr jmpBody;
    jmpBody.op = IROp::Jump;
    jmpBody.targetBlock = bodyBB->id;
    finishBlock(std::move(jmpBody));
    curFunc_->linkBlocks(curBlock_->id, bodyBB->id);

    setBlock(bodyBB);
    emitStatements(s.body);

    IRValue cond = emitExpr(*s.untilCond);

    IRInstr branch;
    branch.op = IROp::Branch;
    branch.src1 = cond;
    branch.targetBlock = exitBB->id;
    branch.falseBlock = bodyBB->id;
    finishBlock(std::move(branch));
    curFunc_->linkBlocks(curBlock_->id, exitBB->id);
    curFunc_->linkBlocks(curBlock_->id, bodyBB->id);

    setBlock(exitBB);
}

void IRBuilder::visit(ForStmt& s) {
    IRValue from = emitExpr(*s.from);

    auto* vi = lookupVar(s.varName);
    if (vi) {
        IRValue addr = vi->isGlobal ? [&]{
            IRValue a = curFunc_->freshTemp();
            IRInstr ig;
            ig.op = IROp::AddrGlobal;
            ig.dst = a;
            ig.name = vi->globalLabel;
            emit(ig);
            return a;
        }() : vi->addr;

        IRInstr store;
        store.op = IROp::Store;
        store.src1 = addr;
        store.src2 = from;
        emit(store);
    }

    auto* headerBB = curFunc_->createBlock();
    auto* bodyBB = curFunc_->createBlock();
    auto* exitBB = curFunc_->createBlock();

    IRInstr jmpHeader;
    jmpHeader.op = IROp::Jump;
    jmpHeader.targetBlock = headerBB->id;
    finishBlock(std::move(jmpHeader));
    curFunc_->linkBlocks(curBlock_->id, headerBB->id);

    setBlock(headerBB);

    IRValue curVal;
    if (vi) {
        IRValue addr = vi->isGlobal ? [&]{
            IRValue a = curFunc_->freshTemp();
            IRInstr ig;
            ig.op = IROp::AddrGlobal;
            ig.dst = a;
            ig.name = vi->globalLabel;
            emit(ig);
            return a;
        }() : vi->addr;

        curVal = curFunc_->freshTemp();
        IRInstr load;
        load.op = IROp::Load;
        load.dst = curVal;
        load.src1 = addr;
        emit(load);
    } else {
        curVal = IRValue::constant(0);
    }

    IRValue toVal = emitExpr(*s.to);
    IRValue cmp = curFunc_->freshTemp();
    IRInstr cmpI;
    cmpI.op = IROp::Gt;
    cmpI.dst = cmp;
    cmpI.src1 = curVal;
    cmpI.src2 = toVal;
    emit(cmpI);

    IRInstr branch;
    branch.op = IROp::Branch;
    branch.src1 = cmp;
    branch.targetBlock = exitBB->id;
    branch.falseBlock = bodyBB->id;
    finishBlock(std::move(branch));
    curFunc_->linkBlocks(headerBB->id, exitBB->id);
    curFunc_->linkBlocks(headerBB->id, bodyBB->id);

    setBlock(bodyBB);
    emitStatements(s.body);

    if (vi) {
        IRValue addr = vi->isGlobal ? [&]{
            IRValue a = curFunc_->freshTemp();
            IRInstr ig;
            ig.op = IROp::AddrGlobal;
            ig.dst = a;
            ig.name = vi->globalLabel;
            emit(ig);
            return a;
        }() : vi->addr;

        IRValue cur2 = curFunc_->freshTemp();
        IRInstr load2;
        load2.op = IROp::Load;
        load2.dst = cur2;
        load2.src1 = addr;
        emit(load2);

        IRValue step = s.by ? emitExpr(*s.by) : IRValue::constant(1);
        IRValue next = curFunc_->freshTemp();
        IRInstr addI;
        addI.op = IROp::Add;
        addI.dst = next;
        addI.src1 = cur2;
        addI.src2 = step;
        emit(addI);

        IRInstr storeI;
        storeI.op = IROp::Store;
        storeI.src1 = addr;
        storeI.src2 = next;
        emit(storeI);
    }

    IRInstr jmpBack;
    jmpBack.op = IROp::Jump;
    jmpBack.targetBlock = headerBB->id;
    finishBlock(std::move(jmpBack));
    curFunc_->linkBlocks(curBlock_->id, headerBB->id);

    setBlock(exitBB);
}

void IRBuilder::visit(ReturnStmt& s) {
    IRInstr ret;
    ret.op = IROp::Ret;
    if (s.value) {
        ret.src1 = emitExpr(*s.value);
    } else {
        ret.src1 = IRValue::voidVal();
    }
    finishBlock(std::move(ret));

    auto* deadBB = curFunc_->createBlock();
    setBlock(deadBB);
}

void IRBuilder::visit(CaseStmt& s) {
    IRValue expr = emitExpr(*s.expr);
    auto* mergeBB = curFunc_->createBlock();

    for (auto& alt : s.alts) {
        if (!alt) continue;

        auto* bodyBB = curFunc_->createBlock();
        auto* nextBB = curFunc_->createBlock();

        for (auto& lbl : alt->labels) {
            if (lbl->from) {
                IRValue labelVal = emitExpr(*lbl->from);
                IRValue cmp = curFunc_->freshTemp();
                IRInstr eq;
                eq.op = IROp::Eq;
                eq.dst = cmp;
                eq.src1 = expr;
                eq.src2 = labelVal;
                emit(eq);

                IRInstr br;
                br.op = IROp::Branch;
                br.src1 = cmp;
                br.targetBlock = bodyBB->id;
                br.falseBlock = nextBB->id;
                finishBlock(std::move(br));
                curFunc_->linkBlocks(curBlock_->id, bodyBB->id);
                curFunc_->linkBlocks(curBlock_->id, nextBB->id);

                auto* contBB = curFunc_->createBlock();
                setBlock(contBB);
                nextBB = contBB;
            }
        }

        IRInstr jmpNext;
        jmpNext.op = IROp::Jump;
        jmpNext.targetBlock = nextBB->id;
        finishBlock(std::move(jmpNext));

        setBlock(bodyBB);
        for (auto& stmt : alt->body)
            if (stmt) stmt->accept(*this);
        IRInstr jmpMerge;
        jmpMerge.op = IROp::Jump;
        jmpMerge.targetBlock = mergeBB->id;
        finishBlock(std::move(jmpMerge));
        curFunc_->linkBlocks(curBlock_->id, mergeBB->id);

        setBlock(nextBB);
    }

    IRInstr jmpMerge;
    jmpMerge.op = IROp::Jump;
    jmpMerge.targetBlock = mergeBB->id;
    finishBlock(std::move(jmpMerge));
    curFunc_->linkBlocks(curBlock_->id, mergeBB->id);

    setBlock(mergeBB);
}
