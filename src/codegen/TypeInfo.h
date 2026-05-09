#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

/// Описание типа для кодогенерации (RV32: все базовые = 4 байта).
struct TypeInfo {
    enum Kind { TInteger, TBoolean, TReal, TChar, TSet, TArray, TRecord, TPointer, TProcedure, TNil, TVoid };

    Kind kind = TVoid;
    int size = 0;

    // ARRAY
    int arrayLength = 0;
    TypeInfo* elemType = nullptr;

    // RECORD
    struct Field {
        std::string name;
        int offset;
        TypeInfo* type;
    };
    std::vector<Field> fields;
    TypeInfo* baseRecord = nullptr;

    // POINTER
    TypeInfo* pointeeType = nullptr;

    const Field* findField(const std::string& name) const;
};

/// Реестр типов: создаёт и хранит все TypeInfo, вычисляет размеры.
class TypeRegistry {
public:
    TypeInfo* integerType();
    TypeInfo* booleanType();
    TypeInfo* realType();
    TypeInfo* charType();
    /// Однобайтовый CHAR для элементов ARRAY n OF CHAR (совместимость с In.Line / Out.String и RARS).
    TypeInfo* char8Type();
    TypeInfo* setType();
    TypeInfo* nilType();
    TypeInfo* voidType();

    TypeInfo* makeArray(int length, TypeInfo* elem);
    TypeInfo* makeRecord(const std::vector<std::pair<std::string, TypeInfo*>>& fields,
                         TypeInfo* base = nullptr);
    TypeInfo* makePointer(TypeInfo* pointee);

    /// Поиск по имени (INTEGER, BOOLEAN, REAL, CHAR, SET).
    TypeInfo* resolveBuiltin(const std::string& name);

private:
    std::vector<std::unique_ptr<TypeInfo>> owned_;
    TypeInfo* alloc(TypeInfo ti);

    TypeInfo* int_ = nullptr;
    TypeInfo* bool_ = nullptr;
    TypeInfo* real_ = nullptr;
    TypeInfo* char_ = nullptr;
    TypeInfo* char8_ = nullptr;
    TypeInfo* set_ = nullptr;
    TypeInfo* nil_ = nullptr;
    TypeInfo* void_ = nullptr;
};
