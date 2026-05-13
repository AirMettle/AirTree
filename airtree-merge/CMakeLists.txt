set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(libairtree-merge STATIC
    ${SRC_DIR}/Logger.cpp
    # Dim1
    ${SRC_DIR}/trie1d/Merge1DxT.cpp
    ${SRC_DIR}/trie1d/Merge1DxF.cpp
    ${SRC_DIR}/trie1d/Merge1DxP.cpp
    # Dim2
    ${SRC_DIR}/trie2d/Merge2DxF.cpp
    ${SRC_DIR}/trie2d/Merge2DxP.cpp
    # Dim3
    ${SRC_DIR}/trie3d/Merge3DxF.cpp
    ${SRC_DIR}/trie3d/Merge3DxP.cpp
    # Dim4
    ${SRC_DIR}/trie4d/Merge4DxF.cpp
    ${SRC_DIR}/trie4d/Merge4DxP.cpp

    ${SRC_DIR}/MergeFactory.cpp
    ${SRC_DIR}/MergeStrategy.cpp
    ${SRC_DIR}/AirTreeMerge.cpp
)

set_target_properties(libairtree-merge PROPERTIES OUTPUT_NAME airtree-merge)

target_include_directories(libairtree-merge
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(libairtree-merge
    PUBLIC
    airtree::airtree-util
    airtree::airtree-core
    PRIVATE
    # No private dependencies
)

add_library(airtree::libairtree-merge ALIAS libairtree-merge)

if (AIRTREE_BUILD_EXECUTABLES)
add_executable(airtree-merge ${SRC_DIR}/main.cpp)

target_link_libraries(airtree-merge
    PUBLIC
    # Add exported dependencies
    PRIVATE
    airtree::libairtree-merge
)
endif ()

if(AIRTREE_ENABLE_TESTING)
# Unit tests
add_executable(ut_merge_test
    tests/TestMergeFunction.cpp
)

target_compile_definitions(ut_merge_test PRIVATE UNIT_TEST)

target_link_libraries(ut_merge_test PRIVATE
    airtree::airtree-util
    airtree::airtree-core
    airtree::libairtree-merge
    gtest::gtest
    gtest::gtest_main
)

add_test(NAME ut_merge_test COMMAND ut_merge_test)
endif()
# Install targets
install(TARGETS libairtree-merge
    ARCHIVE DESTINATION lib
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

if (AIRTREE_BUILD_EXECUTABLES)
install(TARGETS airtree-merge
    RUNTIME DESTINATION bin
)
endif ()
