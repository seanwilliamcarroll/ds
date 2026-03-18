add_executable(coin_change_test coin_change_test.cpp)
target_link_libraries(coin_change_test GTest::gtest_main)
add_test(NAME coin_change COMMAND coin_change_test)

add_executable(house_robber_test house_robber_test.cpp)
target_link_libraries(house_robber_test GTest::gtest_main)
add_test(NAME house_robber COMMAND house_robber_test)

add_executable(min_path_sum_test min_path_sum_test.cpp)
target_link_libraries(min_path_sum_test GTest::gtest_main)
add_test(NAME min_path_sum COMMAND min_path_sum_test)

add_executable(number_of_islands_test number_of_islands_test.cpp)
target_link_libraries(number_of_islands_test GTest::gtest_main)
add_test(NAME number_of_islands COMMAND number_of_islands_test)

add_executable(clone_graph_test clone_graph_test.cpp)
target_link_libraries(clone_graph_test GTest::gtest_main)
add_test(NAME clone_graph COMMAND clone_graph_test)

add_executable(course_schedule_test course_schedule_test.cpp)
target_link_libraries(course_schedule_test GTest::gtest_main)
add_test(NAME course_schedule COMMAND course_schedule_test)

add_executable(network_delay_time_test network_delay_time_test.cpp)
target_link_libraries(network_delay_time_test GTest::gtest_main)
add_test(NAME network_delay_time COMMAND network_delay_time_test)

add_executable(min_heap_test min_heap_test.cpp)
target_link_libraries(min_heap_test GTest::gtest_main)
add_test(NAME min_heap COMMAND min_heap_test)

add_executable(network_delay_time_bench network_delay_time_bench.cpp)
target_link_libraries(network_delay_time_bench benchmark::benchmark)
