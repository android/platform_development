#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.9.0)

project(header-checker)

find_package(Protobuf REQUIRED)

find_program(LLVM_CONFIG
    NAMES llvm-config
    DOC "llvm-config executable"
)

if (NOT LLVM_CONFIG)
    message(FATAL_ERROR "llvm-config not found")
endif ()

execute_process(
    COMMAND "${LLVM_CONFIG}" --includedir
    OUTPUT_VARIABLE LLVM_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND "${LLVM_CONFIG}" --libdir
    OUTPUT_VARIABLE LLVM_LIBDIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND "${LLVM_CONFIG}" --cxxflags
    OUTPUT_VARIABLE LLVM_CXXFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REPLACE " " ";" LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")

execute_process(
    COMMAND "${LLVM_CONFIG}" --ldflags
    OUTPUT_VARIABLE LLVM_LDLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND "${LLVM_CONFIG}" --system-libs
    OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set (CLANG_LIBS
    clangToolingCore
    clangTooling
    clangFrontendTool
    clangFrontend
    clangDriver
    clangIndex
    clangSerialization
    clangCodeGen
    clangParse
    clangSema
    clangAnalysis
    clangEdit
    clangAST
    clangLex
    clangBasic
)

set (LLVM_LIBS
    LLVMBitWriter
    LLVMProfileData
    LLVMOption
    LLVMObject
    LLVMBitReader
    LLVMCore
    LLVMBinaryFormat
    LLVMMC
    LLVMMCParser
    LLVMSupport
)

foreach (lib ${CLANG_LIBS};${LLVM_LIBS})
    add_library("${lib}" STATIC IMPORTED GLOBAL)
    set_property(
        TARGET "${lib}"
        PROPERTY IMPORTED_LOCATION "${LLVM_LIBDIR}/lib${lib}.a"
    )
endforeach()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#------------------------------------------------------------------------------
# header-checker-proto
#------------------------------------------------------------------------------
add_subdirectory(proto)


#------------------------------------------------------------------------------
# header-abi-util
#------------------------------------------------------------------------------

add_library(header-abi-util STATIC
    "header-abi-util/src/abi_diff_helpers.cpp"
    "header-abi-util/src/collect_exported_headers.cpp"
    "header-abi-util/src/ir_representation.cpp"
    "header-abi-util/src/ir_representation_protobuf.cpp"
    "header-abi-util/src/so_file_parser.cpp"
    "header-abi-util/src/version_script_parser.cpp"
)

target_compile_options(header-abi-util
    PRIVATE ${LLVM_CXXFLAGS}
)

target_include_directories(header-abi-util
    PUBLIC "header-abi-util/include"
    PRIVATE "${LLVM_INCLUDE_DIR}"
)

target_link_libraries(header-abi-util
    header-checker-proto
)


#------------------------------------------------------------------------------
# header-abi-dumper
#------------------------------------------------------------------------------

add_executable(header-abi-dumper
    "header-abi-dumper/src/abi_wrappers.cpp"
    "header-abi-dumper/src/ast_processing.cpp"
    "header-abi-dumper/src/fixed_argv.cpp"
    "header-abi-dumper/src/frontend_action.cpp"
    "header-abi-dumper/src/frontend_action_factory.cpp"
    "header-abi-dumper/src/header_checker.cpp"
    "header-abi-dumper/src/workaround_fortify.cpp"
    "header-abi-dumper/src/workaround_openmp.cpp"
)

target_compile_options(header-abi-dumper
    PRIVATE ${LLVM_CXXFLAGS}
)

set_target_properties(header-abi-dumper
    PROPERTIES LINK_FLAGS "${LLVM_LDFLAGS} ${LLVM_SYSTEM_LIBS}"
)

target_include_directories(header-abi-dumper
    PRIVATE "header-abi-dumper/include"
    PRIVATE "${LLVM_INCLUDE_DIR}"
)

target_link_libraries(header-abi-dumper
    header-abi-util
    header-checker-proto
    ${CLANG_LIBS}
    ${LLVM_LIBS}
)


#------------------------------------------------------------------------------
# header-abi-linker
#------------------------------------------------------------------------------

add_executable(header-abi-linker
    "header-abi-linker/src/header_abi_linker.cpp"
)

target_compile_options(header-abi-linker
    PRIVATE ${LLVM_CXXFLAGS}
)

set_target_properties(header-abi-linker
    PROPERTIES LINK_FLAGS "${LLVM_LDFLAGS} ${LLVM_SYSTEM_LIBS}"
)

target_include_directories(header-abi-linker
    PRIVATE "header-abi-linker/include"
    PRIVATE "${LLVM_INCLUDE_DIR}"
)

target_link_libraries(header-abi-linker
    header-abi-util
    header-checker-proto
    ${LLVM_LIBS}
)


#------------------------------------------------------------------------------
# header-abi-diff
#------------------------------------------------------------------------------

add_executable(header-abi-diff
    "header-abi-diff/src/abi_diff.cpp"
    "header-abi-diff/src/abi_diff_wrappers.cpp"
    "header-abi-diff/src/header_abi_diff.cpp"
)

target_compile_options(header-abi-diff
    PRIVATE ${LLVM_CXXFLAGS}
)

set_target_properties(header-abi-diff
    PROPERTIES LINK_FLAGS "${LLVM_LDFLAGS} ${LLVM_SYSTEM_LIBS}"
)

target_include_directories(header-abi-diff
    PRIVATE "header-abi-diff/include"
    PRIVATE "${LLVM_INCLUDE_DIR}"
)

target_link_libraries(header-abi-diff
    header-abi-util
    header-checker-proto
    ${LLVM_LIBS}
)


#------------------------------------------------------------------------------
# merge-abi-diff
#------------------------------------------------------------------------------

add_executable(merge-abi-diff
    "merge-abi-diff/src/merge_abi_diff.cpp"
)

target_compile_options(merge-abi-diff
    PRIVATE ${LLVM_CXXFLAGS}
)

set_target_properties(merge-abi-diff
    PROPERTIES LINK_FLAGS "${LLVM_LDFLAGS} ${LLVM_SYSTEM_LIBS}"
)

target_include_directories(merge-abi-diff
    PRIVATE "merge-abi-diff/include"
    PRIVATE "${LLVM_INCLUDE_DIR}"
)

target_link_libraries(merge-abi-diff
    header-abi-util
    header-checker-proto
    ${LLVM_LIBS}
)


#------------------------------------------------------------------------------
# Install
#------------------------------------------------------------------------------

install(
    TARGETS
        header-abi-dumper
        header-abi-linker
        header-abi-diff
        merge-abi-diff
    DESTINATION bin
)
