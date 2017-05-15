
configure_file(data.txt data.txt COPYONLY)
configure_file(options.py options.py COPYONLY)
configure_file(master.py master.py @ONLY)
configure_file(crash_server.py crash_server.py @ONLY)

if(BUILD_WITH_MPI)
  add_library(heat_utils heat_utils.f90)
  target_compile_options(heat_utils BEFORE PUBLIC -fPIC)
  target_link_libraries(heat_utils ${EXAMPLES_LIBS})

  add_executable(heatf heat.f90)
  target_link_libraries(heatf ${EXAMPLES_LIBS}  melissa_api heat_utils)

  add_executable(heatc heat.c)
  target_link_libraries(heatc ${EXAMPLES_LIBS} melissa_api heat_utils)
endif(BUILD_WITH_MPI)

add_library(heat_utils_no_mpi heat_utils_no_mpi.f90)
target_compile_options(heat_utils_no_mpi BEFORE PUBLIC -fPIC)
target_link_libraries(heat_utils_no_mpi ${EXAMPLES_LIBS})

add_executable(heatf_no_mpi heat_no_mpi.f90)
target_link_libraries(heatf_no_mpi ${EXAMPLES_LIBS} melissa_api heat_utils_no_mpi)

add_executable(heatc_no_mpi heat_no_mpi.c)
target_link_libraries(heatc_no_mpi ${EXAMPLES_LIBS} melissa_api heat_utils_no_mpi)

add_test(TestHeatc python master.py)

#add_test(TestCrashServer python crash_server.py)
