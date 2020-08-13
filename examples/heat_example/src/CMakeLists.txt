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

configure_file(heat.py heat.py @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/heat.py DESTINATION ${EXAMPLES_DIR}/heat_example/solver/src)
install(FILES heat.c DESTINATION ${EXAMPLES_DIR}/heat_example/solver/src)
install(FILES heat.f90 DESTINATION ${EXAMPLES_DIR}/heat_example/solver/src)
install(FILES heat_no_mpi.c DESTINATION ${EXAMPLES_DIR}/heat_example/solver/src)
install(FILES heat_no_mpi.f90 DESTINATION ${EXAMPLES_DIR}/heat_example/solver/src)
install(FILES ../../heat_example_base/heat_utils_no_mpi.f90 DESTINATION ${EXAMPLES_DIR}/heat_example/solver/src)
install(FILES ../../heat_example_base/heat_utils.f90 DESTINATION ${EXAMPLES_DIR}/heat_example/solver/src)
configure_file(CMakeLists.txt.in CMakeLists.txt.in @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt.in DESTINATION ${EXAMPLES_DIR}/heat_example/solver RENAME CMakeLists.txt)

if (BUILD_WITH_MPI)
    add_test(NAME ConfigureHeatc COMMAND cmake ${EXAMPLES_DIR}/heat_example/solver WORKING_DIRECTORY ${EXAMPLES_DIR}/heat_example/solver)
    add_test(NAME CompileHeatc COMMAND make install WORKING_DIRECTORY ${EXAMPLES_DIR}/heat_example/solver)
    add_test(NAME TestHeatc1 COMMAND ${CMAKE_INSTALL_PREFIX}/bin/melissa_launcher -o ${EXAMPLES_DIR}/heat_example/study_local/options.py WORKING_DIRECTORY ${EXAMPLES_DIR}/heat_example/study_local)
    add_test(NAME TestHeatc2 COMMAND python3 ${EXAMPLES_DIR}/heat_example/study_local/script.py WORKING_DIRECTORY ${EXAMPLES_DIR}/heat_example/study_local)
endif (BUILD_WITH_MPI)
