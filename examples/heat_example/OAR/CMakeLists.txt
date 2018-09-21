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
configure_file(README.txt.in README.txt @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/options.py DESTINATION ${EXAMPLES_DIR}/heat_example/study_OAR)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/README.txt DESTINATION ${EXAMPLES_DIR}/heat_example/study_OAR)
