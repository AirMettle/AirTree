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

