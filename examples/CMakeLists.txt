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


set(EXAMPLES_LIBS ${EXTRA_LIBS}
                  m)

add_subdirectory(heat_example_base)
add_subdirectory(heat_example)
add_subdirectory(droplet_example)
add_subdirectory(fluid_example)
add_subdirectory(game_of_life_example)
add_subdirectory(MelissaMonitoring)

install(FILES Melissa-monitoring-notebook.ipynb
        DESTINATION ${EXAMPLES_DIR}
        PERMISSIONS OWNER_WRITE
                    OWNER_READ
                    OWNER_EXECUTE
                    GROUP_READ
                    GROUP_EXECUTE
                    WORLD_READ
                    WORLD_EXECUTE)
