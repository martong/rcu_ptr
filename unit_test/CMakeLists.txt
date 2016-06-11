file (GLOB unit_test_sources "unit_test.cpp")

add_executable (unit_test ${unit_test_sources})
target_link_libraries (unit_test pthread)

install (TARGETS unit_test DESTINATION "${PROJECT_BINARY_DIR}/bin/tests")

