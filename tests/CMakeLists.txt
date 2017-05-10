
set(TESTS_LIBS ${EXTRA_LIBS}
                  m )

add_executable(test_base test_base.c)
target_link_libraries(test_base ${TESTS_LIBS} melissa_stats)
add_test(TestBase mpirun -np 2 ./test_base)

add_executable(test_mean test_mean.c)
target_link_libraries(test_mean ${TESTS_LIBS} melissa_stats)
add_test(TestMean ./test_mean)

add_executable(test_variance test_variance.c)
target_link_libraries(test_variance ${TESTS_LIBS} melissa_stats)
add_test(TestVariance ./test_variance)

add_executable(test_covariance test_covariance.c)
target_link_libraries(test_covariance ${TESTS_LIBS} melissa_stats)
add_test(TestCovariance ./test_covariance)

add_executable(test_sobol test_sobol.c)
target_link_libraries(test_sobol ${TESTS_LIBS} melissa_stats)
add_test(TestSobol ./test_sobol)

add_subdirectory(test_client)

add_executable(test_getoptions test_getoptions.c ../server/melissa_options.c ../server/melissa_options.h)
target_link_libraries(test_getoptions ${TESTS_LIBS})
add_test(TestGetOptions ${EXECUTABLE_OUTPUT_PATH}/test_getoptions -p 3 -s 1000 -g 3000 -t 100 -o mean:variance:min:max:threshold:sobol -e 0.4)


