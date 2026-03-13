add_executable(union_find_test union_find_test.cpp)
target_link_libraries(union_find_test GTest::gtest_main)
add_test(NAME union_find COMMAND union_find_test)

add_executable(union_find_bench union_find_bench.cpp)
target_link_libraries(union_find_bench benchmark::benchmark)

add_executable(redundant_connection_test redundant_connection_test.cpp)
target_link_libraries(redundant_connection_test GTest::gtest_main)
add_test(NAME redundant_connection COMMAND redundant_connection_test)

add_executable(trie_test trie_test.cpp)
target_link_libraries(trie_test GTest::gtest_main)
add_test(NAME trie COMMAND trie_test)

add_executable(trie_bench trie_bench.cpp)
target_link_libraries(trie_bench benchmark::benchmark)

add_executable(word_search_test word_search_test.cpp)
target_link_libraries(word_search_test GTest::gtest_main)
add_test(NAME word_search COMMAND word_search_test)
