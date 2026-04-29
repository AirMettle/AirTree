# Toolchain_msvc.cmake

message(STATUS "Loading Windows MSVC Toolchain...")

# Explicitly set the compilers to MSVC
set(CMAKE_C_COMPILER "cl.exe" CACHE STRING "C Compiler" FORCE)
set(CMAKE_CXX_COMPILER "cl.exe" CACHE STRING "CXX Compiler" FORCE)

# Explicitly set the system name
set(CMAKE_SYSTEM_NAME Windows)

# Chain-load the vcpkg toolchain for dependency management
set(VCPKG_PATH "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
if(EXISTS "${VCPKG_PATH}")
    message(STATUS "Chain-loading vcpkg toolchain from ${VCPKG_PATH}")
    include("${VCPKG_PATH}")
else()
    message(WARNING "vcpkg toolchain not found at ${VCPKG_PATH}. C++ dependencies may fail to resolve.")
endif()
