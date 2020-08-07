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

configure_file(BaseOptions.py.in BaseOptions.py @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/BaseOptions.py DESTINATION ${EXAMPLES_DIR}/game_of_life_example)

set (SCHED_VERSIONS "local" "Slurm")

foreach(sched_version ${SCHED_VERSIONS})
    add_subdirectory(${sched_version})
endforeach(sched_version in ${SCHED_VERSIONS})

add_subdirectory(src)
