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

configure_file(melissa_launcher.py melissa_launcher.py @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/melissa_launcher.py
        DESTINATION share/melissa/launcher)

install(FILES options.py
        DESTINATION share/melissa/launcher)

install(FILES simulation.py
        DESTINATION share/melissa/launcher)

install(FILES study.py
        DESTINATION share/melissa/launcher)
install(FILES study.py
        DESTINATION share/melissa/launcher
        RENAME melissa.py)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/melissa_launcher.py
        DESTINATION bin
        RENAME melissa_launcher)

install(FILES batchspawner.py
        DESTINATION share/melissa/launcher)
