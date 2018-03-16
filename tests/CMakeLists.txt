add_executable (asp_wrapper_test std_asp_core.cpp std_asp_concurrent.cpp)
target_include_directories(asp_wrapper_test SYSTEM
  PUBLIC "${gtest_SOURCE_DIR}/include"
  PUBLIC "${gmock_SOURCE_DIR}/include")
target_link_libraries (asp_wrapper_test gtest_main pthread)
add_test(NAME asp_wrapper_test COMMAND asp_wrapper_test)

add_executable (rcu_ptr_test rcu_unit.cpp rcu_race.cpp)
target_include_directories(rcu_ptr_test SYSTEM
  PUBLIC "${gtest_SOURCE_DIR}/include"
  PUBLIC "${gmock_SOURCE_DIR}/include")
target_link_libraries (rcu_ptr_test gtest_main pthread)
add_test(NAME rcu_ptr_test COMMAND rcu_ptr_test)

add_executable (jss_rcu_ptr_test rcu_unit.cpp rcu_race.cpp)
target_include_directories(jss_rcu_ptr_test SYSTEM
  PUBLIC "${gtest_SOURCE_DIR}/include"
  PUBLIC "${gmock_SOURCE_DIR}/include")
target_link_libraries (jss_rcu_ptr_test gtest_main pthread ${ATOMICLIB})
target_compile_options(jss_rcu_ptr_test PRIVATE -DTEST_WITH_JSS_ASP)
add_test(NAME jss_rcu_ptr_test COMMAND jss_rcu_ptr_test)
