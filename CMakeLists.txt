cmake_minimum_required(VERSION 3.20)
project(o7rc LANGUAGES CXX)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(TESTS_DIR ${CMAKE_SOURCE_DIR}/tests)

option(USE_FLEX    "Build Flex tokenizer"         ON)
option(USE_BISON   "Build Bison parser"           ON)
option(USE_DEBUG   "More info"                    OFF)
option(USE_CODEGEN "Build RISC-V code generator"  ON)
option(USE_RARS    "Use RARS"  ON)


include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_library(core "")

add_executable(o7rc_tests
        ${TESTS_DIR}/basic.cpp
)

target_include_directories(core PRIVATE
        ${SRC_DIR}
        ${CMAKE_BINARY_DIR}
)

target_include_directories(o7rc_tests PRIVATE
        ${SRC_DIR}
        ${CMAKE_BINARY_DIR}
)

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

target_link_libraries(core PUBLIC ast)

if(USE_FLEX)
    message("Use FLEX tokenizer")
    find_package(FLEX REQUIRED)
    FLEX_TARGET(O7Lexer 
            ${CMAKE_SOURCE_DIR}/src/tokenizer/impl/flex/oberon.l 
            ${CMAKE_BINARY_DIR}/oberon_lex.cc
    )
    target_sources(core PRIVATE
            ${SRC_DIR}/tokenizer/impl/flex/FlexTokenizer.cpp 
            ${FLEX_O7Lexer_OUTPUTS}
    )
    target_sources(o7rc_tests PRIVATE
            ${TESTS_DIR}/tokenizer/flex/test_flex_tokenizer_logic.cpp
            ${TESTS_DIR}/tokenizer/flex/test_flex_tokenizer_grammar.cpp
            ${TESTS_DIR}/tokenizer/flex/test_flex_tokenizer_snippets.cpp
    )
    target_compile_definitions(core PUBLIC USE_FLEX=1)
else()
    message(FATAL_ERROR "No tokenizer selected!")    
endif()

if(USE_BISON)
    message("Use bison lexer")
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
    target_compile_definitions(core PUBLIC USE_BISON=1)
    target_sources(o7rc_tests PRIVATE
            ${TESTS_DIR}/parser/bison/test_bison_parser_structure.cpp
            ${TESTS_DIR}/parser/bison/test_bison_parser_snippets.cpp
            ${TESTS_DIR}/parser/bison/test_bison_parser_ast_expr.cpp
            ${TESTS_DIR}/parser/bison/test_bison_parser_ast_stmt.cpp
            ${TESTS_DIR}/parser/bison/test_bison_parser_ast_types.cpp
    )
else()
    message(FATAL_ERROR "No lexer selected!")
endif()

if(USE_DEBUG)
    message("Use debug")
    target_sources(core PRIVATE
            ${SRC_DIR}/tokenizer/impl/debug/BufferedTokenizer.cpp
    )
    target_compile_definitions(core PUBLIC USE_DEBUG=1)
endif()

if(USE_CODEGEN)
    message("Use RISC-V code generator")
    target_sources(core PRIVATE
            ${SRC_DIR}/codegen/Emitter.cpp
            ${SRC_DIR}/codegen/TypeInfo.cpp
            ${SRC_DIR}/codegen/SymbolTable.cpp
            ${SRC_DIR}/codegen/RiscVCodeGen.cpp
    )
    target_compile_definitions(core PUBLIC USE_CODEGEN=1)
endif()

add_executable(o7rc
        ${SRC_DIR}/main.cpp
)

target_include_directories(o7rc PRIVATE
        ${SRC_DIR}
        ${CMAKE_BINARY_DIR}
)

target_link_libraries(o7rc PRIVATE core)

