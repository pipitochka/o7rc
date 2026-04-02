#include "IRDotExporter.h"

std::string IRDotExporter::escape(const std::string& s) {
    std::string r;
    for (char c : s) {
        switch (c) {
            case '"':  r += "\\\""; break;
            case '\\': r += "\\\\"; break;
            case '<':  r += "\\<"; break;
            case '>':  r += "\\>"; break;
            case '{':  r += "\\{"; break;
            case '}':  r += "\\}"; break;
            case '|':  r += "\\|"; break;
            case '\n': r += "\\l"; break;
            default:   r += c;
        }
    }
    return r;
}

void IRDotExporter::exportFunction(const IRFunction& fn, std::ostream& out) {
    out << "digraph \"" << fn.name << "\" {\n";
    out << "  node [shape=record, fontname=\"Courier\", fontsize=10];\n";
    out << "  label=\"" << fn.name << "\";\n";
    out << "  labelloc=t;\n\n";

    for (auto& bb : fn.blocks) {
        if (bb->instrs.empty()) continue;

        out << "  bb" << bb->id << " [label=\"{" << escape(bb->label) << ":\\l";
        for (auto& instr : bb->instrs) {
            out << "  " << escape(printer_.formatInstr(instr)) << "\\l";
        }
        out << "}\"];\n";
    }

    out << "\n";

    for (auto& bb : fn.blocks) {
        if (bb->instrs.empty()) continue;
        for (int succ : bb->successors) {
            out << "  bb" << bb->id << " -> bb" << succ;
            if (!bb->instrs.empty() && bb->instrs.back().op == IROp::Branch) {
                if (succ == bb->instrs.back().targetBlock)
                    out << " [label=\"T\", color=green]";
                else
                    out << " [label=\"F\", color=red]";
            }
            out << ";\n";
        }
    }

    out << "}\n";
}

void IRDotExporter::exportModule(const IRModule& mod, std::ostream& out) {
    out << "digraph \"" << mod.name << "\" {\n";
    out << "  compound=true;\n";
    out << "  node [shape=record, fontname=\"Courier\", fontsize=10];\n\n";

    auto emitSubgraph = [&](const IRFunction& fn, const std::string& prefix) {
        out << "  subgraph cluster_" << prefix << " {\n";
        out << "    label=\"" << fn.name << "\";\n";
        out << "    style=dashed;\n";

        for (auto& bb : fn.blocks) {
            if (bb->instrs.empty()) continue;
            out << "    " << prefix << "_bb" << bb->id
                << " [label=\"{" << escape(bb->label) << ":\\l";
            for (auto& instr : bb->instrs) {
                out << "  " << escape(printer_.formatInstr(instr)) << "\\l";
            }
            out << "}\"];\n";
        }

        for (auto& bb : fn.blocks) {
            if (bb->instrs.empty()) continue;
            for (int succ : bb->successors) {
                out << "    " << prefix << "_bb" << bb->id
                    << " -> " << prefix << "_bb" << succ;
                if (!bb->instrs.empty() && bb->instrs.back().op == IROp::Branch) {
                    if (succ == bb->instrs.back().targetBlock)
                        out << " [label=\"T\", color=green]";
                    else
                        out << " [label=\"F\", color=red]";
                }
                out << ";\n";
            }
        }

        out << "  }\n\n";
    };

    for (size_t i = 0; i < mod.functions.size(); ++i) {
        emitSubgraph(mod.functions[i], "fn" + std::to_string(i));
    }
    emitSubgraph(mod.mainBody, "main");

    out << "}\n";
}
