# Define source files
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SOURCE_FILES
    ${SRC_DIR}/cli/AirTree.cpp
    ${SRC_DIR}/cli/CLI.cpp
    ${SRC_DIR}/cli/Logger.cpp
)

add_executable(airtree ${SOURCE_FILES})

if(TARGET am_airtree_dependencies)
    add_dependencies(airtree am_airtree_dependencies)
endif()

target_include_directories(airtree
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(airtree
    PUBLIC
    # Add exported dependencies
    PRIVATE
    airtree::airtree-util
    airtree::airtree-core
    airtree::airtree-query
    airtree::airtree-reader
    cli11_headers
    spdlog::spdlog
)

# Install targets
install(TARGETS airtree
    RUNTIME DESTINATION bin
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)
