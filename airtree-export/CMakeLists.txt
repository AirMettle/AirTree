set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_airtree_module(airtree-export STATIC 
    ${SRC_DIR}/Logger.cpp
    ${SRC_DIR}/AirTreeExporter.cpp

    ${SRC_DIR}/core/ArrowTableBuilder.cpp
    ${SRC_DIR}/writer/ArrowWriter.cpp
    )

target_link_libraries(airtree-export 
    PUBLIC
    arrow::arrow_static 
    parquet::parquet 
    snappy
    PRIVATE
    airtree::airtree-util 
    airtree::airtree-core 
    airtree::airtree-query
)

target_include_directories(airtree-export
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
add_library(airtree::airtree-export ALIAS airtree-export)

add_executable(AirTreeExport ${SRC_DIR}/ExportCLI.cpp)

target_link_libraries(AirTreeExport 
    PUBLIC
    # No public dependencies
    PRIVATE
    airtree::airtree-export
)


# TODO : KEEPING THIS FOR NOW 
# REMOVE THIS LEGACY EXPORT ONCE 10 bit COMPACT HAS BEEN IMPLEMENTED
add_executable(legacyExport ${SRC_DIR}/LegacyExport.cpp)
target_link_libraries(legacyExport
    PUBLIC
    arrow::arrow_static 
    parquet::parquet 
    snappy
    PRIVATE 
    airtree::airtree-util 
    airtree::airtree-core 
    airtree::airtree-query
)


### TESTS

add_executable(ut_testexport tests/TestExport.cpp)
target_link_libraries(ut_testexport PRIVATE airtree::airtree-util airtree::airtree-core airtree::airtree-query airtree::airtree-export gtest::gtest gtest::gtest_main)
add_test(NAME ut_testexport COMMAND ut_testexport)

# Install targets
install(TARGETS airtree-export
    ARCHIVE DESTINATION lib
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# Install export utilities
install(TARGETS AirTreeExport
    RUNTIME DESTINATION bin
)
