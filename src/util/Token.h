#pragma once
#include <cstdint>
#include <string>
#include <iostream>


enum class TokenType : std::uint16_t {
    // special
    Eof = 0,
    Unknown,

    //
    Ident,
    Integer,
    Real,
    String,
    // Char - not sure

    // operators
    Plus,
    Minus,
    Star,
    Slash,
    Eq,
    Neq,
    Lt,
    Le,
    Gt,
    Ge,
    Assign,
    Caret,
    Amp,
    Tilde,
    Bar,
    Range,

    // punctuation
    LParen,
    RParen,
    LBrack,
    RBrack,
    LBrace,
    RBrace,
    Comma,
    Semicolon,
    Colon,
    Dot,

    KW_ARRAY,
    KW_BEGIN,
    KW_BY,
    KW_CASE,
    KW_CONST,
    KW_DIV,
    KW_DO,
    KW_ELSE,
    KW_ELSIF,
    KW_END,
    KW_FALSE,
    KW_FOR,
    KW_IF,
    KW_IMPORT,
    KW_IN,
    KW_IS,
    KW_MOD,
    KW_MODULE,
    KW_NIL,
    KW_OF,
    KW_OR,
    KW_POINTER,
    KW_PROCEDURE,
    KW_RECORD,
    KW_REPEAT,
    KW_RETURN,
    KW_THEN,
    KW_TO,
    KW_TRUE,
    KW_TYPE,
    KW_UNTIL,
    KW_VAR,
    KW_WHILE
};

struct Token {
    TokenType type = TokenType::Unknown;
    std::string text;

    std::uint32_t line = 1;
    std::uint32_t col = 1;

    std::int64_t intValue = 0;
    double realValue = 0.0;
};

inline const char* toString(TokenType t)
{
    switch (t) {
        // ----- специальные -----
        case TokenType::Eof:      return "Eof";
        case TokenType::Unknown:  return "Unknown";

        // ----- литералы -----
        case TokenType::Ident:    return "Ident";
        case TokenType::Integer:  return "Integer";
        case TokenType::Real:     return "Real";
        case TokenType::String:   return "String";

        // ----- операторы -----
        case TokenType::Plus:     return "Plus";
        case TokenType::Minus:    return "Minus";
        case TokenType::Star:     return "Star";
        case TokenType::Slash:    return "Slash";
        case TokenType::Eq:       return "Eq";
        case TokenType::Neq:      return "Neq";
        case TokenType::Lt:       return "Lt";
        case TokenType::Le:       return "Le";
        case TokenType::Gt:       return "Gt";
        case TokenType::Ge:       return "Ge";
        case TokenType::Assign:   return "Assign";
        case TokenType::Caret:    return "Caret";
        case TokenType::Amp:      return "Amp";
        case TokenType::Tilde:    return "Tilde";
        case TokenType::Bar:      return "Bar";
        case TokenType::Range:    return "Range";

        // ----- пунктуация -----
        case TokenType::LParen:   return "LParen";
        case TokenType::RParen:   return "RParen";
        case TokenType::LBrack:   return "LBrack";
        case TokenType::RBrack:   return "RBrack";
        case TokenType::LBrace:   return "LBrace";
        case TokenType::RBrace:   return "RBrace";
        case TokenType::Comma:    return "Comma";
        case TokenType::Semicolon:return "Semicolon";
        case TokenType::Colon:    return "Colon";
        case TokenType::Dot:      return "Dot";

        // ----- ключевые слова -----
        case TokenType::KW_ARRAY:       return "KW_ARRAY";
        case TokenType::KW_BEGIN:       return "KW_BEGIN";
        case TokenType::KW_BY:          return "KW_BY";
        case TokenType::KW_CASE:        return "KW_CASE";
        case TokenType::KW_CONST:       return "KW_CONST";
        case TokenType::KW_DIV:         return "KW_DIV";
        case TokenType::KW_DO:          return "KW_DO";
        case TokenType::KW_ELSE:        return "KW_ELSE";
        case TokenType::KW_ELSIF:       return "KW_ELSIF";
        case TokenType::KW_END:         return "KW_END";
        case TokenType::KW_FALSE:       return "KW_FALSE";
        case TokenType::KW_FOR:         return "KW_FOR";
        case TokenType::KW_IF:          return "KW_IF";
        case TokenType::KW_IMPORT:      return "KW_IMPORT";
        case TokenType::KW_IN:          return "KW_IN";
        case TokenType::KW_IS:          return "KW_IS";
        case TokenType::KW_MOD:         return "KW_MOD";
        case TokenType::KW_MODULE:      return "KW_MODULE";
        case TokenType::KW_NIL:         return "KW_NIL";
        case TokenType::KW_OF:          return "KW_OF";
        case TokenType::KW_OR:          return "KW_OR";
        case TokenType::KW_POINTER:     return "KW_POINTER";
        case TokenType::KW_PROCEDURE:   return "KW_PROCEDURE";
        case TokenType::KW_RECORD:      return "KW_RECORD";
        case TokenType::KW_REPEAT:      return "KW_REPEAT";
        case TokenType::KW_RETURN:      return "KW_RETURN";
        case TokenType::KW_THEN:        return "KW_THEN";
        case TokenType::KW_TO:          return "KW_TO";
        case TokenType::KW_TRUE:        return "KW_TRUE";
        case TokenType::KW_TYPE:        return "KW_TYPE";
        case TokenType::KW_UNTIL:       return "KW_UNTIL";
        case TokenType::KW_VAR:         return "KW_VAR";
        case TokenType::KW_WHILE:       return "KW_WHILE";

        default: return "???";
    }
}

inline std::ostream& operator<<(std::ostream& os, const TokenType& t)
{
    return os << toString(t);
}

inline std::ostream& operator<<(std::ostream& os, const Token& tk)
{
    os << '[' << tk.type << "] "
       << "text='" << tk.text << "' "
       << "(line:" << tk.line << ", col:" << tk.col << ')';
    if (tk.type == TokenType::Integer)
        os << " int=" << tk.intValue;
    else if (tk.type == TokenType::Real)
        os << " real=" << tk.realValue;
    os << std::endl;
    return os;
}

