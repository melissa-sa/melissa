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

install(FILES run_server.sh
    DESTINATION ${EXAMPLES_DIR}/game_of_life_example/study_Slurm/scripts
    PERMISSIONS OWNER_WRITE
                OWNER_READ
                OWNER_EXECUTE
                GROUP_READ
                GROUP_EXECUTE
                WORLD_READ
                WORLD_EXECUTE)
install(FILES run_simu.sh
    DESTINATION ${EXAMPLES_DIR}/game_of_life_example/study_Slurm/scripts
    PERMISSIONS OWNER_WRITE
                OWNER_READ
                OWNER_EXECUTE
                GROUP_READ
                GROUP_EXECUTE
                WORLD_READ
                WORLD_EXECUTE)
