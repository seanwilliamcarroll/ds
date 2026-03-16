add_executable(coin_change_test coin_change_test.cpp)
target_link_libraries(coin_change_test GTest::gtest_main)
add_test(NAME coin_change COMMAND coin_change_test)
