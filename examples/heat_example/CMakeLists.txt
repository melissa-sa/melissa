
configure_file(options.py options.py @ONLY)

if(BUILD_WITH_MPI)

  add_executable(heatf heat.f90 ../heat_example_base/heat_utils.f90)
  target_link_libraries(heatf ${EXAMPLES_LIBS}  melissa_api)

  add_executable(heatc heat.c)
  target_link_libraries(heatc ${EXAMPLES_LIBS} melissa_api heat_utils)
endif(BUILD_WITH_MPI)

add_executable(heatf_no_mpi heat_no_mpi.f90 ../heat_example_base/heat_utils_no_mpi.f90)
target_link_libraries(heatf_no_mpi ${EXAMPLES_LIBS} melissa_api)

add_executable(heatc_no_mpi heat_no_mpi.c)
target_link_libraries(heatc_no_mpi ${EXAMPLES_LIBS} melissa_api heat_utils_no_mpi)

add_test(TestHeatc python ../../launcher/melissa_launcher.py .)
