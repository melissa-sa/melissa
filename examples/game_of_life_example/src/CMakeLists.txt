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

install(FILES main.cpp DESTINATION ${EXAMPLES_DIR}/game_of_life_example/solver/src)
install(FILES include/lifeUtils.hpp DESTINATION ${EXAMPLES_DIR}/game_of_life_example/solver/src/include)
configure_file(CMakeLists.txt.in CMakeLists.txt.in @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt.in DESTINATION ${EXAMPLES_DIR}/game_of_life_example/solver RENAME CMakeLists.txt)


if (BUILD_WITH_MPI)
    add_test(NAME ConfigureGameOfLife COMMAND cmake ${EXAMPLES_DIR}/game_of_life_example/solver WORKING_DIRECTORY ${EXAMPLES_DIR}/game_of_life_example/solver)
    add_test(NAME CompileGameOfLife COMMAND make install WORKING_DIRECTORY ${EXAMPLES_DIR}/game_of_life_example/solver)
    add_test(NAME TestGameOfLife COMMAND melissa_launcher ${EXAMPLES_DIR}/game_of_life_example/study_local WORKING_DIRECTORY ${EXAMPLES_DIR}/game_of_life_example)
endif (BUILD_WITH_MPI)
