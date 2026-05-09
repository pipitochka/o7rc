#include "StdlibProc.h"

namespace o7rc::runtime {

StdlibProcKind classifyStdlibProc(std::string_view module, std::string_view proc) {
    if (module == "Out") {
        if (proc == "Int") return StdlibProcKind::Out_Int;
        if (proc == "Ln") return StdlibProcKind::Out_Ln;
        if (proc == "String") return StdlibProcKind::Out_String;
        if (proc == "Char") return StdlibProcKind::Out_Char;
        if (proc == "Real") return StdlibProcKind::Out_Real;
        return StdlibProcKind::None;
    }
    if (module == "In") {
        if (proc == "Open") return StdlibProcKind::In_Open;
        if (proc == "Int") return StdlibProcKind::In_Int;
        if (proc == "Char") return StdlibProcKind::In_Char;
        if (proc == "Line") return StdlibProcKind::In_Line;
        return StdlibProcKind::None;
    }
    return StdlibProcKind::None;
}

const std::unordered_set<std::string>& stdlibQualifiedProcNames() {
    static const std::unordered_set<std::string> kNames = {
        "Out.Int", "Out.Ln", "Out.String", "Out.Char", "Out.Real",
        "In.Open", "In.Int", "In.Char", "In.Line",
    };
    return kNames;
}

} // namespace o7rc::runtime
