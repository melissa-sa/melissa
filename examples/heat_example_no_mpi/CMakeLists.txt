
configure_file(data.txt data.txt COPYONLY)
 configure_file(options.py options.py COPYONLY)
 configure_file(master.py master.py COPYONLY)
 configure_file(crash_server.py crash_server.py COPYONLY)

add_library(heat_utils_no_mpi heat_utils.f90)
target_compile_options(heat_utils_no_mpi BEFORE PUBLIC -fPIC)
target_link_libraries(heat_utils_no_mpi ${EXAMPLES_LIBS})

add_executable(heatf_no_mpi ../../api/melissa_api_fortran.f90 heat.f90)
target_link_libraries(heatf_no_mpi ${EXAMPLES_LIBS} melissa_api_fortran heat_utils_no_mpi)

add_executable(heatc_no_mpi heat.c)
target_link_libraries(heatc_no_mpi ${EXAMPLES_LIBS} melissa_api heat_utils_no_mpi)

add_test(TestHeatcNoMPI python master.py)

add_test(TestCrashServerNoMPI python crash_server.py)
