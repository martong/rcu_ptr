file (GLOB race_test_sources "race_test.cpp")

add_executable (race_test ${race_test_sources})
target_link_libraries (race_test pthread)

install (TARGETS race_test DESTINATION "${PROJECT_BINARY_DIR}/bin/tests")

