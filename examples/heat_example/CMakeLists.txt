
configure_file(data.txt data.txt COPYONLY)
configure_file(run_heat_f.sh run_heat_f.sh COPYONLY)
configure_file(run_heat_c.sh run_heat_c.sh COPYONLY)
configure_file(run_heat_c_sobol.sh run_heat_c_sobol.sh COPYONLY)

add_executable(heatf ../../api/stats_api_fortran.f90 heat_utils.f90 heat.f90)
target_link_libraries(heatf ${EXAMPLES_LIBS} stats_api_fortran)

add_executable(heatc heat_utils.f90 heat.c)
target_link_libraries(heatc ${EXAMPLES_LIBS} stats_api)

add_executable(mpi_reader mpi_reader.c)
target_link_libraries(mpi_reader ${EXTRA_LIBS} stats)
