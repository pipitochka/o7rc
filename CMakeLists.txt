cmake_minimum_required(VERSION 3.20)
project(o7rc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

option(USE_FLEX  "Build Flex tokenizer" ON)
option(USE_BISON "Build Bison parser"   ON)
option(USE_DEBUG "More info"   ON)

add_executable(o7rc
        ${SRC_DIR}/main.cpp
)

target_include_directories(o7rc PRIVATE
        ${SRC_DIR}
        ${CMAKE_BINARY_DIR}
)

if(USE_FLEX)
    find_package(FLEX REQUIRED)
    FLEX_TARGET(O7Lexer 
            ${CMAKE_SOURCE_DIR}/src/tokenizer/impl/flex/oberon.l 
            ${CMAKE_BINARY_DIR}/oberon_lex.cc
    )
    target_sources(o7rc PRIVATE
            ${SRC_DIR}/tokenizer/impl/flex/FlexTokenizer.cpp 
            ${FLEX_O7Lexer_OUTPUTS}
    )
    target_compile_definitions(o7rc PRIVATE USE_FLEX=1)
endif()

if(USE_BISON)
    find_package(BISON REQUIRED)
    BISON_TARGET(O7Parser 
            ${CMAKE_SOURCE_DIR}/src/parser/impl/bison/oberon.y 
            ${CMAKE_BINARY_DIR}/oberon.tab.cc
            DEFINES_FILE ${CMAKE_BINARY_DIR}/oberon.tab.h
    )
    target_sources(o7rc PRIVATE
            ${SRC_DIR}/parser/impl/bison/BisonParser.cpp
            ${SRC_DIR}/parser/impl/bison/yylex_adapter.cpp 
            ${BISON_O7Parser_OUTPUTS}
    )
    target_compile_definitions(o7rc PRIVATE USE_BISON=1)
endif()

if(USE_DEBUG)
    target_sources(o7rc PRIVATE
            ${SRC_DIR}/tokenizer/impl/debug/BufferedTokenizer.cpp
    )
    target_compile_definitions(o7rc PRIVATE USE_DEBUG=1)
endif()    
