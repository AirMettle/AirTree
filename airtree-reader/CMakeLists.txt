
# Define source files
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SOURCE_FILES
    ${SRC_DIR}/Logger.cpp

    ${SRC_DIR}/file/Reader.cpp
    ${SRC_DIR}/file/BinaryReader.cpp
    ${SRC_DIR}/file/ParquetReader.cpp
    ${SRC_DIR}/file/CSVReader.cpp
)

# Create the library
add_airtree_module(airtree-reader STATIC ${SOURCE_FILES})
    
target_include_directories(airtree-reader
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link libraries
target_link_libraries(airtree-reader
    PUBLIC
    arrow::arrow
    parquet::parquet 
    snappy 
    openssl::openssl 
    openssl::crypto 
    spdlog::spdlog
    PRIVATE 
    airtree::airtree-util
)

# Create alias
add_library(airtree::airtree-reader ALIAS airtree-reader)

# Install targets
install(TARGETS airtree-reader
    ARCHIVE DESTINATION lib
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# Tests
if(ENABLE_TESTING)
    # Binary Reader Tests
    add_executable(ut_binary_reader_test tests/TestBinaryReader.cpp)
    target_link_libraries(ut_binary_reader_test PRIVATE
        airtree::airtree-reader
        airtree::airtree-util
        gtest::gtest
        gtest::gtest_main)
    add_test(NAME ut_binary_reader_test COMMAND ut_binary_reader_test)

    # Parquet Reader Tests
    add_executable(ut_parquet_reader_test tests/TestParquetReader.cpp)
    target_link_libraries(ut_parquet_reader_test PRIVATE
        airtree::airtree-reader
        airtree::airtree-util
        gtest::gtest
        gtest::gtest_main)
    add_test(NAME ut_parquet_reader_test COMMAND ut_parquet_reader_test)

    # CSV Reader Tests
    add_executable(ut_csv_reader_test tests/TestCSVReader.cpp)
    target_link_libraries(ut_csv_reader_test PRIVATE
        airtree::airtree-reader
        airtree::airtree-util
        gtest::gtest
        gtest::gtest_main)
    add_test(NAME ut_csv_reader_test COMMAND ut_csv_reader_test)
endif()
