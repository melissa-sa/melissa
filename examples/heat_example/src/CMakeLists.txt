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
install(FILES ../../heat_example_base/heat_utils.f90 DESTINATION ${EXAMPLES_DIR}/heat_example/solver/src)
configure_file(CMakeLists.txt.in CMakeLists.txt.in @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt.in DESTINATION ${EXAMPLES_DIR}/heat_example/solver RENAME CMakeLists.txt)
