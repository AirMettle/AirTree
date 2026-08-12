cmake_minimum_required(VERSION 3.22.1)

include(${CMAKE_SOURCE_DIR}/cmake/Version.cmake)

project(AirTree VERSION ${AIRTREE_VERSION_NUMERIC} LANGUAGES CXX)
add_compile_definitions(VERSION_NUMBER="${AIRTREE_FULL_VERSION}")
message(STATUS "Building version ${AIRTREE_FULL_VERSION} of AirTree")

# Target to aggregate all dependencies builds
add_custom_target(am_airtree_dependencies)

include(${CMAKE_SOURCE_DIR}/cmake/CMakeOptions.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/target_dag.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/find_program.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/flatbuffer_compile.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Settings.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/Sanitizers.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/DepsRegistry.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/DepsCache.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_zlib.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_zstd.cmake)
# if(NOT AIRTREE_SANITIZER_USES_ASAN)
    # include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_mimalloc.cmake)
# endif()
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_openssl.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_boost.cmake)
# include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_thrift.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_lz4.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_libxml2.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_curl.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_asio.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_spdlog.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_gtest.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_gbenchmark.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_cli11.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_nlohmann_json.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_arrow.cmake)
# include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_hdf5.cmake)
# include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_netcdf4.cmake)

# Detect architecture and define preprocessor directives
if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    add_compile_definitions(x86_ARCH)
    set(ARCH "x86_64")
    message(STATUS "Detected architecture: x86_64. Defining x86_ARCH.")
elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64|aarch64|ARM64")
    add_compile_definitions(ARM_ARCH)
    set(ARCH "aarch64")
    message(STATUS "Detected architecture: arm64. Defining ARM_ARCH.")
else ()
    message(STATUS "Detected architecture: ${CMAKE_HOST_SYSTEM_PROCESSOR}. No specific preprocessor definition added.")
endif ()

message(STATUS "ARCH: ${ARCH}")
message(STATUS "CMAKE_SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

# Platform-specific configurations
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "Configuring for macOS.")
    set(OS "DARWIN")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(OS "LINUX")
    message(STATUS "Configuring for Ubuntu/Linux.")
    find_package(PkgConfig REQUIRED)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(OS "WINDOWS")
    message(STATUS "Configuring for Windows.")
    message(STATUS "Windows: Disabling Incremental Linking to prevent LNK1116/Disk Full errors.")
    
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO " /INCREMENTAL:NO")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO " /INCREMENTAL:NO")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO " /INCREMENTAL:NO")
    
    string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG " /INCREMENTAL:NO")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS_DEBUG " /INCREMENTAL:NO")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS_DEBUG " /INCREMENTAL:NO")

    # Ensure UTF-8 encoding for MSVC
    add_compile_options(/utf-8 /fp:strict)
    # Link against necessary Windows libraries - right now we need it for crypt32 and ws2_32 for OpenSSL on Windows
    # shlwapi is needed to get accurate cpu clock speed for benchmarking on Windows
    link_libraries(crypt32 ws2_32 shlwapi)
    # The <windows.h> header defines min and max macros which can interfere with std::min and std::max. Since we are 
    # using it a lot in out codebase I had to add this definition to prevent those macros from being defined.
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
else ()
    message(FATAL_ERROR "Unsupported OS")
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(OS "LINUX")
    execute_process(
            COMMAND uname -m
            OUTPUT_VARIABLE ARCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # Read /etc/os-release to find the distribution ID (handles quoted and unquoted)
    file(READ "/etc/os-release" OS_RELEASE_CONTENT)
    string(REGEX MATCH "ID=\"?([a-zA-Z0-9_\-]+)\"?" DISTRO_MATCH "${OS_RELEASE_CONTENT}")

    if (DISTRO_MATCH)
        string(REGEX REPLACE "ID=\"?([a-zA-Z0-9_\-]+)\"?" "\\1" DISTRO "${DISTRO_MATCH}")
        string(STRIP "${DISTRO}" DISTRO) # Remove any whitespace
        string(REGEX REPLACE "^\"|\"$" "" DISTRO "${DISTRO}") # Remove any surrounding quotes
    else ()
        message(WARNING "Failed to determine Linux distribution from /etc/os-release")
        set(DISTRO "") # Set DISTRO to a blank value
    endif ()
endif ()

message(STATUS "Detected distribution: ${DISTRO}")

# # Enable testing
if (AIRTREE_ENABLE_TESTING)
    message(STATUS "Testing enabled. Adding tests.")
    enable_testing()
    add_subdirectory(tests)
else ()
    message(STATUS "Testing disabled. Skipping tests.")
endif ()

add_subdirectory(airtree-util)
add_subdirectory(airtree-core)
add_subdirectory(airtree-query)
add_subdirectory(airtree-reader)
add_subdirectory(airtree-merge)
add_subdirectory(airtree-export)
if (AIRTREE_BUILD_EXECUTABLES)
add_subdirectory(airtree-cli)
endif ()
if (AIRTREE_ENABLE_BENCHMARKS)
    add_subdirectory(airtree-bench)
endif ()

# Packaging / CPack config
include(${CMAKE_SOURCE_DIR}/cmake/Packaging.cmake)

