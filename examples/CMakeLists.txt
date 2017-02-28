
set(EXAMPLES_LIBS ${EXTRA_LIBS}
                  m )

add_executable(test_base test_base.c)
target_link_libraries(test_base ${EXAMPLES_LIBS} melissa_stats)
add_test(TestBase mpirun -np 2 ./test_base)

add_executable(test_mean test_mean.c)
target_link_libraries(test_mean ${EXAMPLES_LIBS} melissa_stats)
add_test(TestMean ./test_mean)

add_executable(test_variance test_variance.c)
target_link_libraries(test_variance ${EXAMPLES_LIBS} melissa_stats)
add_test(TestVariance ./test_variance)

add_executable(test_covariance test_covariance.c)
target_link_libraries(test_covariance ${EXAMPLES_LIBS} melissa_stats)
add_test(TestCovariance ./test_covariance)

#add_executable(test_sobol test_sobol.c)
#target_link_libraries(test_sobol ${EXAMPLES_LIBS})

#if(BUILD_WITH_FLOWVR)
#  add_subdirectory(test_flowvr)
#endif(BUILD_WITH_FLOWVR)

add_subdirectory(test_client)

if(BUILD_WITH_MPI)
  add_subdirectory(heat_example)
endif(BUILD_WITH_MPI)

add_executable(create_file_server_name create_file_server_name.c)

add_executable(create_file_master_name create_file_master_name.c)

