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

configure_file(run_server.sh.in run_server.sh @ONLY)
configure_file(run_simu.sh.in run_simu.sh @ONLY)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/run_server.sh
    DESTINATION ${EXAMPLES_DIR}/heat_example/study_Slurm/scripts)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/run_simu.sh
    DESTINATION ${EXAMPLES_DIR}/heat_example/study_Slurm/scripts)
