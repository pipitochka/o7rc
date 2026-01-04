CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra
FLEX     := flex

# Вход/выход
LEXER_SRC := lexer.l
LEXER_GEN := lex.yy.cc
MAIN_SRC  := main.cpp
TARGET    := lexer

.PHONY: all clean

all: $(TARGET)

$(LEXER_GEN): $(LEXER_SRC)
	$(FLEX) -o $@ $<

$(TARGET): $(LEXER_GEN) $(MAIN_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(LEXER_GEN) $(TARGET)

run: $(TARGET)
	./$(TARGET)