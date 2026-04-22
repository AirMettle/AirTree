# benchmark/CMakeLists.txt

# Platform-specific configurations
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "Configuring for macOS.")
    set(OS "DARWIN")
    set(SNAPPY_INCLUDE_DIRS "/opt/homebrew/opt/snappy/include")
    set(SNAPPY_LIBRARY_DIRS "/opt/homebrew/opt/snappy/lib")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(OS "LINUX")
    message(STATUS "Configuring for Ubuntu/Linux.")
    pkg_check_modules(SNAPPY REQUIRED snappy)
else ()
    message(FATAL_ERROR "Unsupported OS")
endif ()

# Include directories
include_directories(SYSTEM
        ${SNAPPY_INCLUDE_DIRS}
)

# Link directories
link_directories(
        ${SNAPPY_LIBRARY_DIRS}
)

set(BENCHMARK_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

add_executable(airtree_bench 
    ${BENCHMARK_DIR}/include/airtree/bench/generate/trie1d/1DxT.hpp 
    ${BENCHMARK_DIR}/src/generate/trie1d/1DxT.cpp 
    ${BENCHMARK_DIR}/include/airtree/bench/generate/trie1d/1DxF.hpp 
    ${BENCHMARK_DIR}/src/generate/trie1d/1DxF.cpp 
    ${BENCHMARK_DIR}/include/airtree/bench/generate/trie1d/1DxP.hpp 
    ${BENCHMARK_DIR}/src/generate/trie1d/1DxP.cpp 

    ${BENCHMARK_DIR}/include/airtree/bench/generate/trie2d/2DxF.hpp 
    ${BENCHMARK_DIR}/src/generate/trie2d/2DxF.cpp 
    ${BENCHMARK_DIR}/include/airtree/bench/generate/trie2d/2DxP.hpp 
    ${BENCHMARK_DIR}/src/generate/trie2d/2DxP.cpp 

    ${BENCHMARK_DIR}/include/airtree/bench/generate/trie3d/3DxF.hpp 
    ${BENCHMARK_DIR}/src/generate/trie3d/3DxF.cpp 
    ${BENCHMARK_DIR}/include/airtree/bench/generate/trie3d/3DxP.hpp 
    ${BENCHMARK_DIR}/src/generate/trie3d/3DxP.cpp 

    ${BENCHMARK_DIR}/include/airtree/bench/generate/trie4d/4DxF.hpp 
    ${BENCHMARK_DIR}/src/generate/trie4d/4DxF.cpp 
    ${BENCHMARK_DIR}/include/airtree/bench/generate/trie4d/4DxP.hpp 
    ${BENCHMARK_DIR}/src/generate/trie4d/4DxP.cpp 

    ${BENCHMARK_DIR}/include/airtree/bench/BenchmarkData.hpp
    ${BENCHMARK_DIR}/include/airtree/bench/Logger.hpp
    ${BENCHMARK_DIR}/src/Main.cpp
    ${BENCHMARK_DIR}/src/Logger.cpp
    include/airtree/bench/BenchmarkConfigs.hpp
)

if(TARGET am_airtree_dependencies)
    add_dependencies(airtree_bench am_airtree_dependencies)
endif()

target_include_directories(airtree_bench 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(airtree_bench PRIVATE 
    airtree::airtree-util 
    airtree::airtree-reader
    airtree::airtree-core 
    spdlog::spdlog
    netcdf::netcdf  
    hdf5::hdf5_hl 
    hdf5::hdf5 
    benchmark::benchmark
    cli11_headers
)
