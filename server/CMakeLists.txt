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

file(GLOB
     ALL_SERVER
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     *.c
     *.h
     )

#if(BUILD_WITH_FLOWVR)
#  add_executable(interface_flowvr interface_flowvr.cpp $<TARGET_OBJECTS:melissa_utils>)
#  target_link_libraries(interface_flowvr ${EXTRA_LIBS} ${ZMQ_LIBRARY})
#endif(BUILD_WITH_FLOWVR)

add_executable(melissa_server ${ALL_SERVER} server.c $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(melissa_server ${EXTRA_LIBS} ${ZEROMQ_LIBRARIES} melissa_stats)
INSTALL(TARGETS melissa_server RUNTIME DESTINATION bin)

#add_executable(server_sofiia ${ALL_SERVER} server_sofiia.c $<TARGET_OBJECTS:melissa_utils>)
#target_link_libraries(server_sofiia ${EXTRA_LIBS} melissa_stats)

#add_executable(ensight_server ${ALL_SERVER} ensight_server.c $<TARGET_OBJECTS:melissa_utils>)
#target_link_libraries(ensight_server ${EXTRA_LIBS} melissa_stats)

#add_executable(convert_ensight ${ALL_SERVER} convert_ensight.c $<TARGET_OBJECTS:melissa_utils>)
#target_link_libraries(convert_ensight ${EXTRA_LIBS} melissa_stats)
