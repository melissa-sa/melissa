###################################################################
#                            Melissa                              #
#-----------------------------------------------------------------#
#   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    #
#                                                                 #
# This source is covered by the BSD 3-Clause License.             #
# Refer to the  LICENCE file for further information.             #
#                                                                 #
#-----------------------------------------------------------------#
#  Original Contributors:                                         #
#    Theophile Terraz,                                            #
#    Bruno Raffin,                                                #
#    Alejandro Ribes,                                             #
#    Bertrand Iooss,                                              #
###################################################################


set(TESTS_LIBS ${EXTRA_LIBS}
               ${ZMQ_LIBRARY}
                  m mpi )

#foreach(lang C CXX Fortran)
#    if(CMAKE_COMPILER_IS_GNU${lang})
#        set(TESTS_LIBS ${TESTS_LIBS}
#                   gcov )
#        include(CodeCoverage)
#        setup_target_for_coverage(${PROJECT_NAME}_coverage ${PROJECT_TEST_NAME} coverage)
#    endif()
#endforeach()

add_executable(test_base test_base.c $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(test_base ${TESTS_LIBS} melissa_stats)
add_test(TestBase mpirun -np 2 ./test_base)

add_executable(test_mean test_mean.c $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(test_mean ${TESTS_LIBS} melissa_stats)
add_test(TestMean ./test_mean)

add_executable(test_variance test_variance.c $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(test_variance ${TESTS_LIBS} melissa_stats)
add_test(TestVariance ./test_variance)

add_executable(test_covariance test_covariance.c $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(test_covariance ${TESTS_LIBS} melissa_stats)
add_test(TestCovariance ./test_covariance)

add_executable(test_sobol test_sobol.c $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(test_sobol ${TESTS_LIBS} melissa_stats)
add_test(TestSobol ./test_sobol)

add_executable(test_getoptions test_getoptions.c ../server/melissa_options.c ../server/melissa_options.h $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(test_getoptions ${TESTS_LIBS})
add_test(TestGetOptions ${EXECUTABLE_OUTPUT_PATH}/test_getoptions -p 3 -s 1000 -t 100 -o mean:variance:min:max:threshold:sobol -e 0.4)


