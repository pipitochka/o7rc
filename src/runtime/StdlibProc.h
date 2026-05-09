#pragma once

#include <string>
#include <string_view>
#include <unordered_set>

namespace o7rc::runtime {

/// Идентификаторы процедур стандартных модулей ввода/вывода (реализация — в RiscVStdlibAsm / IRStdlib).
enum class StdlibProcKind : uint8_t {
    None = 0,
    Out_Int,
    Out_Ln,
    Out_String,
    Out_Char,
    Out_Real,
    In_Open,
    In_Int,
    In_Char,
    In_Line,
};

StdlibProcKind classifyStdlibProc(std::string_view module, std::string_view proc);

/// Полные имена вида "Out.Int" — считаются известными вызовами для Sema (наряду с INC, ABS, …).
const std::unordered_set<std::string>& stdlibQualifiedProcNames();

} // namespace o7rc::runtime
