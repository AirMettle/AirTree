# =============================================================================
# AirTree C++ API — Example CMake Project
# =============================================================================
#
# This is a general-purpose template for building your own C++ programs against
# the AirTree static libraries and public headers (<airtree/...>). Copy this
# directory (or just this CMakeLists.txt) into your project and adapt the
# "Your application" section at the bottom.
#
# The bundled example (test_top_k.cpp) demonstrates the generate + Top-K query
# workflow end-to-end: build a 1D histogram in memory, then query it.
#
# Prerequisites
#   • A built AirTree tree on disk. From the AirTree repo root:
#       ./tools/build/build.sh
#   • A C++17 compiler and CMake >= 3.22.
#
# Quick start (from this directory)
#   cmake -S . -B build
#   cmake --build build
#   ./build/test_top_k
#
# Path configuration
#   By default this file assumes it lives inside the AirTree repository and
#   derives paths from AIRTREE_SOURCE_ROOT (defaults to the parent directory).
#   The AirTree build directory follows the repo convention:
#
#     cmake-build-<toolchain>-<buildtype>-<sanitizer>
#
#   Example (Ubuntu 22.04, gcc-11, RelWithDebInfo, no sanitizer):
#     cmake-build-gnu-11-RelWithDebInfo-nosan
#
#   Override the build-directory components if yours differ:
#     cmake -S . -B build \
#       -D AIRTREE_TOOLCHAIN=gnu-11 \
#       -D AIRTREE_BUILD_TYPE=RelWithDebInfo \
#       -D AIRTREE_SANITIZER=nosan
#
#   Or bypass auto-detection entirely and point at any install prefix that
#   contains include/ and lib/ (CPack tarball extract, .deb install, etc.):
#     cmake -S . -B build \
#       -D AIRTREE_INSTALL_ROOT=/opt/airmettle/airtree/<version>
#
#   spdlog is a transitive dependency of the AirTree archives. If you built
#   AirTree from source, its cached copy is discovered automatically under
#   <build-dir>/.airmettle/airtree-deps/. Override with -D SPDLOG_ROOT=... if
#   you link against a system or custom spdlog install instead.
#
# Linking notes
#   AirTree ships as static archives (.a). Link only the libraries your program
#   uses, plus their transitive deps. See docs/cpp-api.md → Linking for the
#   full matrix. Common combinations:
#     Generate only : core, util, spdlog
#     Query         : query, core, util, spdlog
#     Merge         : merge, core, util, spdlog
#     Export        : export, query, core, util, spdlog (+ Arrow/Parquet)
#
# =============================================================================

cmake_minimum_required(VERSION 3.22)
project(AirTree_Example LANGUAGES CXX)

# ====================== USER CONFIGURABLE PATHS ======================

