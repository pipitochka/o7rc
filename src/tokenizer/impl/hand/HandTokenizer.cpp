#include "HandTokenizer.h"
#include <cctype>
#include <stdexcept>

const std::unordered_map<std::string, TokenType> HandTokenizer::keywords_ = {
    {"ARRAY", TokenType::KW_ARRAY},       {"BEGIN", TokenType::KW_BEGIN},
    {"BY", TokenType::KW_BY},             {"CASE", TokenType::KW_CASE},
    {"CONST", TokenType::KW_CONST},       {"DIV", TokenType::KW_DIV},
    {"DO", TokenType::KW_DO},             {"ELSE", TokenType::KW_ELSE},
    {"ELSIF", TokenType::KW_ELSIF},       {"END", TokenType::KW_END},
    {"FALSE", TokenType::KW_FALSE},       {"FOR", TokenType::KW_FOR},
    {"IF", TokenType::KW_IF},             {"IMPORT", TokenType::KW_IMPORT},
    {"IN", TokenType::KW_IN},             {"IS", TokenType::KW_IS},
    {"MOD", TokenType::KW_MOD},           {"MODULE", TokenType::KW_MODULE},
    {"NIL", TokenType::KW_NIL},           {"OF", TokenType::KW_OF},
    {"OR", TokenType::KW_OR},             {"POINTER", TokenType::KW_POINTER},
    {"PROCEDURE", TokenType::KW_PROCEDURE}, {"RECORD", TokenType::KW_RECORD},
    {"REPEAT", TokenType::KW_REPEAT},     {"RETURN", TokenType::KW_RETURN},
    {"THEN", TokenType::KW_THEN},         {"TO", TokenType::KW_TO},
    {"TRUE", TokenType::KW_TRUE},         {"TYPE", TokenType::KW_TYPE},
    {"UNTIL", TokenType::KW_UNTIL},       {"VAR", TokenType::KW_VAR},
    {"WHILE", TokenType::KW_WHILE},
};

HandTokenizer::HandTokenizer(std::istream& in) : in_(in) {}

char HandTokenizer::peekChar() {
    int ch = in_.peek();
    return (ch == std::char_traits<char>::eof()) ? '\0' : static_cast<char>(ch);
}

char HandTokenizer::getChar() {
    int ch = in_.get();
    if (ch == std::char_traits<char>::eof()) return '\0';
    char c = static_cast<char>(ch);
    if (c == '\n') {
        ++line_;
        col_ = 1;
    } else {
        ++col_;
    }
    return c;
}

void HandTokenizer::skipWhitespace() {
    while (true) {
        char c = peekChar();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            getChar();
        } else if (c == '(') {
            // Check for comment start "(* "
            in_.get(); // consume '('
            char c2 = peekChar();
            if (c2 == '*') {
                // Put back tracking: we already consumed '(' so update position
                if (c == '\n') { /* shouldn't happen */ }
                else { ++col_; }
                getChar(); // consume '*'
                skipComment();
            } else {
                in_.putback('(');
                break;
            }
        } else {
            break;
        }
    }
}

void HandTokenizer::skipComment() {
    int depth = 1;
    while (depth > 0) {
        char c = getChar();
        if (c == '\0') break;
        if (c == '(' && peekChar() == '*') {
            getChar();
            ++depth;
        } else if (c == '*' && peekChar() == ')') {
            getChar();
            --depth;
        }
    }
}

static bool isHexDigit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
}

Token HandTokenizer::readIdent() {
    Token t;
    t.line = line_;
    t.col = col_;
    std::string s;
    while (std::isalpha(static_cast<unsigned char>(peekChar())) ||
           std::isdigit(static_cast<unsigned char>(peekChar()))) {
        s += getChar();
    }
    auto it = keywords_.find(s);
    if (it != keywords_.end()) {
        t.type = it->second;
    } else {
        t.type = TokenType::Ident;
    }
    t.text = std::move(s);
    return t;
}

