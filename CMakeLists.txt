cmake_minimum_required(VERSION 3.20)
project(o7rc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

FLEX_TARGET(O7Lexer
        ${CMAKE_SOURCE_DIR}/src/tokenizer/impl/flex/oberon.l
        ${CMAKE_BINARY_DIR}/oberon_lex.cc
)

BISON_TARGET(O7Parser
        ${SRC_DIR}/parser/impl/bison/oberon.y
        ${CMAKE_BINARY_DIR}/oberon.tab.cc
        DEFINES_FILE ${CMAKE_BINARY_DIR}/oberon.tab.h
)

add_executable(o7rc
        ${SRC_DIR}/main.cpp

        ${SRC_DIR}/tokenizer/impl/flex/FlexTokenizer.cpp
        ${FLEX_O7Lexer_OUTPUTS}

        ${SRC_DIR}/parser/impl/bison/BisonParser.cpp
        ${SRC_DIR}/parser/impl/bison/yylex_adapter.cpp
        ${BISON_O7Parser_OUTPUTS}
)

target_include_directories(o7rc PRIVATE
        ${SRC_DIR}
        ${CMAKE_BINARY_DIR}
)

