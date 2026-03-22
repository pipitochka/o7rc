#pragma once
#include <ostream>
#include <sstream>
#include <string>

/// Буфер генерации RISC-V ассемблера.
/// Разделяет вывод на секции .data и .text, управляет метками.
class Emitter {
public:
    /// Генерирует уникальную метку с заданным префиксом.
    std::string freshLabel(const std::string& prefix = "L");

    void data(const std::string& line);
    void text(const std::string& instr);
    void label(const std::string& lbl);
    void comment(const std::string& msg);
    void blank();

    /// Записывает итоговый .asm в поток: .data затем .text.
    void writeTo(std::ostream& out) const;

private:
    std::ostringstream dataSection_;
    std::ostringstream textSection_;
    int labelCounter_ = 0;
};