Token HandTokenizer::readNumber() {
    Token t;
    t.line = line_;
    t.col = col_;
    std::string s;

    while (isHexDigit(peekChar())) {
        s += getChar();
    }

    if (peekChar() == 'H') {
        s += getChar();
        t.type = TokenType::Integer;
        t.text = s;
        std::string hexStr = s.substr(0, s.size() - 1);
        t.intValue = std::stoll(hexStr, nullptr, 16);
        return t;
    }

    if (peekChar() == 'X') {
        s += getChar();
        t.type = TokenType::String;
        std::string hexStr = s.substr(0, s.size() - 1);
        char ch = static_cast<char>(std::stoi(hexStr, nullptr, 16));
        t.text = std::string(1, ch);
        return t;
    }

    if (peekChar() == '.' && in_.good()) {
        in_.get();
        char afterDot = peekChar();
        in_.putback('.');
        if (afterDot == '.') {
            // It's a range like 1..5, so this number is an integer
            t.type = TokenType::Integer;
            t.text = s;
            t.intValue = std::stoll(s);
            return t;
        }

        s += getChar();
        while (std::isdigit(static_cast<unsigned char>(peekChar()))) {
            s += getChar();
        }
        if (peekChar() == 'E' || peekChar() == 'e') {
            s += getChar();
            if (peekChar() == '+' || peekChar() == '-') {
                s += getChar();
            }
            while (std::isdigit(static_cast<unsigned char>(peekChar()))) {
                s += getChar();
            }
        }
        t.type = TokenType::Real;
        t.text = s;
        t.realValue = std::stod(s);
        return t;
    }

    t.type = TokenType::Integer;
    t.text = s;
    t.intValue = std::stoll(s);
    return t;
}

Token HandTokenizer::readString() {
    Token t;
    t.line = line_;
    t.col = col_;
    t.type = TokenType::String;

    getChar();
    std::string s;
    while (peekChar() != '"' && peekChar() != '\n' && peekChar() != '\0') {
        s += getChar();
    }
    if (peekChar() == '"') {
        getChar();
    }
    t.text = s;
    return t;
}

Token HandTokenizer::readOne() {
    skipWhitespace();

    Token t;
    t.line = line_;
    t.col = col_;

    char c = peekChar();

    if (c == '\0') {
        t.type = TokenType::Eof;
        return t;
    }

    if (std::isalpha(static_cast<unsigned char>(c))) {
        return readIdent();
    }

    if (std::isdigit(static_cast<unsigned char>(c))) {
        return readNumber();
    }

    if (c == '"') {
        return readString();
    }

    getChar();

    switch (c) {
    case '+': t.type = TokenType::Plus; t.text = "+"; break;
    case '-': t.type = TokenType::Minus; t.text = "-"; break;
    case '*': t.type = TokenType::Star; t.text = "*"; break;
    case '/': t.type = TokenType::Slash; t.text = "/"; break;
    case '&': t.type = TokenType::Amp; t.text = "&"; break;
    case '~': t.type = TokenType::Tilde; t.text = "~"; break;
    case '^': t.type = TokenType::Caret; t.text = "^"; break;
    case '|': t.type = TokenType::Bar; t.text = "|"; break;
    case '(': t.type = TokenType::LParen; t.text = "("; break;
    case ')': t.type = TokenType::RParen; t.text = ")"; break;
    case '[': t.type = TokenType::LBrack; t.text = "["; break;
    case ']': t.type = TokenType::RBrack; t.text = "]"; break;
    case '{': t.type = TokenType::LBrace; t.text = "{"; break;
    case '}': t.type = TokenType::RBrace; t.text = "}"; break;
    case ',': t.type = TokenType::Comma; t.text = ","; break;
    case ';': t.type = TokenType::Semicolon; t.text = ";"; break;
    case '#': t.type = TokenType::Neq; t.text = "#"; break;
    case '=': t.type = TokenType::Eq; t.text = "="; break;
    case ':':
        if (peekChar() == '=') {
            getChar();
            t.type = TokenType::Assign;
            t.text = ":=";
        } else {
            t.type = TokenType::Colon;
            t.text = ":";
        }
        break;
    case '<':
        if (peekChar() == '=') {
            getChar();
            t.type = TokenType::Le;
            t.text = "<=";
        } else {
            t.type = TokenType::Lt;
            t.text = "<";
        }
        break;
    case '>':
        if (peekChar() == '=') {
            getChar();
            t.type = TokenType::Ge;
            t.text = ">=";
        } else {
            t.type = TokenType::Gt;
            t.text = ">";
        }
        break;
    case '.':
        if (peekChar() == '.') {
            getChar();
            t.type = TokenType::Range;
            t.text = "..";
        } else {
            t.type = TokenType::Dot;
            t.text = ".";
        }
        break;
    default:
        t.type = TokenType::Unknown;
        t.text = std::string(1, c);
        break;
    }
    return t;
}

Token HandTokenizer::peek() {
    if (!hasLA_) {
        la_ = readOne();
        hasLA_ = true;
    }
    return la_;
}

Token HandTokenizer::next() {
    Token t = peek();
    hasLA_ = false;
    return t;
}
