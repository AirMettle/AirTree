# Configure generated directory and files
set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/include/gen-src/include)

# Resolve log level: explicit setting or build-type default
if(AIRTREE_LOG_LEVEL STREQUAL "")
    # Auto-detect from build type
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_LOG_LEVEL "trace")
    elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(_LOG_LEVEL "info")
    else()
        # Release, MinSizeRel, Benchmark
        set(_LOG_LEVEL "warn")
    endif()
else()
    set(_LOG_LEVEL "${AIRTREE_LOG_LEVEL}")
endif()

# Map level name to spdlog constant
string(TOLOWER "${_LOG_LEVEL}" _LOG_LEVEL)
if(_LOG_LEVEL STREQUAL "trace")
    set(SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE)
elseif(_LOG_LEVEL STREQUAL "debug")
    set(SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG)
elseif(_LOG_LEVEL STREQUAL "info")
    set(SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO)
elseif(_LOG_LEVEL STREQUAL "warn")
    set(SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_WARN)
elseif(_LOG_LEVEL STREQUAL "error")
    set(SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_ERROR)
elseif(_LOG_LEVEL STREQUAL "critical")
    set(SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_CRITICAL)
else()
    message(FATAL_ERROR "Unknown AIRTREE_LOG_LEVEL: '${_LOG_LEVEL}'. Use: trace, debug, info, warn, error, critical")
endif()

message(STATUS "C++ log level: ${_LOG_LEVEL} (${SPDLOG_ACTIVE_LEVEL})")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/airtree/util/logging/LoggingConfig.hpp.in"
    "${GEN_DIR}/airtree/util/logging/LoggingConfig.hpp"
)

# Set up include directories
set(AIRTREE_UTIL_INC_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include  # source headers (airtree/util/...)
    ${GEN_DIR}                           # generated headers
)

# Define source files
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SOURCE_FILES
    ${SRC_DIR}/logging/Logging.cpp
    ${SRC_DIR}/UUID.cpp
)

# Create the library
add_airtree_module(airtree-util STATIC ${SOURCE_FILES})

# Set target include directories
target_include_directories(airtree-util PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${GEN_DIR}>
    $<INSTALL_INTERFACE:include>
)
# Link libraries
target_link_libraries(airtree-util PRIVATE spdlog::spdlog boost_headers)
target_link_libraries(airtree-util PUBLIC boost_headers)

# Create alias
add_library(airtree::airtree-util ALIAS airtree-util)

# Install targets
install(TARGETS airtree-util
    ARCHIVE DESTINATION lib
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# Install generated headers
install(DIRECTORY ${GEN_DIR}/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)
