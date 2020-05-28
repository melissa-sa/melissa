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
configure_file(script.py.in script.py @ONLY)
configure_file(functions.py.in functions.py @ONLY)
configure_file(server.py.in server.py @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/options.py DESTINATION ${EXAMPLES_DIR}/heat_example/study_python_server)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/script.py DESTINATION ${EXAMPLES_DIR}/heat_example/study_python_server)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/functions.py DESTINATION ${EXAMPLES_DIR}/heat_example/study_python_server)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/server.py DESTINATION ${EXAMPLES_DIR}/heat_example/study_python_server)
