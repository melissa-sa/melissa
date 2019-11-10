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


add_subdirectory(scripts)
configure_file(options.py.in options.py @ONLY)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/options.py DESTINATION
        ${EXAMPLES_DIR}/heat_example/study_cluster)
configure_file(job_scheduler_config.py.in job_scheduler_config.py @ONLY)
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/job_scheduler_config.py DESTINATION
        ${EXAMPLES_DIR}/heat_example/study_cluster)  ## TODO tu czy nie tu
