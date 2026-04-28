cmake_minimum_required(VERSION 3.22.1)

file(STRINGS "${CMAKE_SOURCE_DIR}/version.txt" PROJECT_VERSION)

# Extract numeric version for CMake (remove any suffix like -SNAPSHOT)
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" PROJECT_VERSION_NUMERIC "${PROJECT_VERSION}")

project(FloatingPointHistogram VERSION ${PROJECT_VERSION_NUMERIC} LANGUAGES CXX)
add_compile_definitions(VERSION_NUMBER="${PROJECT_VERSION}")
message(STATUS "Building version ${PROJECT_VERSION_NUMERIC} of AirTree")

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
if(NOT AIRTREE_SANITIZER_USES_ASAN)
    include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_mimalloc.cmake)
endif()
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_openssl.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_boost.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_thrift.cmake)
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
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_hdf5.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/third-party/Dependency_netcdf4.cmake)

if (APPLE)
    # For macOS, ensure that GCC-14 is used
    message(STATUS "Setting GCC 14 for macOS")
    # Find GCC and G++ version 14
    find_program(GCC_PATH gcc-14)
    find_program(GPP_PATH g++-14)
    # If GCC or G++ are not found, throw an error
    if (NOT GCC_PATH OR NOT GPP_PATH)
        message(FATAL_ERROR "GCC-14 and/or G++-14 not found! Ensure GCC is installed and available in your PATH.")
    endif ()
    # Set the compilers
    set(CMAKE_C_COMPILER "${GCC_PATH}")
    set(CMAKE_CXX_COMPILER "${GPP_PATH}")
    message(STATUS "Using GCC as C compiler: ${CMAKE_C_COMPILER}")
    message(STATUS "Using G++ as C++ compiler: ${CMAKE_CXX_COMPILER}")
elseif (UNIX AND NOT APPLE)
    # For Ubuntu (assuming default GCC/G++ installations)
    message(STATUS "Setting default GCC/G++ for Ubuntu")
    # Find default GCC and G++
    find_program(GCC_PATH gcc)
    find_program(GPP_PATH g++)
    # If GCC or G++ are not found, throw an error
    if (NOT GCC_PATH OR NOT GPP_PATH)
        message(FATAL_ERROR "GCC and/or G++ not found! Ensure GCC is installed and available in your PATH.")
    endif ()
    # Set the compilers
    # set(CMAKE_C_COMPILER "${GCC_PATH}")
    # set(CMAKE_CXX_COMPILER "${GPP_PATH}")
    # message(STATUS "Using GCC as C compiler: ${CMAKE_C_COMPILER}")
    # message(STATUS "Using G++ as C++ compiler: ${CMAKE_CXX_COMPILER}")
elseif (WIN32)
    message(STATUS "Using default MSVC compiler for Windows.")
    set(OS "WINDOWS")
    find_program(MSVC_CL cl)
    find_program(MSVC_LINK link)
    if (NOT MSVC_CL OR NOT MSVC_LINK)
        message(FATAL_ERROR "MSVC compiler tools not found! Ensure Visual Studio Build Tools are installed and available in your PATH.")
    endif ()
    message(STATUS "Using MSVC CL as C/C++ compiler: ${MSVC_CL}")
else ()
    message(FATAL_ERROR "Unsupported OS or platform.")
endif ()

# Detect architecture and define preprocessor directives
if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
    add_compile_definitions(x86_ARCH)
    set(ARCH "x86_64")
    message(STATUS "Detected architecture: x86_64. Defining x86_ARCH.")
elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64" OR CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
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
if (AIRTREE_BUILD_EXECUTABLES)
add_subdirectory(airtree-cli)
add_subdirectory(airtree-export)
endif ()
if (AIRTREE_ENABLE_BENCHMARKS)
    add_subdirectory(airtree-bench)
endif ()

# Package
find_program(LSB_RELEASE_COMMAND lsb_release)
execute_process(COMMAND ${LSB_RELEASE_COMMAND} -is
    OUTPUT_VARIABLE LSB_RELEASE_ID_SHORT
    OUTPUT_STRIP_TRAILING_WHITESPACE)

string(TIMESTAMP CURRENT_DATE "%Y%m%d")
string(TIMESTAMP SNAPSHOT_TIMESTAMP "%Y%m%d-%H%M%S")

if ("${LSB_RELEASE_ID_SHORT}" MATCHES "Debian" OR "${LSB_RELEASE_ID_SHORT}" MATCHES "Ubuntu")
  set(CPACK_GENERATOR "DEB;TGZ")
elseif ("${LSB_RELEASE_ID_SHORT}" MATCHES "CentOS" OR "${LSB_RELEASE_ID_SHORT}" MATCHES "CentOSStream")
  set(CPACK_GENERATOR "RPM;TGZ")
endif ()

# Set default install prefix if not specified
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/opt/airmettle/airtree/${PROJECT_VERSION}" CACHE PATH "Install path prefix" FORCE)
endif()

# CPack configuration
set(CPACK_PACKAGE_NAME "airtree")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "support@airmettle.com")
set(CPACK_PACKAGE_DESCRIPTION "AirMettle AirTree - Hierarchical Multi Dimensional Histograms")
set(CPACK_PACKAGE_VENDOR "AirMettle")
set(CPACK_WARN_ON_ABSOLUTE_INSTALL_DESTINATION TRUE)
set(CPACK_STRIP_FILES TRUE)
set(CPACK_PACKAGE_CHECKSUM SHA256)

include(CPack)
include(InstallRequiredSystemLibraries)

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

