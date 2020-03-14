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

configure_file(melissa_launcher.py.in melissa_launcher.py @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/launcher/melissa_launcher.py
        DESTINATION share/melissa/launcher)

install(FILES options.py
        DESTINATION share/melissa/launcher)

configure_file(simulation.py.in simulation.py @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/launcher/simulation.py
        DESTINATION share/melissa/launcher)

configure_file(study.py.in study.py @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/launcher/study.py
        DESTINATION share/melissa/launcher)
install(FILES ${CMAKE_BINARY_DIR}/launcher/study.py
        DESTINATION share/melissa/launcher
        RENAME melissa.py)

install(FILES ${CMAKE_BINARY_DIR}/launcher/melissa_launcher.py
        DESTINATION bin
        PERMISSIONS OWNER_WRITE
                    OWNER_READ
                    OWNER_EXECUTE
                    GROUP_READ
                    GROUP_EXECUTE
                    WORLD_READ
                    WORLD_EXECUTE
        RENAME melissa_launcher)

install(FILES batchspawner.py
        DESTINATION share/melissa/launcher)
