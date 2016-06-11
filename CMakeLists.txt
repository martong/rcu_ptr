# CMakeLists.txt
#
cmake_minimum_required (VERSION 2.8.12.2)

if(${_CRF_SANITIZE_BUILD} STREQUAL "tsan")
  set (CXX_SANITIZER_FLAGS "-fsanitize=thread -fno-omit-frame-pointer")
  message ("-- Making ThreadSanitize-d Build")
elseif(${_CRF_SANITIZE_BUILD} STREQUAL "asan")
  set (CXX_SANITIZER_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
  message ("-- Making AddressSanitize-d Build")
else(${_CRF_SANITIZE_BUILD} STREQUAL "tsan")
  set (CXX_SANITIZER_FLAGS "-fsanitize=thread -fno-omit-frame-pointer")
endif(${_CRF_SANITIZE_BUILD} STREQUAL "tsan")


set (CXX_STANDARD "-std=c++14")
set (CXX_COMMON_FLAGS "${CXX_STANDARD} -Wall -Wextra -Wdeprecated -pedantic ${CXX_SANITIZER_FLAGS}")
set (CMAKE_CXX_FLAGS_DEBUG "${CXX_COMMON_FLAGS} -O0 -ggdb")
set (CMAKE_CXX_FLAGS_RELEASE "${CXX_COMMON_FLAGS} -O3")
include_directories ("${PROJECT_SOURCE_DIR}")

add_subdirectory (unit_test)
add_subdirectory (race_test)

message ("-- Building ${CMAKE_BUILD_TYPE} build")
message ("-- Installing to ${_CRF_INSTALL_HOME}")

install (DIRECTORY examples/lib DESTINATION "${_CRF_INSTALL_HOME}/include"
         FILES_MATCHING PATTERN "*.hpp")

