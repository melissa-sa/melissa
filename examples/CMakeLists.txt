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


set(EXAMPLES_LIBS ${EXTRA_LIBS}
                  m)

add_subdirectory(heat_example_base)
add_subdirectory(heat_example)
add_subdirectory(droplet_example)
add_subdirectory(fluid_example)
add_subdirectory(game_of_life_example)
