# CMakeOptions.cmake — User-facing build options
#
# Toggle these to enable/disable features. Pass via CLI:
#   cmake -DENABLE_TESTING=OFF -S . -B build

option(AIRTREE_ENABLE_TESTING           "Enable testing"                             ON)
option(AIRTREE_ENABLE_BENCHMARKS        "Enable benchmarks"                          ON)
option(AIRTREE_BUILD_EXECUTABLES        "Build CLI tools and legacy executables"     ON)
option(PACKAGE_FPH                      "Generate the package for the target system" OFF)
option(BUILD_SHARED_LIBS                "Build shared libraries"                     ON)
option(AIRMETTLE_AIRTREE_USE_SHARED_LIBS "Use shared libraries for dependencies" OFF)

# Sanitizer mode: none, asan, ubsan, asan_ubsan
set(AIRTREE_SANITIZER "nosan" CACHE STRING "Enable sanitizer instrumentation (nosan, asan, ubsan, asan_ubsan)")
set_property(CACHE AIRTREE_SANITIZER PROPERTY STRINGS nosan asan ubsan asan_ubsan)

# C++ log level (spdlog). Empty string means auto-detect from build type.
# Options: trace, debug, info, warn, error, critical, or empty for auto
set(AIRTREE_LOG_LEVEL "" CACHE STRING "C++ log level (trace, debug, info, warn, error, critical). Empty = auto from build type")
set_property(CACHE AIRTREE_LOG_LEVEL PROPERTY STRINGS "" trace debug info warn error critical)

# Dependency binary cache (S3). Bucket/region/prefix configured in Settings.cmake.
option(AIRTREE_DEPS_CACHE_ENABLED "Enable S3 binary cache for dependencies" ON)
