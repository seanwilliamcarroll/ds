add_executable(coin_change_test coin_change_test.cpp)
target_link_libraries(coin_change_test GTest::gtest_main)
add_test(NAME coin_change COMMAND coin_change_test)

add_executable(house_robber_test house_robber_test.cpp)
target_link_libraries(house_robber_test GTest::gtest_main)
add_test(NAME house_robber COMMAND house_robber_test)
