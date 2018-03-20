set(TBB_ROOT ${PROJECT_SOURCE_DIR}/tbb)
include(${TBB_ROOT}/cmake/TBBBuild.cmake)
tbb_build(TBB_ROOT ${TBB_ROOT} CONFIG_DIR TBB_DIR)
find_package(TBB REQUIRED tbb)
message("tbb imported libs: " ${TBB_IMPORTED_TARGETS})
include_directories(${TBB_ROOT}/include)

add_executable (measure_rcuptr measure.cpp)
target_link_libraries (measure_rcuptr pthread ${ATOMICLIB})

add_executable (measure_rcuptr_jss measure.cpp)
target_link_libraries (measure_rcuptr_jss pthread ${ATOMICLIB})
target_compile_options(measure_rcuptr_jss PRIVATE -DTEST_WITH_JSS_ASP)

add_executable (measure_std_mutex measure.cpp)
target_link_libraries (measure_std_mutex pthread ${ATOMICLIB})
target_compile_options(measure_std_mutex PRIVATE -DX_STD_MUTEX)

add_executable (measure_tbb_qrw_mutex measure.cpp)
target_link_libraries (measure_tbb_qrw_mutex pthread ${ATOMICLIB} ${TBB_IMPORTED_TARGETS})
target_compile_options(measure_tbb_qrw_mutex PRIVATE -DX_TBB_QRW_MUTEX)

add_executable (measure_tbb_srw_mutex measure.cpp)
target_link_libraries (measure_tbb_srw_mutex pthread ${ATOMICLIB} ${TBB_IMPORTED_TARGETS})
target_compile_options(measure_tbb_srw_mutex PRIVATE -DX_TBB_SRW_MUTEX)

add_executable (measure_urcu measure.cpp)
target_link_libraries (measure_urcu urcu pthread)
target_compile_options(measure_urcu PRIVATE -DX_URCU)

add_executable (measure_urcu_mb measure.cpp)
target_link_libraries (measure_urcu_mb urcu-mb pthread)
target_compile_options(measure_urcu_mb PRIVATE -DRCU_MB -DX_URCU)

add_executable (measure_urcu_bp measure.cpp)
target_link_libraries (measure_urcu_bp urcu-bp pthread)
target_compile_options(measure_urcu_bp PRIVATE -DX_URCU -DX_URCU_BP)
