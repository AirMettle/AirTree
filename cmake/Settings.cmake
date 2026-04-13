include_guard(GLOBAL)


include(GNUInstallDirs)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Enable to debug target tree (produces a lot of output)
#set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_DEBUG_MODE ON)

# Determine the number of available processors in a cross-platform way
include(ProcessorCount)
ProcessorCount(NPROC)
if(NOT NPROC)
    set(NPROC 1)  # Default to 1 if ProcessorCount fails
endif()

set(DEPS_PREFIX "_deps" CACHE INTERNAL "")
set(WORKING_PREFIX "_working" CACHE INTERNAL "")

### Build type
# Default to RelWithDebInfo if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

# Dependency build type: map custom types to standard CMake types for third-party libs
# Standard types pass through; custom types (e.g. Benchmark) map to Release
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE "Debug" CACHE INTERNAL "Build type for third-party dependencies")
    set(AIRMETTLE_AIRTREE_BOOST_VARIANT "debug" CACHE INTERNAL "Boost variant for b2")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" OR CMAKE_BUILD_TYPE STREQUAL "Benchmark")
    set(AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE "Release" CACHE INTERNAL "Build type for third-party dependencies")
    set(AIRMETTLE_AIRTREE_BOOST_VARIANT "release" CACHE INTERNAL "Boost variant for b2")
else()
    message(WARNING "Unknown CMAKE_BUILD_TYPE '${CMAKE_BUILD_TYPE}', defaulting deps to Release")
    set(AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE "Release" CACHE INTERNAL "Build type for third-party dependencies")
    set(AIRMETTLE_AIRTREE_BOOST_VARIANT "release" CACHE INTERNAL "Boost variant for b2")
endif()

### Config-keyed dependency directory
set(AIRMETTLE_AIRTREE_DEPENDENCY_ROOT "${CMAKE_BINARY_DIR}/.airmettle/airtree-deps" CACHE PATH "Root directory for AirMettle AirTree dependencies")

# Detect platform: distro + major version
if(EXISTS "/etc/os-release")
    file(STRINGS "/etc/os-release" _OS_RELEASE_ID REGEX "^ID=")
    file(STRINGS "/etc/os-release" _OS_RELEASE_VERSION REGEX "^VERSION_ID=")
    string(REGEX REPLACE "^ID=\"?([^\"]+)\"?" "\\1" _DISTRO_NAME "${_OS_RELEASE_ID}")
    string(REGEX REPLACE "^VERSION_ID=\"?([0-9]+).*" "\\1" _DISTRO_MAJOR "${_OS_RELEASE_VERSION}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(_DISTRO_NAME "macos")
    execute_process(
        COMMAND sw_vers -productVersion
        OUTPUT_VARIABLE _macos_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REGEX REPLACE "^([0-9]+).*" "\\1" _DISTRO_MAJOR "${_macos_version}")
else()
    set(_DISTRO_NAME "unknown")
    set(_DISTRO_MAJOR "0")
endif()

# Architecture
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _ARCH)

# Compiler ID + major version (available after toolchain is loaded)
string(TOLOWER "${CMAKE_CXX_COMPILER_ID}" _COMPILER_ID)
string(REGEX REPLACE "^([0-9]+).*" "\\1" _COMPILER_MAJOR "${CMAKE_CXX_COMPILER_VERSION}")

# Deps build type tag (lowercase)
string(TOLOWER "${AIRMETTLE_AIRTREE_DEPS_BUILD_TYPE}" _DEPS_BT_LOWER)

set(_CONFIG_ID "${_DISTRO_NAME}${_DISTRO_MAJOR}-${_ARCH}-${_COMPILER_ID}${_COMPILER_MAJOR}-${_DEPS_BT_LOWER}")
set(AIRMETTLE_AIRTREE_DEPENDENCY_DIR "${AIRMETTLE_AIRTREE_DEPENDENCY_ROOT}/${_CONFIG_ID}" CACHE PATH "Directory for AirMettle AirTree dependencies" FORCE)
message(STATUS "Dependency directory: ${AIRMETTLE_AIRTREE_DEPENDENCY_DIR}")

### Dependency binary cache (S3)
set(AIRTREE_DEPS_CACHE_S3_BUCKET "s3://airmettle-airtree" CACHE STRING
    "S3 bucket for dep binary cache")
set(AIRTREE_DEPS_CACHE_S3_REGION "us-east-2" CACHE STRING
    "AWS region for S3 cache bucket")
set(AIRTREE_DEPS_CACHE_S3_PREFIX "deps/v1" CACHE STRING
    "Key prefix in S3 bucket (cache version namespace)")

### Dependency versions

set(CLI11_VERSION "v2.5.0" CACHE INTERNAL "")
set(OPENSSL_VERSION "openssl-3.5.2" CACHE INTERNAL "")
set(CURL_VERSION "curl-8_15_0" CACHE INTERNAL "")
set(ASIO_VERSION "asio-1-34-2" CACHE INTERNAL "")
set(RE2_VERSION "2025-08-05" CACHE INTERNAL "")
set(ABSL_VERSION "20250512.1" CACHE INTERNAL "")
set(LZ4_VERSION "v1.9.4" CACHE INTERNAL "")
set(LIBXML2_VERSION "v2.14.5" CACHE INTERNAL "")
set(ZSTD_VERSION "v1.5.7" CACHE INTERNAL "")
set(THRIFT_VERSION "v0.22.0" CACHE INTERNAL "")
set(BOOST_VERSION "boost-1.88.0" CACHE INTERNAL "")
set(MIMALLOC_VERSION "v3.0.8" CACHE INTERNAL "")
set(ZLIB_VERSION "v1.3.1" CACHE INTERNAL "")
set(NETCDF4_VERSION "9328ba17cb53f13a63707547c94f4715243dafdf" CACHE INTERNAL "")
set(HDF5_VERSION "hdf5-1_14_0" CACHE INTERNAL "")
set(SPDLOG_VERSION "v1.15.3" CACHE INTERNAL "")
set(GTEST_VERSION "release-1.12.1" CACHE INTERNAL "")
set(ARROW_VERSION "apache-arrow-20.0.0" CACHE INTERNAL "")
set(CROW_VERSION "v1.2.1.2" CACHE INTERNAL "")
set(NLOHMANN_JSON_VERSION "v3.11.3" CACHE INTERNAL "")
set(LIBRDKAFKA_VERSION "v2.8.0" CACHE INTERNAL "")
set(GOOGLE_BENCH_VERSION "v1.9.4" CACHE INTERNAL "")
set(FLATBUFFERS_VERSION "v25.2.10" CACHE INTERNAL "")
set(PYBIND11_VERSION "v3.0.0" CACHE INTERNAL "")

### Policies

# Disable exporting packages to the user package registry. Enabling it creates havoc with libraries being found
# when they shouldn't be
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.15)
    cmake_policy(SET CMP0090 NEW)
endif ()
set(CMAKE_FIND_USE_PACKAGE_REGISTRY FALSE)
set(CMAKE_EXPORT_NO_PACKAGE_REGISTRY TRUE)
set(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY TRUE)

### Build infrastructure

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Project include paths
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/include/)

# Preprocessor macros for build types
add_compile_definitions($<$<STREQUAL:${CMAKE_BUILD_TYPE},Benchmark>:BENCHMARK_MODE>)
add_compile_definitions($<$<STREQUAL:${CMAKE_BUILD_TYPE},Release>:RELEASE_MODE>)
add_compile_definitions($<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:DEBUG_MODE>)
