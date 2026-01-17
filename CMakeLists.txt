cmake_minimum_required(VERSION 3.20)
project(o7rc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(FLEX REQUIRED)

FLEX_TARGET(O7Lexer
        ${CMAKE_SOURCE_DIR}/src/tokenizer/impl/flex/oberon.l
        ${CMAKE_BINARY_DIR}/oberon_lex.cc
)

add_executable(o7rc
        src/main.cpp
        src/tokenizer/impl/flex/FlexTokenizer.cpp
        ${FLEX_O7Lexer_OUTPUTS}
)

target_include_directories(o7rc PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}
)
