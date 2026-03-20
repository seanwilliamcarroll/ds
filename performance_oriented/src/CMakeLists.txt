add_executable(hash_map_test hash_map_test.cpp)
target_link_libraries(hash_map_test GTest::gtest_main)
add_test(NAME hash_map COMMAND hash_map_test)

add_executable(hash_map_bench hash_map_bench.cpp)
target_link_libraries(hash_map_bench benchmark::benchmark)
