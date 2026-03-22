#include "TypeInfo.h"

const TypeInfo::Field* TypeInfo::findField(const std::string& name) const {
    for (auto& f : fields)
        if (f.name == name) return &f;
    if (baseRecord)
        return baseRecord->findField(name);
    return nullptr;
}

TypeInfo* TypeRegistry::alloc(TypeInfo ti) {
    owned_.push_back(std::make_unique<TypeInfo>(std::move(ti)));
    return owned_.back().get();
}

TypeInfo* TypeRegistry::integerType() {
    if (!int_) { int_ = alloc({TypeInfo::TInteger, 4}); }
    return int_;
}

TypeInfo* TypeRegistry::booleanType() {
    if (!bool_) { bool_ = alloc({TypeInfo::TBoolean, 4}); }
    return bool_;
}

TypeInfo* TypeRegistry::realType() {
    if (!real_) { real_ = alloc({TypeInfo::TReal, 4}); }
    return real_;
}

TypeInfo* TypeRegistry::charType() {
    if (!char_) { char_ = alloc({TypeInfo::TChar, 4}); }
    return char_;
}

TypeInfo* TypeRegistry::setType() {
    if (!set_) { set_ = alloc({TypeInfo::TSet, 4}); }
    return set_;
}

TypeInfo* TypeRegistry::nilType() {
    if (!nil_) { nil_ = alloc({TypeInfo::TNil, 4}); }
    return nil_;
}

TypeInfo* TypeRegistry::voidType() {
    if (!void_) { void_ = alloc({TypeInfo::TVoid, 0}); }
    return void_;
}

TypeInfo* TypeRegistry::makeArray(int length, TypeInfo* elem) {
    TypeInfo ti;
    ti.kind = TypeInfo::TArray;
    ti.arrayLength = length;
    ti.elemType = elem;
    ti.size = length * elem->size;
    return alloc(std::move(ti));
}

TypeInfo* TypeRegistry::makeRecord(
        const std::vector<std::pair<std::string, TypeInfo*>>& fields,
        TypeInfo* base) {
    TypeInfo ti;
    ti.kind = TypeInfo::TRecord;
    ti.baseRecord = base;
    int offset = base ? base->size : 0;
    for (auto& [name, type] : fields) {
        ti.fields.push_back({name, offset, type});
        offset += type->size;
    }
    ti.size = offset;
    return alloc(std::move(ti));
}

TypeInfo* TypeRegistry::makePointer(TypeInfo* pointee) {
    TypeInfo ti;
    ti.kind = TypeInfo::TPointer;
    ti.pointeeType = pointee;
    ti.size = 4;
    return alloc(std::move(ti));
}

TypeInfo* TypeRegistry::resolveBuiltin(const std::string& name) {
    if (name == "INTEGER") return integerType();
    if (name == "BOOLEAN") return booleanType();
    if (name == "REAL") return realType();
    if (name == "CHAR") return charType();
    if (name == "SET") return setType();
    return nullptr;
}
