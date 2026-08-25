set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)


set(SOURCE_FILES 
    ${SRC_DIR}
)

add_library(airtree-query STATIC
    ${SOURCE_FILES}/Logger.cpp
    # meta module
    ${SOURCE_FILES}/meta/Histogram.cpp
    # percentile module
    ${SOURCE_FILES}/percentile/Percentile.cpp
    # topK module
    ${SOURCE_FILES}/topk/TopK.cpp
    # bounding box module
    ${SOURCE_FILES}/bounding-box/BoundingBox.cpp
    # minmax module
    ${SOURCE_FILES}/minmax/MinMax.cpp
    # bin boundaries module
    ${SOURCE_FILES}/bin-boundary/BinBoundary.cpp
    # cdf module
    ${SOURCE_FILES}/cdf/CDF.cpp
    # grid module
    ${SOURCE_FILES}/grid/GridQuery.cpp
)

target_include_directories(airtree-query
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)


target_link_libraries(airtree-query 
    PUBLIC 
    airtree::airtree-util 
    airtree::airtree-core
    PRIVATE
    # No private dependencies
)

add_library(airtree::airtree-query ALIAS airtree-query)

if(AIRTREE_ENABLE_TESTING)
# Unit tests
add_executable(ut_query_test 
    tests/TestBoundingBox.cpp 
    tests/TestBinBoundaries.cpp 
    tests/TestCDF.cpp
    tests/TestFuzzEncoding.cpp
    tests/TestMinMax.cpp
    tests/TestPercentile.cpp
    tests/TestTopK.cpp
    tests/TestGridQuery.cpp
    tests/TestSparseWalk.cpp)

target_link_libraries(ut_query_test PRIVATE 
    airtree::airtree-util 
    airtree::airtree-core 
    airtree::airtree-query 
    gtest::gtest 
    gtest::gtest_main 
)

add_test(NAME ut_query_test COMMAND ut_query_test)
endif()

# Install targets
install(TARGETS airtree-query
    ARCHIVE DESTINATION lib
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)
