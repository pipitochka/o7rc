#pragma once
#include <cstdint>
#include <string>

struct IRValue {
    enum Kind { Temp, Const, Param, Void };

    Kind kind = Void;
    int id = -1;
    int64_t constVal = 0;

    bool operator==(const IRValue& o) const {
        return kind == o.kind && id == o.id && constVal == o.constVal;
    }
    bool operator!=(const IRValue& o) const { return !(*this == o); }

    bool isVoid() const { return kind == Void; }
    bool isConst() const { return kind == Const; }
    bool isTemp() const { return kind == Temp; }
    bool isParam() const { return kind == Param; }

    static IRValue temp(int id) { return {Temp, id, 0}; }
    static IRValue constant(int64_t v) { return {Const, -1, v}; }
    static IRValue param(int index) { return {Param, index, 0}; }
    static IRValue voidVal() { return {Void, -1, 0}; }

    std::string str() const {
        switch (kind) {
            case Temp:  return "%" + std::to_string(id);
            case Const: return std::to_string(constVal);
            case Param: return "%p" + std::to_string(id);
            case Void:  return "void";
        }
        return "?";
    }
};
