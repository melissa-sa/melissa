# Copyright (c) 2017, Institut National de Recherche en Informatique et en Automatique (https://www.inria.fr/)
#               2017, EDF (https://www.edf.fr/)
#               2020, Institut National de Recherche en Informatique et en Automatique (https://www.inria.fr/)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# do not move executables from this directory around
unset(CMAKE_RUNTIME_OUTPUT_DIRECTORY)

set(TESTS_LIBS ${EXTRA_LIBS}
               zmq
               m)


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

add_executable(test_getoptions test_getoptions.c ../server/melissa_options.c $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(test_getoptions ${TESTS_LIBS})
add_test(TestGetOptions ${EXECUTABLE_OUTPUT_PATH}/test_getoptions -p 3 -s 1000 -t 100 -o mean:variance:min:max:threshold:sobol -e 0.4)


# check for sanitizer presence
try_compile(
    HAS_SANITIZERS ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/dummy.c
    COMPILE_DEFINITIONS -fsanitize=address -fsanitize=undefined
    LINK_LIBRARIES -fsanitize=address -fsanitize=undefined
)


if(HAS_SANITIZERS)
    message(STATUS "Address sanitizer detected")
    message(STATUS "Undefined behavior sanitizer detected")
else()
    message(STATUS "No sanitizers detected")
endif()


if(HAS_SANITIZERS)
    add_executable(regression_16 regression_16.c $<TARGET_OBJECTS:melissa_utils>)
    target_compile_options(regression_16 PRIVATE -fsanitize=undefined -fno-sanitize-recover=all)
    target_include_directories(regression_16 PRIVATE ${CMAKE_SOURCE_DIR})
# this flag must also be passed at link time
    target_link_libraries(regression_16 "-fsanitize=undefined")
    target_link_libraries(regression_16 ${TESTS_LIBS})
    add_test(Regression16 regression_16)
endif()


add_executable(dummy-simulation dummy-simulation.c)
set_source_files_properties(
    dummy-simulation.c PROPERTIES COMPILE_DEFINITIONS _XOPEN_SOURCE
)

separate_arguments(MPI_C_COMPILE_FLAGS)

target_compile_options(dummy-simulation PRIVATE ${MPI_C_COMPILE_FLAGS})
target_include_directories(dummy-simulation PRIVATE ${MPI_C_INCLUDE_PATH})
target_link_libraries(
    dummy-simulation
    melissa_api
    ${MPI_C_LINK_FLAGS} ${MPI_C_LIBRARIES}
)

configure_file(dummy-simulation-options.py.in dummy-simulation-options.py @ONLY)

# TODO:
# this test clutters the test directory; this may be a problem when re-running
# the test
add_test(
    NAME dummy-simulation
    COMMAND env PYTHONPATH=${CMAKE_BINARY_DIR}:$ENV{PYTHONPATH}
                python3 -m melissa.launcher
                    openmpi
                    ${CMAKE_CURRENT_BINARY_DIR}/dummy-simulation-options.py
                    ${CMAKE_CURRENT_BINARY_DIR}/dummy-simulation
)
