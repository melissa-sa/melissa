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

add_definitions(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

file(GLOB
     ALL_SERVER
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     *.c
     *.h
     )

add_executable(melissa_server ${ALL_SERVER} $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(melissa_server ${EXTRA_LIBS} ${ZEROMQ_LIBRARIES} melissa_stats dl)
INSTALL(TARGETS melissa_server RUNTIME DESTINATION bin)
