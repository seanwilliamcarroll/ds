add_executable(union_find_test union_find_test.cpp)
target_link_libraries(union_find_test GTest::gtest_main)
add_test(NAME union_find COMMAND union_find_test)
