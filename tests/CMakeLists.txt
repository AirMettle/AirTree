# # FUNCTIONAL TESTS HERE
add_executable(fn_1D_test FnTest/1-D.cpp)
target_link_libraries(fn_1D_test PRIVATE 
    airtree::airtree-util 
    airtree::airtree-core 
    airtree::airtree-query 
    gtest::gtest 
    gtest::gtest_main 
)
add_test(NAME fn_1D_test COMMAND fn_1D_test)


add_executable(fn_2D_test FnTest/2-D.cpp)
target_link_libraries(fn_2D_test PRIVATE 
    airtree::airtree-util 
    airtree::airtree-core 
    airtree::airtree-query 
    gtest::gtest 
    gtest::gtest_main 
)
add_test(NAME fn_2D_test COMMAND fn_2D_test)


add_executable(fn_3D_test FnTest/3-D.cpp)
target_link_libraries(fn_3D_test PRIVATE 
    airtree::airtree-util 
    airtree::airtree-core 
    airtree::airtree-query 
    gtest::gtest 
    gtest::gtest_main 
)
add_test(NAME fn_3D_test COMMAND fn_3D_test)


add_executable(fn_4D_test FnTest/4-D.cpp)
target_link_libraries(fn_4D_test PRIVATE
    airtree::airtree-util
    airtree::airtree-core
    airtree::airtree-query
    gtest::gtest
    gtest::gtest_main
)
add_test(NAME fn_4D_test COMMAND fn_4D_test)


# Public-header self-contained tests.
# Each public header in docs/cpp-api.md "Headers at a Glance" must compile
# in isolation with no other includes. This sweep catches missing system
# header includes early.
set(_airtree_public_header_targets
    AirTreeGenerator
    Percentile
    TopK
    MinMax
    CDF
    BoundingBox
    BinBoundary
    AirTreeExporter
    AirTreeMerge
    Reader
)
set(_airtree_public_header_libs
    airtree::airtree-core
    airtree::airtree-query
    airtree::airtree-export
    airtree::airtree-merge
    airtree::airtree-reader
    airtree::airtree-util
)
foreach(_h IN LISTS _airtree_public_header_targets)
    add_executable(header_${_h}_self_contained cpp-api/header_${_h}.cpp)
    target_link_libraries(header_${_h}_self_contained PRIVATE
        ${_airtree_public_header_libs})
    add_test(NAME header_${_h}_self_contained
             COMMAND header_${_h}_self_contained)
endforeach()

