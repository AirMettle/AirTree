set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(libairtree-export STATIC
    ${SRC_DIR}/Logger.cpp
    ${SRC_DIR}/AirTreeExporter.cpp

    ${SRC_DIR}/core/ArrowTableBuilder.cpp
    ${SRC_DIR}/writer/ArrowWriter.cpp
    )

set_target_properties(libairtree-export PROPERTIES OUTPUT_NAME airtree-export)

target_link_libraries(libairtree-export
    PUBLIC
    arrow::arrow_static
    parquet::parquet
    PRIVATE
    airtree::airtree-util
    airtree::airtree-core
    airtree::airtree-query
)

target_include_directories(libairtree-export
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
add_library(airtree::libairtree-export ALIAS libairtree-export)

add_executable(airtree-export ${SRC_DIR}/ExportCLI.cpp)

target_link_libraries(airtree-export
    PUBLIC
    arrow::arrow_static
    parquet::parquet
    PRIVATE
    airtree::libairtree-export
    airtree::airtree-util
)


# TODO : KEEPING THIS FOR NOW
# REMOVE THIS LEGACY EXPORT ONCE 10 bit COMPACT HAS BEEN IMPLEMENTED
add_executable(legacyExport ${SRC_DIR}/LegacyExport.cpp)
target_link_libraries(legacyExport
    PUBLIC
    arrow::arrow_static
    parquet::parquet
    PRIVATE
    airtree::airtree-util
    airtree::airtree-core
    airtree::airtree-query
)


### TESTS
if(AIRTREE_ENABLE_TESTING)
add_executable(ut_testexport tests/TestExport.cpp)
target_link_libraries(ut_testexport
    PRIVATE
    airtree::airtree-util
    airtree::airtree-core
    airtree::airtree-query
    airtree::libairtree-export
    gtest::gtest
    gtest::gtest_main)
add_test(NAME ut_testexport COMMAND ut_testexport)
endif()

# Install targets
install(TARGETS libairtree-export
    ARCHIVE DESTINATION lib
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# Install export utilities
install(TARGETS airtree-export
    RUNTIME DESTINATION bin
)
