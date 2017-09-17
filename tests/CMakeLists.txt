file (GLOB units_sources "*.cpp")

add_executable (units ${units_sources})
target_include_directories(units SYSTEM
  PUBLIC "${gtest_SOURCE_DIR}/include"
  PUBLIC "${gmock_SOURCE_DIR}/include")
target_link_libraries (units gtest pthread)
