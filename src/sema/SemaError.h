#pragma once
#include <string>
#include <vector>
#include <util/ast/Node.h>

struct SemaError {
    enum Kind {
        UndefinedSymbol,
        DuplicateSymbol,
        TypeMismatch,
        ArityMismatch,
        MissingReturn,
        ModuleNameMismatch,
        InvalidAssignTarget,
        ConstAssign,
        NotCallable,
        Other
    };

    Kind kind;
    std::string message;
    SourcePos pos;

    std::string str() const {
        return std::to_string(pos.line) + ":" + std::to_string(pos.col)
               + ": " + message;
    }
};

using SemaErrors = std::vector<SemaError>;
