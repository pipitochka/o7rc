cmake_minimum_required(VERSION 3.20)
project(o7rc LANGUAGES CXX)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(TESTS_DIR ${CMAKE_SOURCE_DIR}/tests)

option(USE_FLEX  "Build Flex tokenizer" ON)
option(USE_BISON "Build Bison parser"   ON)
option(USE_DEBUG "More info"   ON)

add_library(ast INTERFACE
        ${SRC_DIR}/util/ast/AstFwd.h
        ${SRC_DIR}/util/ast/IVisitor.h
        ${SRC_DIR}/util/ast/Node.h
        ${SRC_DIR}/util/ast/Expr.h
        ${SRC_DIR}/util/ast/Selector.h
        ${SRC_DIR}/util/ast/Type.h
        ${SRC_DIR}/util/ast/Decl.h
        ${SRC_DIR}/util/ast/Block.h
        ${SRC_DIR}/util/ast/Stmt.h
        ${SRC_DIR}/util/ast/Module.h
        ${SRC_DIR}/util/ast/Ast.h
)

target_include_directories(ast INTERFACE ${SRC_DIR}/util/ast)

add_library(core "")

target_link_libraries(core PUBLIC ast)

target_include_directories(core PRIVATE
        ${SRC_DIR}
        ${CMAKE_BINARY_DIR}
)

if(USE_FLEX)
    find_package(FLEX REQUIRED)
    FLEX_TARGET(O7Lexer 
            ${CMAKE_SOURCE_DIR}/src/tokenizer/impl/flex/oberon.l 
            ${CMAKE_BINARY_DIR}/oberon_lex.cc
    )
    target_sources(core PRIVATE
            ${SRC_DIR}/tokenizer/impl/flex/FlexTokenizer.cpp 
            ${FLEX_O7Lexer_OUTPUTS}
    )
    target_compile_definitions(core PRIVATE USE_FLEX=1)
endif()

if(USE_BISON)
    find_package(BISON REQUIRED)
    BISON_TARGET(O7Parser 
            ${CMAKE_SOURCE_DIR}/src/parser/impl/bison/oberon.y 
            ${CMAKE_BINARY_DIR}/oberon.tab.cc
            DEFINES_FILE ${CMAKE_BINARY_DIR}/oberon.tab.h
    )
    target_sources(core PRIVATE
            ${SRC_DIR}/parser/impl/bison/BisonParser.cpp
            ${SRC_DIR}/parser/impl/bison/yylex_adapter.cpp 
            ${BISON_O7Parser_OUTPUTS}
    )
    target_compile_definitions(core PRIVATE USE_BISON=1)
endif()

if(USE_DEBUG)
    target_sources(core PRIVATE
            ${SRC_DIR}/tokenizer/impl/debug/BufferedTokenizer.cpp
    )
    target_compile_definitions(core PRIVATE USE_DEBUG=1)
endif()    

add_executable(o7rc
        ${SRC_DIR}/main.cpp
)

target_include_directories(o7rc PRIVATE
        ${SRC_DIR}
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(o7rc PRIVATE core)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(o7rc_tests
        ${TESTS_DIR}/basic.cpp
)

target_link_libraries(o7rc_tests
        PRIVATE
        core
        GTest::gtest_main
)

target_include_directories(o7rc_tests PRIVATE
        ${SRC_DIR}
        ${CMAKE_BINARY_DIR}
)

include(GoogleTest)
gtest_discover_tests(o7rc_tests)
