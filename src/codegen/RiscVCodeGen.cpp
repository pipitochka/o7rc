#include "RiscVCodeGen.h"
#include <module/ModuleLoader.h>
#include <stdexcept>
#include <cstring>

void RiscVCodeGen::generate(Module& module, std::ostream& out) {
    module.accept(*this);
    emit_.writeTo(out);
}

void RiscVCodeGen::generate(Module& module, std::ostream& out,
                            const std::vector<ModuleInfo>& imports) {
    sym_.enterScope();
    inGlobalScope_ = true;

    for (auto& mi : imports) {
        std::string savedModule = moduleName_;
        moduleName_ = mi.name;
        for (auto& d : mi.ast->decls) {
            if (auto* cd = dynamic_cast<ConstDecl*>(d.get())) {
                if (!cd->exported) continue;
                cd->accept(*this);
            } else if (auto* td = dynamic_cast<TypeDecl*>(d.get())) {
                if (!td->exported) continue;
                td->accept(*this);
            } else if (auto* vd = dynamic_cast<VarDecl*>(d.get())) {
                vd->accept(*this);
            } else if (auto* pd = dynamic_cast<ProcDecl*>(d.get())) {
                if (!pd->exported) continue;
                pd->accept(*this);
            }
        }
        moduleName_ = savedModule;
    }

    inGlobalScope_ = false;

    module.accept(*this);

    sym_.leaveScope();
    emit_.writeTo(out);
}

std::string RiscVCodeGen::globalVarLabel(const std::string& name) {
    return "_" + moduleName_ + "_" + name;
}

TypeInfo* RiscVCodeGen::resolveType(TypeNode* t) {
    if (!t) return types_.integerType();
    if (auto* named = dynamic_cast<NamedType*>(t)) {
        auto* bi = types_.resolveBuiltin(named->name);
        if (bi) return bi;
        auto it = namedTypes_.find(named->name);
        if (it != namedTypes_.end()) return it->second;
        return types_.integerType();
    }
    if (auto* arr = dynamic_cast<ArrayType*>(t)) {
        int length = 1;
        if (!arr->length.empty()) {
            if (auto* lit = dynamic_cast<LiteralExpr*>(arr->length[0].get()))
                length = static_cast<int>(lit->intValue);
        }
        return types_.makeArray(length, resolveType(arr->elemType.get()));
    }
    if (auto* ptr = dynamic_cast<PointerType*>(t)) {
        return types_.makePointer(resolveType(ptr->baseType.get()));
    }
    if (auto* rec = dynamic_cast<RecordType*>(t)) {
        std::vector<std::pair<std::string, TypeInfo*>> fields;
        for (auto& fd : rec->fields) {
            TypeInfo* fti = resolveType(fd->type.get());
            for (auto& fname : fd->names)
                fields.push_back({fname, fti});
        }
        TypeInfo* base = rec->baseType ? resolveType(rec->baseType.get()) : nullptr;
        return types_.makeRecord(fields, base);
    }
    return types_.integerType();
}

void RiscVCodeGen::emitStatements(const std::vector<StmtPtr>& stmts) {
    for (auto& s : stmts)
        if (s) s->accept(*this);
}

void RiscVCodeGen::visit(Module& mod) {
    moduleName_ = mod.name;

    // --- глобальная область ---
    sym_.enterScope();
    inGlobalScope_ = true;

    // Объявления: CONST, TYPE, VAR, PROCEDURE
    for (auto& d : mod.decls)
        d->accept(*this);

    inGlobalScope_ = false;

    // --- main ---
    emit_.blank();
    emit_.label("main");

    // Тело модуля (BEGIN ... END)
    emitStatements(mod.block);

    // Выход
    emit_.blank();
    emit_.comment("exit");
    emit_.text("li a7, 10");
    emit_.text("ecall");

    sym_.leaveScope();
}

void RiscVCodeGen::visit(Import&) {}
void RiscVCodeGen::visit(Block&) {}

