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

add_subdirectory(stats)

add_subdirectory(output)

add_definitions(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

set(MELISSA_CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/cmake/${PROJECT_NAME}" CACHE STRING "install path for MelissaConfig.cmake")

file(GLOB
     ALL_SERVER
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     *.c
     *.cxx
     )

add_executable(melissa_server ${ALL_SERVER} $<TARGET_OBJECTS:melissa_utils>)


target_link_libraries(melissa_server ${EXTRA_LIBS}
                                     ${OPENTURNS_LIBRARIES}
                                     melissa_stats
                                     melissa_output
                                     melissa_messages
                                     dl)
INSTALL(TARGETS melissa_server RUNTIME DESTINATION bin RENAME melissa_server)
