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
