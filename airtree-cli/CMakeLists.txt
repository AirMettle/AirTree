# Define source files
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SOURCE_FILES
    ${SRC_DIR}/cli/AirTree.cpp
    ${SRC_DIR}/cli/CLI.cpp
    ${SRC_DIR}/cli/Logger.cpp
)

add_executable(airtree-cli ${SOURCE_FILES})

if(TARGET am_airtree_dependencies)
    add_dependencies(airtree-cli am_airtree_dependencies)
endif()

target_include_directories(airtree-cli 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(airtree-cli
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
install(TARGETS airtree-cli
    RUNTIME DESTINATION bin
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

if(AIRTREE_ENABLE_TESTING)
    add_test(NAME ut_cli_parquet_generate
        COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_parquet_generate.sh
            $<TARGET_FILE:airtree-cli>
            ${CMAKE_SOURCE_DIR}/airtree-reader/tests/testdata/test_double.parquet)
endif()
