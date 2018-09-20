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


#configure_file(options.py options_base.py @ONLY)
#install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example_base/options_base.py DESTINATION ${EXAMPLES_DIR}/heat_example)
configure_file(heat_base.c heat_base.c COPYONLY)
install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example_base/heat_base.c DESTINATION ${EXAMPLES_DIR}/heat_example/base/src)
configure_file(heat_base.f90 heat_base.f90 COPYONLY)
install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example_base/heat_base.f90 DESTINATION ${EXAMPLES_DIR}/heat_example/base/src)
configure_file(heat_base.f90 heat_ulits.f90 COPYONLY)
install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example_base/heat_ulits.f90 DESTINATION ${EXAMPLES_DIR}/heat_example/base/src)
configure_file(heat_base.f90 heat_ulits_no_mpi.f90 COPYONLY)
install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example_base/heat_ulits_no_mpi.f90 DESTINATION ${EXAMPLES_DIR}/heat_example/base/src)

if(BUILD_WITH_MPI)
   add_library(heat_utils heat_utils.f90)
   target_compile_options(heat_utils BEFORE PUBLIC -fPIC)
   target_link_libraries(heat_utils ${EXAMPLES_LIBS})
   install(TARGETS heat_utils ARCHIVE DESTINATION ${EXAMPLES_DIR}/heat_example/base/lib)

   add_executable(heatf_base heat_base.f90)
   target_link_libraries(heatf_base ${EXAMPLES_LIBS} heat_utils)
   install(TARGETS heatf_base RUNTIME DESTINATION ${EXAMPLES_DIR}/heat_example/base/bin)

   add_executable(heatc_base heat_base.c)
   target_link_libraries(heatc_base ${EXAMPLES_LIBS} heat_utils)
   install(TARGETS heatc_base RUNTIME DESTINATION ${EXAMPLES_DIR}/heat_example/base/bin)
endif(BUILD_WITH_MPI)

add_library(heat_utils_no_mpi heat_utils_no_mpi.f90)
target_compile_options(heat_utils_no_mpi BEFORE PUBLIC -fPIC)
target_link_libraries(heat_utils_no_mpi ${EXAMPLES_LIBS})
install(TARGETS heat_utils_no_mpi ARCHIVE DESTINATION ${EXAMPLES_DIR}/heat_example/base/lib)

add_test(TestHeatcBase ${EXAMPLES_DIR}/heat_example/base/bin/heatc_base 5)
add_test(TestHeatfBase ${EXAMPLES_DIR}/heat_example/base/bin/heatf_base 5)