void RiscVCodeGen::visit(ConstDecl& d) {
    Symbol s;
    s.kind = Symbol::Const;
    s.name = d.name;
    s.type = types_.integerType();
    if (auto* lit = dynamic_cast<LiteralExpr*>(d.value.get()))
        s.constValue = lit->intValue;
    s.isGlobal = inGlobalScope_;
    sym_.define(s);
}

void RiscVCodeGen::visit(TypeDecl& d) {
    TypeInfo* ti = resolveType(d.type.get());
    namedTypes_[d.name] = ti;
}

void RiscVCodeGen::visit(VarDecl& d) {
    TypeInfo* ti = resolveType(d.type.get());

    for (auto& name : d.names) {
        Symbol s;
        s.kind = Symbol::Var;
        s.name = name;
        s.type = ti;

        if (inGlobalScope_) {
            s.isGlobal = true;
            s.globalLabel = globalVarLabel(name);
            sym_.define(s);

            if (ti->kind == TypeInfo::TArray || ti->kind == TypeInfo::TRecord) {
                emit_.data(s.globalLabel + ": .space " + std::to_string(ti->size));
            } else {
                emit_.data(s.globalLabel + ": .word 0");
            }
        } else {
            s.isGlobal = false;
            s.stackOffset = sym_.allocLocal(ti->size);
            sym_.define(s);
        }
    }
}

void RiscVCodeGen::visit(ProcDecl& proc) {
    std::string procLabel = "proc_" + moduleName_ + "_" + proc.name;

    // Регистрируем символ процедуры в текущей области
    {
        Symbol ps;
        ps.kind = Symbol::Proc;
        ps.name = proc.name;
        ps.procLabel = procLabel;
        ps.type = types_.voidType();
        sym_.define(ps);
    }

    bool prevGlobal = inGlobalScope_;
    inGlobalScope_ = false;

    sym_.enterScope();
    sym_.allocLocal(8); // ra + s0

    // Параметры
    auto* sig = dynamic_cast<ProcType*>(proc.type.get());
    int paramRegIdx = 0; // a0..a7
    std::vector<std::pair<std::string, bool>> params; // name, isVar

    if (sig) {
        for (auto& section : sig->params) {
            TypeInfo* pti = resolveType(section->type.get());
            for (auto& pname : section->names) {
                int off = sym_.allocLocal(4);
                Symbol ps;
                ps.kind = Symbol::Param;
                ps.name = pname;
                ps.type = pti;
                ps.isGlobal = false;
                ps.stackOffset = off;
                ps.isVarParam = section->isVar;
                sym_.define(ps);
                params.push_back({pname, section->isVar});
            }
        }
    }

    // Локальные объявления процедуры
    for (auto& d : proc.decls)
        d->accept(*this);

    int frameSize = sym_.currentFrameSize();
    // Выравнивание до 16 байт
    frameSize = (frameSize + 15) & ~15;

    std::string epilogueLabel = emit_.freshLabel("L_epilogue");
    std::string savedEpilogue = currentEpilogueLabel_;
    currentEpilogueLabel_ = epilogueLabel;

    emit_.blank();
    emit_.label(procLabel);

    // Пролог
    emit_.text("addi sp, sp, -" + std::to_string(frameSize));
    emit_.text("sw ra, " + std::to_string(frameSize - 4) + "(sp)");
    emit_.text("sw s0, " + std::to_string(frameSize - 8) + "(sp)");
    emit_.text("addi s0, sp, " + std::to_string(frameSize));

    // Сохраняем параметры из регистров a0..a7 на стек
    paramRegIdx = 0;
    for (auto& [pname, isVar] : params) {
        if (paramRegIdx > 7) break;
        auto* ps = sym_.lookup(pname);
        if (ps) {
            emit_.text("sw a" + std::to_string(paramRegIdx) +
                       ", " + std::to_string(ps->stackOffset) + "(s0)");
        }
        paramRegIdx++;
    }

    // Тело
    emitStatements(proc.body);

    // RETURN expression (из ProcedureBody, если есть)
    if (proc.returnValue) {
        proc.returnValue->accept(*this);
    }

    // Эпилог
    emit_.label(epilogueLabel);
    emit_.text("lw ra, " + std::to_string(frameSize - 4) + "(sp)");
    emit_.text("lw s0, " + std::to_string(frameSize - 8) + "(sp)");
    emit_.text("addi sp, sp, " + std::to_string(frameSize));
    emit_.text("jr ra");

    currentEpilogueLabel_ = savedEpilogue;
    sym_.leaveScope();
    inGlobalScope_ = prevGlobal;
}

