# CMakeLists.txt
#
cmake_minimum_required (VERSION 2.8.12.2)

# dispatch linking ThreadSanitizer lib based
# on target platform
#
function (set_libtsan_link_flag)
message ("-- Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
message ("-- Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.3.0")
  set (_CRF_LIBTSAN_LINK_FLAG "-static-libtsan")
else(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.3.0")
  set (_CRF_LIBTSAN_LINK_FALG "")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.3.0")
endfunction ()


# setup sanitized build
#
if(${_CRF_SANITIZE_BUILD} STREQUAL "msan")
  set (CXX_SANITIZER_FLAGS "-fsanitize=memory -fno-omit-frame-pointer")
  message ("-- Making MemorySanitize-d Build")
elseif(${_CRF_SANITIZE_BUILD} STREQUAL "asan")
  set (CXX_SANITIZER_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
  message ("-- Making AddressSanitize-d Build")
else(${_CRF_SANITIZE_BUILD} STREQUAL "msan")
  set_libtsan_link_flag ()
  set (CXX_SANITIZER_FLAGS "-fsanitize=thread ${_CRF_LIBTSAN_LINK_FLAG} -fno-omit-frame-pointer")
  message ("-- Making ThreadSanitize-d Build")
endif(${_CRF_SANITIZE_BUILD} STREQUAL "msan")


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

