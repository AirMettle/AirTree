
# Define source files
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SOURCE_FILES
    ${SRC_DIR}/Logger.cpp
    ${SRC_DIR}/common/BitCodec.cpp
    ${SRC_DIR}/common/BitInterleave.cpp
    ${SRC_DIR}/common/BooleanArray.cpp
    ${SRC_DIR}/common/Conversion.cpp
    ${SRC_DIR}/common/FPHArray.cpp
    ${SRC_DIR}/common/InternalEncoding.cpp
    ${SRC_DIR}/common/NDims.cpp
    ${SRC_DIR}/common/Reconstruct.cpp
    ${SRC_DIR}/common/SpecialCounts.cpp
    ${SRC_DIR}/common/TLE.cpp

    ${SRC_DIR}/io/AirTreeReader.cpp
    ${SRC_DIR}/io/AirTreeWriter.cpp

    ${SRC_DIR}/schema/trie1d/1DxF.cpp
    ${SRC_DIR}/schema/trie1d/1DxP.cpp
    ${SRC_DIR}/schema/trie1d/1DxT.cpp
    ${SRC_DIR}/schema/trie2d/2DxF.cpp
    ${SRC_DIR}/schema/trie2d/2DxP.cpp
    ${SRC_DIR}/schema/trie3d/3DxF.cpp
    ${SRC_DIR}/schema/trie3d/3DxP.cpp
    ${SRC_DIR}/schema/trie4d/4DxP.cpp
    ${SRC_DIR}/schema/trie4d/4DxF.cpp

    ${SRC_DIR}/serdes/BooleanArray.cpp
    ${SRC_DIR}/serdes/Count.cpp
    ${SRC_DIR}/serdes/EOF.cpp
    ${SRC_DIR}/serdes/ND.cpp
    ${SRC_DIR}/serdes/trie1d/1DxF.cpp
    ${SRC_DIR}/serdes/trie1d/1DxP.cpp
    ${SRC_DIR}/serdes/trie1d/1DxT.cpp
    ${SRC_DIR}/serdes/trie2d/2DxF.cpp
    ${SRC_DIR}/serdes/trie2d/2DxP.cpp
    ${SRC_DIR}/serdes/trie3d/3DxF.cpp
    ${SRC_DIR}/serdes/trie3d/3DxP.cpp
    ${SRC_DIR}/serdes/trie4d/4DxP.cpp
    ${SRC_DIR}/serdes/trie4d/4DxF.cpp

    ${SRC_DIR}/utils/TrieManager.cpp
    ${SRC_DIR}/utils/Utils.cpp

    ${SRC_DIR}/api/AirTreeGenerator.cpp

    ${SRC_DIR}/common/ConfigRegistry.cpp
    ${SRC_DIR}/common/AirTreeHeader.cpp
)

# Create the library
add_library(airtree-core STATIC ${SOURCE_FILES})

target_include_directories(airtree-core 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Link libraries
target_link_libraries(airtree-core 
    PUBLIC 
    spdlog::spdlog
    PRIVATE 
    airtree::airtree-util 
)

# Create alias
add_library(airtree::airtree-core ALIAS airtree-core)

if(AIRTREE_ENABLE_TESTING)
# Unit tests
add_executable(ut_core 
    tests/TestCombineChunks.cpp 
    tests/TestFpInternal_1D.cpp
    tests/TestFpInternalManual.cpp
    tests/TestAirTreeHeader.cpp
    tests/TestHeaderDeserialization.cpp
    tests/TestInterleave.cpp
    tests/TestReconstuctionOrignalFP.cpp
    tests/TestSerialization.cpp
    tests/TestSpecialCase.cpp
    tests/TestTrieManager.cpp
    tests/TestUtilities.cpp
)

target_link_libraries(ut_core PRIVATE 
    airtree::airtree-util 
    airtree::airtree-core 
    gtest::gtest 
    gtest::gtest_main 
)

add_test(NAME ut_core COMMAND ut_core)
endif()

# Install targets
install(TARGETS airtree-core
    ARCHIVE DESTINATION lib
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)
