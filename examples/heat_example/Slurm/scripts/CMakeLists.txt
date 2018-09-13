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

configure_file(run_server.sh.in run_server.sh @ONLY)
configure_file(run_simu.sh.in run_simu.sh @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/run_server.sh
    DESTINATION ${EXAMPLES_DIR}/heat_example/Slurm/scripts
    PERMISSIONS OWNER_WRITE
                OWNER_READ
                OWNER_EXECUTE
                GROUP_READ
                GROUP_EXECUTE
                WORLD_READ
                WORLD_EXECUTE)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/run_simu.sh
    DESTINATION ${EXAMPLES_DIR}/heat_example/Slurm/scripts
    PERMISSIONS OWNER_WRITE
                OWNER_READ
                OWNER_EXECUTE
                GROUP_READ
                GROUP_EXECUTE
                WORLD_READ
                WORLD_EXECUTE)
