
configure_file(data.txt data.txt COPYONLY)
configure_file(run_heat_f.sh run_heat_f.sh COPYONLY)
configure_file(run_heat_c.sh run_heat_c.sh COPYONLY)
configure_file(run_heat_c_sobol.sh run_heat_c_sobol.sh COPYONLY)
configure_file(master.py master.py COPYONLY)
configure_file(crash_server.py crash_server.py COPYONLY)

add_library(heat_utils heat_utils.f90)
target_compile_options(heat_utils BEFORE PUBLIC -fPIC)
target_link_libraries(heat_utils ${EXAMPLES_LIBS})

add_executable(heatf ../../api/stats_api_fortran.f90 heat.f90)
target_link_libraries(heatf ${EXAMPLES_LIBS} stats_api_fortran heat_utils)

add_executable(heatc heat.c)
target_link_libraries(heatc ${EXAMPLES_LIBS} stats_api heat_utils)

add_test(TestHeatc python master.py)

add_test(TestCrashServer python crash_server.py)