void RiscVCodeGen::visit(ParamDecl&) {}


void RiscVCodeGen::visit(NamedType&) {}
void RiscVCodeGen::visit(ArrayType&) {}
void RiscVCodeGen::visit(RecordType&) {}
void RiscVCodeGen::visit(PointerType&) {}
void RiscVCodeGen::visit(ProcParams&) {}
void RiscVCodeGen::visit(ProcType&) {}
void RiscVCodeGen::visit(FieldDecl&) {}

void RiscVCodeGen::visit(LiteralExpr& e) {
    switch (e.kind) {
        case LiteralExpr::Kind::Int:
            emit_.text("li a0, " + std::to_string(e.intValue));
            break;
        case LiteralExpr::Kind::Bool:
            emit_.text("li a0, " + std::to_string(e.boolValue ? 1 : 0));
            break;
        case LiteralExpr::Kind::Nil:
            emit_.text("li a0, 0");
            break;
        case LiteralExpr::Kind::Real: {
            // Загружаем float как целочисленный паттерн битов
            float f = static_cast<float>(e.realValue);
            int32_t bits;
            std::memcpy(&bits, &f, sizeof(bits));
            emit_.text("li a0, " + std::to_string(bits));
            break;
        }
        case LiteralExpr::Kind::String: {
            std::string lbl = emit_.freshLabel("_str");
            emit_.data(lbl + ": .asciz \"" + e.strValue + "\"");
            emit_.text("la a0, " + lbl);
            break;
        }
    }
}

void RiscVCodeGen::visit(UnaryExpr& e) {
    e.rhs->accept(*this); // a0 = operand
    switch (e.op) {
        case UnaryExpr::Op::Neg:
            emit_.text("neg a0, a0");
            break;
        case UnaryExpr::Op::Not:
            emit_.text("seqz a0, a0");
            break;
    }
}

void RiscVCodeGen::visit(BinaryExpr& e) {
    e.lhs->accept(*this);
    emit_.text("addi sp, sp, -4");
    emit_.text("sw a0, 0(sp)");
    e.rhs->accept(*this);
    emit_.text("mv t1, a0");
    emit_.text("lw t0, 0(sp)");
    emit_.text("addi sp, sp, 4");

    switch (e.op) {
        case BinaryExpr::Op::Add:  emit_.text("add a0, t0, t1"); break;
        case BinaryExpr::Op::Sub:  emit_.text("sub a0, t0, t1"); break;
        case BinaryExpr::Op::Mul:  emit_.text("mul a0, t0, t1"); break;
        case BinaryExpr::Op::IDiv: emit_.text("div a0, t0, t1"); break;
        case BinaryExpr::Op::RDiv: emit_.text("div a0, t0, t1"); break;
        case BinaryExpr::Op::Mod:  emit_.text("rem a0, t0, t1"); break;
        case BinaryExpr::Op::And:  emit_.text("and a0, t0, t1"); break;
        case BinaryExpr::Op::Or:   emit_.text("or a0, t0, t1"); break;

        case BinaryExpr::Op::Eq:
            emit_.text("sub a0, t0, t1");
            emit_.text("seqz a0, a0");
            break;
        case BinaryExpr::Op::Neq:
            emit_.text("sub a0, t0, t1");
            emit_.text("snez a0, a0");
            break;
        case BinaryExpr::Op::Lt:
            emit_.text("slt a0, t0, t1");
            break;
        case BinaryExpr::Op::Ge:
            emit_.text("slt a0, t0, t1");
            emit_.text("xori a0, a0, 1");
            break;
        case BinaryExpr::Op::Gt:
            emit_.text("slt a0, t1, t0");
            break;
        case BinaryExpr::Op::Le:
            emit_.text("slt a0, t1, t0");
            emit_.text("xori a0, a0, 1");
            break;

        case BinaryExpr::Op::In:
            // a IN b  →  (b >> a) & 1
            emit_.text("srl a0, t1, t0");
            emit_.text("andi a0, a0, 1");
            break;

        default:
            emit_.comment("unhandled binary op");
            emit_.text("li a0, 0");
            break;
    }
}

