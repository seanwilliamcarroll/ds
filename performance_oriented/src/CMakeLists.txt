add_executable(hash_map_test hash_map_test.cpp)
target_link_libraries(hash_map_test GTest::gtest_main)
add_test(NAME hash_map COMMAND hash_map_test)
