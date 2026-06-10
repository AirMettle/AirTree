# shellcheck shell=bash
# Build settings — override any variable via environment, e.g.:
#   BUILD_TYPE=Debug TOOLCHAIN=gnu-9 ./tools/build/partial_build.sh airtree-util

# Build type passed to CMake.
# Values: Debug, Release, RelWithDebInfo, MinSizeRel, Benchmark
BUILD_TYPE="${BUILD_TYPE:-RelWithDebInfo}"

# Sanitizer mode passed to CMake.
# Values: nosan, asan, ubsan, asan_ubsan
SANITIZER="${SANITIZER:-nosan}"

# Toolchain selector — maps to cmake/Toolchain_<name>.cmake (hyphens become underscores).
# Values: gnu-11, gnu-9, msvc (forced on Windows)
TOOLCHAIN="${TOOLCHAIN:-gnu-11}"

# CMake generator.
# Values: Ninja, Unix Makefiles
GENERATOR="${GENERATOR:-Ninja}"

# Windows always builds with MSVC + Ninja, overriding any env request. This must
# happen here, where TOOLCHAIN is first resolved, so every script (parent or
# child process) derives the same cmake-build-msvc-* directory name.
case "$(uname | tr '[:upper:]' '[:lower:]')" in
  *mingw* | *msys* | *cygwin*)
    TOOLCHAIN="msvc"
    GENERATOR="Ninja"
    ;;
esac

# Log level for shell scripts.
# Values: 0 (DEBUG), 1 (INFO), 2 (SUCCESS), 3 (WARNING), 4 (ERROR), 5 (SILENT)
LOG_LEVEL="${LOG_LEVEL:-1}"

# C++ log level (spdlog) passed to CMake.
# Empty = auto from build type (Debug→trace, RelWithDebInfo→info, Release→warn)
# Values: trace, debug, info, warn, error, critical
CPP_LOG_LEVEL="${CPP_LOG_LEVEL:-}"

# Dependency binary cache (S3). Bucket/region configured in cmake/CMakeOptions.cmake.
# Values: ON, OFF
AIRTREE_DEPS_CACHE_ENABLED="${AIRTREE_DEPS_CACHE_ENABLED:-ON}"
