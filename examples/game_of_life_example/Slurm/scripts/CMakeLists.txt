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

install(PROGRAMS run_server.sh
    DESTINATION ${EXAMPLES_DIR}/game_of_life_example/study_Slurm/scripts)

install(PROGRAMS run_simu.sh
    DESTINATION ${EXAMPLES_DIR}/game_of_life_example/study_Slurm/scripts)
