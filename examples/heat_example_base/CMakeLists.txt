###################################################################
#                            Melissa                              #
#-----------------------------------------------------------------#
#   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    #
#                                                                 #
# This source is covered by the BSD 3-Clause License.             #
# Refer to the  LICENSE file for further information.             #
#                                                                 #
#-----------------------------------------------------------------#
#  Original Contributors:                                         #
#    Theophile Terraz,                                            #
#    Bruno Raffin,                                                #
#    Alejandro Ribes,                                             #
#    Bertrand Iooss,                                              #
###################################################################


#configure_file(options.py options_base.py @ONLY)
#install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example_base/options_base.py DESTINATION ${EXAMPLES_DIR}/heat_example)
configure_file(heat_base.c heat_base.c COPYONLY)
install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example_base/heat_base.c DESTINATION ${EXAMPLES_DIR}/heat_example_base/src)
configure_file(heat_base.f90 heat_base.f90 COPYONLY)
install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example_base/heat_base.f90 DESTINATION ${EXAMPLES_DIR}/heat_example_base/src)
configure_file(heat_utils.f90 heat_utils.f90 COPYONLY)
install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example_base/heat_utils.f90 DESTINATION ${EXAMPLES_DIR}/heat_example_base/src)
configure_file(heat_utils_no_mpi.f90 heat_utils_no_mpi.f90 COPYONLY)
install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example_base/heat_utils_no_mpi.f90 DESTINATION ${EXAMPLES_DIR}/heat_example_base/src)
configure_file(heat_base.py heat_base.py @ONLY)
install(PROGRAMS ${CMAKE_BINARY_DIR}/examples/heat_example_base/heat_base.py
        DESTINATION ${EXAMPLES_DIR}/heat_example_base/src)

if(BUILD_WITH_MPI)

    add_library(heat_utils SHARED heat_utils.f90)
    target_compile_options(heat_utils BEFORE PUBLIC -fPIC)
    target_link_libraries(heat_utils ${EXAMPLES_LIBS})
    install(TARGETS heat_utils DESTINATION ${EXAMPLES_DIR}/heat_example_base/lib)
    add_library(heat_utils_static STATIC heat_utils.f90)
    target_compile_options(heat_utils_static BEFORE PUBLIC -fPIC)
    target_link_libraries(heat_utils_static ${EXAMPLES_LIBS})
    set_target_properties(heat_utils_static PROPERTIES OUTPUT_NAME heat_utils)
    install(TARGETS heat_utils_static ARCHIVE DESTINATION ${EXAMPLES_DIR}/heat_example_base/lib)

    add_executable(heatf_base heat_base.f90)
    target_link_libraries(heatf_base ${EXAMPLES_LIBS} heat_utils_static)
    install(TARGETS heatf_base RUNTIME DESTINATION ${EXAMPLES_DIR}/heat_example_base/bin)

    add_executable(heatc_base heat_base.c)
    target_link_libraries(heatc_base ${EXAMPLES_LIBS} heat_utils_static)
    install(TARGETS heatc_base RUNTIME DESTINATION ${EXAMPLES_DIR}/heat_example_base/bin)
endif(BUILD_WITH_MPI)

add_library(heat_utils_no_mpi STATIC heat_utils_no_mpi.f90)
target_compile_options(heat_utils_no_mpi BEFORE PUBLIC -fPIC)
target_link_libraries(heat_utils_no_mpi ${EXAMPLES_LIBS})
install(TARGETS heat_utils_no_mpi DESTINATION ${EXAMPLES_DIR}/heat_example_base/lib)

if(BUILD_WITH_MPI)
    add_test(TestHeatcBase ${EXAMPLES_DIR}/heat_example_base/bin/heatc_base 5)
    add_test(TestHeatfBase ${EXAMPLES_DIR}/heat_example_base/bin/heatf_base 5)
endif(BUILD_WITH_MPI)