target_link_libraries(o7rc_tests
        PRIVATE
        core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(o7rc_tests)

# ── End-to-end тесты: o7rc+RARS vs OBNC (эталон) ────────────
option(USE_RARS "Download RARS/OBNC and enable e2e tests" OFF)

if(USE_RARS)
    find_package(Java QUIET COMPONENTS Runtime)
    if(NOT Java_FOUND)
        message(WARNING "Java not found — e2e tests disabled")
    else()
        message(STATUS "Java found: ${Java_JAVA_EXECUTABLE}")

        # ── RARS ──
        set(RARS_VERSION "1.6")
        set(RARS_JAR "${CMAKE_BINARY_DIR}/rars.jar")
        if(NOT EXISTS "${RARS_JAR}")
            message(STATUS "Downloading RARS v${RARS_VERSION}...")
            file(DOWNLOAD
                "https://github.com/TheThirdOne/rars/releases/download/v${RARS_VERSION}/rars1_6.jar"
                "${RARS_JAR}"
                SHOW_PROGRESS
                STATUS RARS_DL_STATUS
            )
            list(GET RARS_DL_STATUS 0 RARS_DL_CODE)
            if(NOT RARS_DL_CODE EQUAL 0)
                message(WARNING "Failed to download RARS — e2e tests disabled")
                set(RARS_JAR "")
            endif()
        endif()

        # ── OBNC ──
        set(OBNC_VERSION "0.17.2")
        set(OBNC_PREFIX "${CMAKE_BINARY_DIR}/obnc-install")
        set(OBNC_BIN "${OBNC_PREFIX}/bin/obnc")
        if(NOT EXISTS "${OBNC_BIN}")
            set(OBNC_TARBALL "${CMAKE_BINARY_DIR}/obnc-${OBNC_VERSION}.tar.gz")
            set(OBNC_SRC "${CMAKE_BINARY_DIR}/obnc-${OBNC_VERSION}")
            if(NOT EXISTS "${OBNC_TARBALL}")
                message(STATUS "Downloading OBNC v${OBNC_VERSION}...")
                file(DOWNLOAD
                    "https://miasap.se/obnc/downloads/obnc_${OBNC_VERSION}.tar.gz"
                    "${OBNC_TARBALL}"
                    SHOW_PROGRESS
                    STATUS OBNC_DL_STATUS
                )
                list(GET OBNC_DL_STATUS 0 OBNC_DL_CODE)
                if(NOT OBNC_DL_CODE EQUAL 0)
                    message(WARNING "Failed to download OBNC")
                    set(OBNC_BIN "")
                endif()
            endif()
            if(OBNC_BIN AND NOT EXISTS "${OBNC_SRC}")
                execute_process(
                    COMMAND ${CMAKE_COMMAND} -E tar xzf "${OBNC_TARBALL}"
                    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
                )
            endif()
            if(OBNC_BIN AND EXISTS "${OBNC_SRC}")
                message(STATUS "Building OBNC...")
                execute_process(
                    COMMAND ./build --prefix=${OBNC_PREFIX}
                    WORKING_DIRECTORY "${OBNC_SRC}"
                    RESULT_VARIABLE OBNC_BUILD_RC
                )
                if(OBNC_BUILD_RC EQUAL 0)
                    execute_process(
                        COMMAND ./install
                        WORKING_DIRECTORY "${OBNC_SRC}"
                        RESULT_VARIABLE OBNC_INSTALL_RC
                    )
                    if(NOT OBNC_INSTALL_RC EQUAL 0)
                        message(WARNING "OBNC install failed")
                        set(OBNC_BIN "")
                    endif()
                else()
                    message(WARNING "OBNC build failed")
                    set(OBNC_BIN "")
                endif()
            endif()
        endif()

        if(OBNC_BIN)
            message(STATUS "OBNC: ${OBNC_BIN}")
        else()
            message(WARNING "OBNC not available — e2e tests disabled")
        endif()

        # ── Найти libgc для линковки OBNC-скомпилированных программ ──
        set(GC_LIB_DIR "")
        find_library(GC_LIBRARY NAMES gc PATHS /opt/homebrew/lib /usr/local/lib /usr/lib)
        if(GC_LIBRARY)
            get_filename_component(GC_LIB_DIR "${GC_LIBRARY}" DIRECTORY)
            message(STATUS "libgc found: ${GC_LIBRARY}")
        endif()
        find_path(GC_INCLUDE_DIR NAMES gc/gc.h gc.h PATHS /opt/homebrew/include /usr/local/include /usr/include)
        if(GC_INCLUDE_DIR)
            message(STATUS "gc.h found: ${GC_INCLUDE_DIR}")
        endif()

        # ── Регистрация тестов ──
        if(RARS_JAR AND OBNC_BIN)
            set(E2E_DIR "${CMAKE_SOURCE_DIR}/tests/e2e")
            set(E2E_RUNNER "${E2E_DIR}/run_test.sh")

            file(GLOB E2E_SOURCES "${E2E_DIR}/programs/*.obr")
            foreach(SRC ${E2E_SOURCES})
                get_filename_component(TEST_NAME ${SRC} NAME_WE)
                add_test(
                    NAME "e2e_${TEST_NAME}"
                    COMMAND bash "${E2E_RUNNER}"
                        "$<TARGET_FILE:o7rc>"
                        "${Java_JAVA_EXECUTABLE}"
                        "${RARS_JAR}"
                        "${OBNC_BIN}"
                        "${SRC}"
                )
                set(E2E_ENV "")
                if(GC_LIB_DIR)
                    list(APPEND E2E_ENV "LIBRARY_PATH=${GC_LIB_DIR}")
                endif()
                if(GC_INCLUDE_DIR)
                    list(APPEND E2E_ENV "C_INCLUDE_PATH=${GC_INCLUDE_DIR}")
                endif()
                set_tests_properties("e2e_${TEST_NAME}" PROPERTIES
                    TIMEOUT 60
                    LABELS "e2e"
                    ENVIRONMENT "${E2E_ENV}"
                )
            endforeach()
        endif()
    endif()
endif()