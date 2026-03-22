#include "Emitter.h"

std::string Emitter::freshLabel(const std::string& prefix) {
    return prefix + "_" + std::to_string(labelCounter_++);
}

void Emitter::data(const std::string& line) {
    dataSection_ << line << "\n";
}

void Emitter::text(const std::string& instr) {
    textSection_ << "    " << instr << "\n";
}

void Emitter::label(const std::string& lbl) {
    textSection_ << lbl << ":\n";
}

void Emitter::comment(const std::string& msg) {
    textSection_ << "    # " << msg << "\n";
}

void Emitter::blank() {
    textSection_ << "\n";
}

void Emitter::writeTo(std::ostream& out) const {
    std::string d = dataSection_.str();
    if (!d.empty()) {
        out << ".data\n" << d << "\n";
    }
    out << ".text\n";
    out << ".globl main\n";
    out << textSection_.str();
}