void RiscVCodeGen::visit(CallExpr& e) {
    // CallExpr не используется парсером напрямую (вызовы — через ArgsSelector).
    emit_.comment("CallExpr (unused path)");
}

void RiscVCodeGen::visit(DesignatorExpr& des) {
    // Проверяем встроенные
    if (tryEmitBuiltinCall(des))
        return;

    // Если есть ArgsSelector на конце — это вызов функции
    if (!des.selectors.empty()) {
        if (auto* args = dynamic_cast<ArgsSelector*>(des.selectors.back().get())) {
            // Вызов функции: определяем имя, кладём аргументы в a0..a7
            // Извлекаем имя: может быть «Module.Proc» → берём Proc
            std::string baseName = des.baseName;
            std::string::size_type dotPos = baseName.find('.');
            std::string procName = (dotPos != std::string::npos)
                ? baseName.substr(dotPos + 1) : baseName;

            // Аргументы
            for (int i = 0; i < static_cast<int>(args->args.size()) && i < 8; ++i) {
                args->args[i]->accept(*this); // a0 = arg value
                if (i > 0) {
                    emit_.text("mv a" + std::to_string(i) + ", a0");
                }
                if (i < static_cast<int>(args->args.size()) - 1) {
                    emit_.text("addi sp, sp, -4");
                    emit_.text("sw a0, 0(sp)");
                }
            }
            // Восстанавливаем аргументы из стека в регистры a0..aN-1
            int n = static_cast<int>(args->args.size());
            if (n > 1) {
                for (int i = n - 2; i >= 0; --i) {
                    emit_.text("lw a" + std::to_string(i) + ", 0(sp)");
                    emit_.text("addi sp, sp, 4");
                }
            }

            // Вызов
            auto* sym = sym_.lookup(procName);
            if (sym && sym->kind == Symbol::Proc) {
                emit_.text("jal ra, " + sym->procLabel);
            } else {
                emit_.text("jal ra, proc_" + moduleName_ + "_" + procName);
            }
            return;
        }
    }

    // Просто designator — загрузка значения
    emitLoad(des);
}

void RiscVCodeGen::visit(SetExpr& e) {
    emit_.text("li a0, 0"); // пустое множество
    for (auto& elem : e.elements) {
        if (elem.high) {
            // Диапазон low..high — генерируем маску
            elem.low->accept(*this);
            emit_.text("mv t2, a0");              // t2 = low
            emit_.text("addi sp, sp, -4");
            emit_.text("sw a0, 0(sp)");

            elem.high->accept(*this);
            emit_.text("mv t3, a0");              // t3 = high

            emit_.text("lw t2, 0(sp)");
            emit_.text("addi sp, sp, 4");

            // Цикл: for i = low to high: result |= (1 << i)
            // Используем простой подход с unroll'ом невозможен — цикл
            std::string loopLbl = emit_.freshLabel("L_setloop");
            std::string endLbl = emit_.freshLabel("L_setend");
            emit_.text("li t4, 0");               // t4 = accumulator
            emit_.label(loopLbl);
            emit_.text("bgt t2, t3, " + endLbl);
            emit_.text("li t5, 1");
            emit_.text("sll t5, t5, t2");
            emit_.text("or t4, t4, t5");
            emit_.text("addi t2, t2, 1");
            emit_.text("j " + loopLbl);
            emit_.label(endLbl);

            // OR с предыдущим результатом (на стеке)
            emit_.text("or a0, a0, t4");
        } else {
            // Одиночный элемент: result |= (1 << val)
            emit_.text("addi sp, sp, -4");
            emit_.text("sw a0, 0(sp)");

            elem.low->accept(*this);
            emit_.text("li t5, 1");
            emit_.text("sll t5, t5, a0");

            emit_.text("lw a0, 0(sp)");
            emit_.text("addi sp, sp, 4");
            emit_.text("or a0, a0, t5");
        }
    }
}

