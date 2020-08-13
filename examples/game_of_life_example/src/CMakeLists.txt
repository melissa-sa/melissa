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

install(FILES main.cpp DESTINATION ${EXAMPLES_DIR}/game_of_life_example/solver/src)
install(FILES include/lifeUtils.hpp DESTINATION ${EXAMPLES_DIR}/game_of_life_example/solver/src/include)
configure_file(CMakeLists.txt.in CMakeLists.txt.in @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt.in DESTINATION ${EXAMPLES_DIR}/game_of_life_example/solver RENAME CMakeLists.txt)
