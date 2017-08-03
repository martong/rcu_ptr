
file (GLOB units_sources "*.cpp")

add_executable (units ${units_sources})
target_link_libraries (units gtest pthread)

install (TARGETS units DESTINATION "${PROJECT_BINARY_DIR}/bin/tests")