// ============================================================
//  Designator helpers
// ============================================================

void RiscVCodeGen::emitAddress(DesignatorExpr& des) {
    std::string baseName = des.baseName;
    std::string::size_type dotPos = baseName.find('.');
    std::string lookupName = (dotPos != std::string::npos)
        ? baseName.substr(dotPos + 1) : baseName;

    auto* sym = sym_.lookup(lookupName);
    if (!sym) sym = sym_.lookup(baseName);

    std::string implicitField;
    if (!sym && dotPos != std::string::npos) {
        std::string firstPart = baseName.substr(0, dotPos);
        sym = sym_.lookup(firstPart);
        if (sym) implicitField = baseName.substr(dotPos + 1);
    }

    if (!sym) {
        emit_.comment("ERROR: unknown symbol " + baseName);
        emit_.text("li a0, 0");
        return;
    }

    if (sym->kind == Symbol::Const) {
        emit_.text("li a0, " + std::to_string(sym->constValue));
        return;
    }

    if (sym->isGlobal) {
        emit_.text("la a0, " + sym->globalLabel);
    } else if (sym->isVarParam) {
        emit_.text("lw a0, " + std::to_string(sym->stackOffset) + "(s0)");
    } else {
        emit_.text("addi a0, s0, " + std::to_string(sym->stackOffset));
    }

    TypeInfo* curType = sym->type;

    if (!implicitField.empty()) {
        if (curType && curType->kind == TypeInfo::TPointer) {
            emit_.text("lw a0, 0(a0)");
            curType = curType->pointeeType;
        }
        if (curType && curType->kind == TypeInfo::TRecord) {
            auto* f = curType->findField(implicitField);
            if (f) {
                if (f->offset != 0)
                    emit_.text("addi a0, a0, " + std::to_string(f->offset));
                curType = f->type;
            }
        }
    }

    for (auto& sel : des.selectors) {
        if (dynamic_cast<ArgsSelector*>(sel.get())) break;

        if (auto* field = dynamic_cast<FieldSelector*>(sel.get())) {
            if (curType && curType->kind == TypeInfo::TPointer) {
                emit_.text("lw a0, 0(a0)");
                curType = curType->pointeeType;
            }
            if (curType && curType->kind == TypeInfo::TRecord) {
                auto* f = curType->findField(field->name);
                if (f) {
                    if (f->offset != 0)
                        emit_.text("addi a0, a0, " + std::to_string(f->offset));
                    curType = f->type;
                }
            }
        } else if (auto* idx = dynamic_cast<IndexSelector*>(sel.get())) {
            emit_.text("mv t3, a0");
            idx->index[0]->accept(*this);
            emit_.text("slli a0, a0, 2");
            emit_.text("add a0, t3, a0");
            if (curType && curType->kind == TypeInfo::TArray)
                curType = curType->elemType;
        } else if (dynamic_cast<DerefSelector*>(sel.get())) {
            emit_.text("lw a0, 0(a0)");
            if (curType && curType->kind == TypeInfo::TPointer)
                curType = curType->pointeeType;
        }
    }
}

