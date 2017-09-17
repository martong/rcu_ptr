cmake_minimum_required (VERSION 3.2)

include(CTest)

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
if (_CRF_SANITIZE_BUILD)
  add_compile_options (-fno-omit-frame-pointer)

if (${_CRF_SANITIZE_BUILD} STREQUAL "msan")
  add_compile_options (-fsanitize=memory)
  message ("-- Making MemorySanitize-d Build")

elseif (${_CRF_SANITIZE_BUILD} STREQUAL "asan")
  add_compile_options (-fsanitize=address)
  message ("-- Making AddressSanitize-d Build")

else ()
  set_libtsan_link_flag ()
  add_compile_options (-fsanitize=thread)
  add_compile_options (${_CRF_LIBTSAN_LINK_FLAG})
  message ("-- Making ThreadSanitize-d Build")

endif ()
endif ()


add_compile_options (-std=c++14)
add_compile_options (-Wall)
add_compile_options (-Wextra)
add_compile_options (-Wdeprecated)
add_compile_options (-pedantic)


if (CMAKE_BUILD_TYPE MATCHES Release)
  add_compile_options (-O3)

else ()
  add_compile_options (-O0)
  add_compile_options (-ggdb3)

endif ()


include_directories ("${PROJECT_SOURCE_DIR}")

add_subdirectory (googletest)
add_subdirectory (unit_test)
add_subdirectory (race_test)
add_subdirectory (tests)