# Root of the AirTree source repository. Defaults to the parent of this folder,
# which is correct when AirTreeTest/ lives inside the AirTree checkout.
if(NOT AIRTREE_SOURCE_ROOT)
    get_filename_component(AIRTREE_SOURCE_ROOT
        "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
endif()
set(AIRTREE_SOURCE_ROOT "${AIRTREE_SOURCE_ROOT}" CACHE PATH
    "Path to the AirTree repository root")

# Components of the AirTree build-directory name. Must match the values used
# when you ran ./tools/build/build.sh (see tools/build/settings.sh).
set(AIRTREE_TOOLCHAIN "gnu-11" CACHE STRING
    "Toolchain tag in the AirTree build directory (e.g. gnu-11)")
set(AIRTREE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
    "CMake build type used for the AirTree build (e.g. RelWithDebInfo)")
set(AIRTREE_SANITIZER "nosan" CACHE STRING
    "Sanitizer suffix used for the AirTree build (nosan, asan, ubsan, asan_ubsan)")

set(_airtree_build_dir_name
    "cmake-build-${AIRTREE_TOOLCHAIN}-${AIRTREE_BUILD_TYPE}-${AIRTREE_SANITIZER}")
set(AIRTREE_BUILD_DIR "${AIRTREE_SOURCE_ROOT}/${_airtree_build_dir_name}" CACHE PATH
    "AirTree CMake build directory (auto-derived from the settings above)")

# Install prefix containing include/ and lib/libairtree-*.a.
# Auto-detected from the CPack TGZ staging tree when left empty.
set(AIRTREE_INSTALL_ROOT "" CACHE PATH
    "AirTree install prefix (include/ + lib/). Overrides auto-detection.")

if(NOT AIRTREE_INSTALL_ROOT)
    file(GLOB _airtree_install_candidates
        "${AIRTREE_BUILD_DIR}/_CPack_Packages/Linux/TGZ/airtree-*-Linux"
        LIST_DIRECTORIES true)
    list(SORT _airtree_install_candidates COMPARE NATURAL ORDER DESCENDING)
    if(_airtree_install_candidates)
        list(GET _airtree_install_candidates 0 AIRTREE_INSTALL_ROOT)
    endif()
endif()

# spdlog install prefix (include/ + lib/libspdlog.a).
# Auto-detected from the AirTree third-party dependency cache when left empty.
set(SPDLOG_ROOT "" CACHE PATH
    "spdlog install prefix (include/ + lib/). Overrides auto-detection.")

if(NOT SPDLOG_ROOT)
    file(GLOB _spdlog_candidates
        "${AIRTREE_BUILD_DIR}/.airmettle/airtree-deps/*/spdlog_ep/*"
        LIST_DIRECTORIES true)
    foreach(_candidate IN LISTS _spdlog_candidates)
        if(EXISTS "${_candidate}/lib/libspdlog.a")
            set(SPDLOG_ROOT "${_candidate}")
            break()
        endif()
    endforeach()
endif()

# ====================== PATH VALIDATION ======================

if(NOT AIRTREE_INSTALL_ROOT
   OR NOT EXISTS "${AIRTREE_INSTALL_ROOT}/include/airtree"
   OR NOT EXISTS "${AIRTREE_INSTALL_ROOT}/lib/libairtree-core.a")
    message(FATAL_ERROR
        "Could not locate a valid AirTree install tree.\n"
        "  Expected: ${AIRTREE_INSTALL_ROOT}\n"
        "  Build AirTree first (./tools/build/build.sh from the repo root), then\n"
        "  reconfigure with one of:\n"
        "    -D AIRTREE_SOURCE_ROOT=<path-to-AirTree-repo>\n"
        "    -D AIRTREE_BUILD_DIR=<path-to-cmake-build-...>\n"
        "    -D AIRTREE_INSTALL_ROOT=<path-with-include-and-lib>")
endif()

if(NOT SPDLOG_ROOT OR NOT EXISTS "${SPDLOG_ROOT}/lib/libspdlog.a")
    message(FATAL_ERROR
        "Could not locate spdlog (required by the AirTree static archives).\n"
        "  Searched under: ${AIRTREE_BUILD_DIR}/.airmettle/airtree-deps/\n"
        "  Override with:  -D SPDLOG_ROOT=<path-with-include-and-lib>")
endif()

message(STATUS "AirTree install : ${AIRTREE_INSTALL_ROOT}")
message(STATUS "AirTree build   : ${AIRTREE_BUILD_DIR}")
message(STATUS "spdlog install  : ${SPDLOG_ROOT}")

# ====================== IMPORTED AIRTREE LIBRARIES ======================
# Each target wraps one libairtree-*.a archive from the install tree.
# Link only the ones your application needs (see header comment above).

macro(_define_airtree_imported_target _alias _archive)
    add_library(${_alias} STATIC IMPORTED)
    set_target_properties(${_alias} PROPERTIES
        IMPORTED_LOCATION "${AIRTREE_INSTALL_ROOT}/lib/${_archive}"
        INTERFACE_INCLUDE_DIRECTORIES "${AIRTREE_INSTALL_ROOT}/include"
    )
endmacro()

_define_airtree_imported_target(airtree::core   libairtree-core.a)
_define_airtree_imported_target(airtree::util   libairtree-util.a)
_define_airtree_imported_target(airtree::query  libairtree-query.a)
_define_airtree_imported_target(airtree::merge  libairtree-merge.a)
_define_airtree_imported_target(airtree::reader libairtree-reader.a)
_define_airtree_imported_target(airtree::export libairtree-export.a)

# spdlog — logging dependency pulled in by airtree-core and airtree-util.
add_library(spdlog::spdlog STATIC IMPORTED)
set_target_properties(spdlog::spdlog PROPERTIES
    IMPORTED_LOCATION "${SPDLOG_ROOT}/lib/libspdlog.a"
    INTERFACE_INCLUDE_DIRECTORIES "${SPDLOG_ROOT}/include"
)

# Convenience list of POSIX libs commonly required when linking AirTree
# statically on Linux. Extend this if you also pull in Arrow/Parquet (export).
set(AIRTREE_SYSTEM_LIBS pthread dl rt m)

# ====================== YOUR APPLICATION ======================
# Replace test_top_k with your own target(s). Keep only the airtree::* archives
# your code actually calls into.

add_executable(test_top_k test_top_k.cpp)
target_compile_features(test_top_k PRIVATE cxx_std_17)

target_link_libraries(test_top_k
    PRIVATE
        airtree::query    # TopK, Percentile, CDF, ...
        airtree::core     # generate(), histogram encoding
        airtree::util     # shared utilities (transitive dep of core/query)
        spdlog::spdlog    # logging (transitive dep of core/util)
        ${AIRTREE_SYSTEM_LIBS}
)