void RiscVCodeGen::emitLoad(DesignatorExpr& des) {
    std::string baseName = des.baseName;
    std::string::size_type dotPos = baseName.find('.');
    std::string lookupName = (dotPos != std::string::npos)
        ? baseName.substr(dotPos + 1) : baseName;

    auto* sym = sym_.lookup(lookupName);
    if (!sym) sym = sym_.lookup(baseName);

    if (!sym && dotPos != std::string::npos) {
        sym = sym_.lookup(baseName.substr(0, dotPos));
    }

    if (sym && sym->kind == Symbol::Const && des.selectors.empty()) {
        emit_.text("li a0, " + std::to_string(sym->constValue));
        return;
    }

    emitAddress(des);
    emit_.text("lw a0, 0(a0)");
}

// ============================================================
//  Встроенные процедуры
// ============================================================

bool RiscVCodeGen::tryEmitBuiltinCall(DesignatorExpr& des) {
    if (des.selectors.empty()) return false;
    auto* args = dynamic_cast<ArgsSelector*>(des.selectors.back().get());
    if (!args) return false;

    std::string name = des.baseName;
    std::string::size_type dotPos = name.find('.');
    std::string module, proc;
    if (dotPos != std::string::npos) {
        module = name.substr(0, dotPos);
        proc = name.substr(dotPos + 1);
    } else {
        proc = name;
    }

    // Out.Int(value, width)  — печать целого числа
    if (module == "Out" && proc == "Int") {
        if (!args->args.empty()) {
            args->args[0]->accept(*this);
            emit_.text("li a7, 1");
            emit_.text("ecall");
        }
        return true;
    }

    // Out.Ln  — перевод строки
    if (module == "Out" && proc == "Ln") {
        emit_.text("li a0, 10"); // '\n'
        emit_.text("li a7, 11");
        emit_.text("ecall");
        return true;
    }

    // Out.String(s) / Out.Char(ch)
    if (module == "Out" && proc == "String") {
        if (!args->args.empty()) {
            args->args[0]->accept(*this);
            emit_.text("li a7, 4");
            emit_.text("ecall");
        }
        return true;
    }

    if (module == "Out" && proc == "Char") {
        if (!args->args.empty()) {
            args->args[0]->accept(*this);
            emit_.text("li a7, 11");
            emit_.text("ecall");
        }
        return true;
    }

    if (module == "In" && proc == "Open") {
        return true;
    }

    if (module == "In" && proc == "Int") {
        emit_.text("li a7, 5");
        emit_.text("ecall");
        // a0 = считанное значение, нужно сохранить в аргумент
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                emit_.text("mv t0, a0");
                emitAddress(*d);
                emit_.text("sw t0, 0(a0)");
            }
        }
        return true;
    }

    // INC(x) / INC(x, n)
    if (proc == "INC" && module.empty()) {
        if (!args->args.empty()) {
            auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get());
            if (d) {
                emitAddress(*d);
                emit_.text("mv t2, a0");
                emit_.text("lw t0, 0(t2)");
                if (args->args.size() > 1) {
                    args->args[1]->accept(*this);
                    emit_.text("add t0, t0, a0");
                } else {
                    emit_.text("addi t0, t0, 1");
                }
                emit_.text("sw t0, 0(t2)");
            }
        }
        return true;
    }

    // DEC(x) / DEC(x, n)
    if (proc == "DEC" && module.empty()) {
        if (!args->args.empty()) {
            auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get());
            if (d) {
                emitAddress(*d);
                emit_.text("mv t2, a0");
                emit_.text("lw t0, 0(t2)");
                if (args->args.size() > 1) {
                    args->args[1]->accept(*this);
                    emit_.text("sub t0, t0, a0");
                } else {
                    emit_.text("addi t0, t0, -1");
                }
                emit_.text("sw t0, 0(t2)");
            }
        }
        return true;
    }

    if (proc == "NEW" && module.empty()) {
        int allocSize = 4;
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                std::string lname = d->baseName;
                auto dotP = lname.find('.');
                if (dotP != std::string::npos) lname = lname.substr(dotP + 1);
                auto* s = sym_.lookup(lname);
                if (s && s->type && s->type->kind == TypeInfo::TPointer && s->type->pointeeType)
                    allocSize = s->type->pointeeType->size;
            }
        }
        if (allocSize < 4) allocSize = 4;
        emit_.text("li a0, " + std::to_string(allocSize));
        emit_.text("li a7, 9");
        emit_.text("ecall");
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                emit_.text("mv t0, a0");
                emitAddress(*d);
                emit_.text("sw t0, 0(a0)");
            }
        }
        return true;
    }

    // ODD(x)
    if (proc == "ODD" && module.empty()) {
        if (!args->args.empty()) {
            args->args[0]->accept(*this);
            emit_.text("andi a0, a0, 1");
        }
        return true;
    }

    // ABS(x)
    if (proc == "ABS" && module.empty()) {
        if (!args->args.empty()) {
            args->args[0]->accept(*this);
            emit_.text("srai t0, a0, 31");
            emit_.text("xor a0, a0, t0");
            emit_.text("sub a0, a0, t0");
        }
        return true;
    }

    // ORD(x), CHR(x) — noop, значение уже целое
    if ((proc == "ORD" || proc == "CHR") && module.empty()) {
        if (!args->args.empty())
            args->args[0]->accept(*this);
        return true;
    }

    // LEN(arr) — подстановка длины массива как константы
    if (proc == "LEN" && module.empty()) {
        if (!args->args.empty()) {
            if (auto* d = dynamic_cast<DesignatorExpr*>(args->args[0].get())) {
                auto* sym = sym_.lookup(d->baseName);
                if (sym && sym->type && sym->type->kind == TypeInfo::TArray) {
                    emit_.text("li a0, " + std::to_string(sym->type->arrayLength));
                    return true;
                }
            }
        }
        emit_.text("li a0, 0");
        return true;
    }

    return false;
}

