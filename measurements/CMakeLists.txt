add_executable (measure measure.cpp)
target_link_libraries (measure pthread ${ATOMICLIB})

add_executable (measure_jss measure.cpp)
target_link_libraries (measure_jss pthread ${ATOMICLIB})
target_compile_options(measure_jss PRIVATE -DTEST_WITH_JSS_ASP)

add_executable (measure_mutex measure.cpp)
target_link_libraries (measure_mutex pthread ${ATOMICLIB})
target_compile_options(measure_mutex PRIVATE -DXMUTEX)
