#pragma once
#include <string>
#include <cstdint>

enum class TokenType : std::uint16_t {
    // служебные
    Eof = 0,
    Unknown,

    // литералы и идентификаторы
    Ident,
    Integer,
    Real,
    Char,
    String,

    // ключевые слова Oberon-7 (с префиксом KW_ чтобы не конфликтовать с flex BEGIN и т.п.)
    KW_ARRAY, KW_BEGIN, KW_BY, KW_CASE, KW_CONST, KW_DIV, KW_DO, KW_ELSE, KW_ELSIF, KW_END,
    KW_EXIT, KW_FALSE, KW_FOR, KW_IF, KW_IMPORT, KW_IN, KW_IS, KW_LOOP, KW_MOD, KW_MODULE,
    KW_NIL, KW_OF, KW_OR, KW_POINTER, KW_PROCEDURE, KW_RECORD, KW_REPEAT, KW_RETURN,
    KW_THEN, KW_TO, KW_TRUE, KW_TYPE, KW_UNTIL, KW_VAR, KW_WHILE, KW_WITH,

    // операторы/знаки
    Plus, Minus, Star, Slash,
    Eq, Neq, Lt, Le, Gt, Ge,
    Assign,
    Caret, Amp, Tilde, Bar,
    Range,

    // пунктуация
    LParen, RParen,
    LBrack, RBrack,
    LBrace, RBrace,
    Comma, Semicolon, Colon, Dot
};

struct Token {
    TokenType type = TokenType::Unknown;
    std::string text;

    std::uint32_t line = 1;
    std::uint32_t col  = 1;

    std::int64_t intValue = 0;
    double realValue = 0.0;
};

inline const char* toString(TokenType t) {
    switch (t) {
        case TokenType::Eof: return "Eof";
        case TokenType::Unknown: return "Unknown";
        case TokenType::Ident: return "Ident";
        case TokenType::Integer: return "Integer";
        case TokenType::Real: return "Real";
        case TokenType::Char: return "Char";
        case TokenType::String: return "String";

        case TokenType::KW_MODULE: return "KW_MODULE";
        case TokenType::KW_BEGIN:  return "KW_BEGIN";
        case TokenType::KW_END:    return "KW_END";
        default: return "Other";
    }
}