// ============================================================
//  Selectors (обрабатываются внутри emitAddress)
// ============================================================

void RiscVCodeGen::visit(FieldSelector&) {}
void RiscVCodeGen::visit(IndexSelector&) {}
void RiscVCodeGen::visit(DerefSelector&) {}
void RiscVCodeGen::visit(TypeGuardSelector&) {}
void RiscVCodeGen::visit(ArgsSelector&) {}

// ============================================================
//  Statements
// ============================================================

void RiscVCodeGen::visit(AssignStmt& s) {
    s.rhs->accept(*this);           // a0 = value
    emit_.text("mv t0, a0");        // t0 = value (сохраняем)
    emitAddress(*s.lhs);            // a0 = address of lhs
    emit_.text("sw t0, 0(a0)");     // *lhs = value
}

void RiscVCodeGen::visit(CallStmt& s) {
    auto& des = *s.designator;

    // Обработка встроенных процедур без скобок (например Out.Ln)
    std::string name = des.baseName;
    std::string::size_type dotPos = name.find('.');
    std::string module, proc;
    if (dotPos != std::string::npos) {
        module = name.substr(0, dotPos);
        proc = name.substr(dotPos + 1);
    } else {
        proc = name;
    }

    if (module == "Out" && proc == "Ln") {
        emit_.text("li a0, 10");
        emit_.text("li a7, 11");
        emit_.text("ecall");
        return;
    }

    if (module == "In" && proc == "Open") {
        return;
    }

    des.accept(*this);
}

void RiscVCodeGen::visit(IfStmt& s) {
    std::string endLabel = emit_.freshLabel("L_endif");

    for (size_t i = 0; i < s.branches.size(); ++i) {
        std::string nextLabel = emit_.freshLabel("L_elif");
        auto& br = s.branches[i];

        br.cond->accept(*this);                    // a0 = condition
        emit_.text("beqz a0, " + nextLabel);       // if false → next

        emitStatements(br.body);
        emit_.text("j " + endLabel);

        emit_.label(nextLabel);
    }

    // ELSE
    if (!s.elseBody.empty()) {
        emitStatements(s.elseBody);
    }

    emit_.label(endLabel);
}

void RiscVCodeGen::visit(WhileStmt& s) {
    std::string loopLabel = emit_.freshLabel("L_while");
    std::string endLabel = emit_.freshLabel("L_endwhile");

    emit_.label(loopLabel);
    s.cond->accept(*this);
    emit_.text("beqz a0, " + endLabel);

    emitStatements(s.body);
    emit_.text("j " + loopLabel);

    emit_.label(endLabel);
}

void RiscVCodeGen::visit(RepeatStmt& s) {
    std::string loopLabel = emit_.freshLabel("L_repeat");

    emit_.label(loopLabel);
    emitStatements(s.body);

    s.untilCond->accept(*this);
    emit_.text("beqz a0, " + loopLabel);
}

void RiscVCodeGen::visit(ForStmt& s) {
    // for varName := from TO to BY by DO body END
    std::string loopLabel = emit_.freshLabel("L_for");
    std::string endLabel = emit_.freshLabel("L_endfor");

    // i := from
    s.from->accept(*this);                     // a0 = from
    auto* sym = sym_.lookup(s.varName);
    if (sym && sym->isGlobal) {
        emit_.text("la t2, " + sym->globalLabel);
        emit_.text("sw a0, 0(t2)");
    } else if (sym) {
        emit_.text("sw a0, " + std::to_string(sym->stackOffset) + "(s0)");
    }

    emit_.label(loopLabel);

    // Загрузка текущего значения i
    if (sym && sym->isGlobal) {
        emit_.text("la t2, " + sym->globalLabel);
        emit_.text("lw t0, 0(t2)");
    } else if (sym) {
        emit_.text("lw t0, " + std::to_string(sym->stackOffset) + "(s0)");
    }

    // Загрузка to
    s.to->accept(*this);                       // a0 = to
    emit_.text("bgt t0, a0, " + endLabel);     // if i > to → end

    emitStatements(s.body);

    // i := i + by
    if (sym && sym->isGlobal) {
        emit_.text("la t2, " + sym->globalLabel);
        emit_.text("lw t0, 0(t2)");
    } else if (sym) {
        emit_.text("lw t0, " + std::to_string(sym->stackOffset) + "(s0)");
    }

    if (s.by) {
        s.by->accept(*this);
        emit_.text("add t0, t0, a0");
    } else {
        emit_.text("addi t0, t0, 1");
    }

    if (sym && sym->isGlobal) {
        emit_.text("la t2, " + sym->globalLabel);
        emit_.text("sw t0, 0(t2)");
    } else if (sym) {
        emit_.text("sw t0, " + std::to_string(sym->stackOffset) + "(s0)");
    }

    emit_.text("j " + loopLabel);
    emit_.label(endLabel);
}

void RiscVCodeGen::visit(ReturnStmt& s) {
    if (s.value) {
        s.value->accept(*this); // a0 = return value
    }
    emit_.text("j " + currentEpilogueLabel_);
}

void RiscVCodeGen::visit(CaseStmt& s) {
    std::string endLabel = emit_.freshLabel("L_endcase");

    s.expr->accept(*this);                       // a0 = case expr
    emit_.text("mv t6, a0");                     // t6 = сохраняем значение

    for (auto& alt : s.alts) {
        if (!alt) continue;

        std::string bodyLabel = emit_.freshLabel("L_casebody");
        std::string nextLabel = emit_.freshLabel("L_casenext");

        for (auto& lbl : alt->labels) {
            if (lbl->to) {
                // Диапазон from..to
                lbl->from->accept(*this);
                emit_.text("blt t6, a0, " + nextLabel);
                lbl->to->accept(*this);
                emit_.text("bgt t6, a0, " + nextLabel);
            } else {
                lbl->from->accept(*this);
                emit_.text("beq t6, a0, " + bodyLabel);
            }
        }
        emit_.text("j " + nextLabel);

        emit_.label(bodyLabel);
        for (auto& stmt : alt->body)
            if (stmt) stmt->accept(*this);
        emit_.text("j " + endLabel);

        emit_.label(nextLabel);
    }

    emit_.label(endLabel);
}

void RiscVCodeGen::visit(CaseAlternative&) {}
void RiscVCodeGen::visit(CaseLabel&) {}
