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


#if(BUILD_WITH_FLOWVR)
#  add_executable(interface_flowvr interface_flowvr.cpp $<TARGET_OBJECTS:melissa_utils>)
#  target_link_libraries(interface_flowvr ${EXTRA_LIBS} ${ZMQ_LIBRARY})
#endif(BUILD_WITH_FLOWVR)

add_executable(melissa_server ${SERVER_SRC} ${SERVER_H} server.c $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(melissa_server ${EXTRA_LIBS} ${ZMQ_LIBRARY} melissa_stats)
INSTALL(TARGETS melissa_server RUNTIME DESTINATION bin)

#add_executable(server_sofiia ${SERVER_SRC} ${SERVER_H} server_sofiia.c $<TARGET_OBJECTS:melissa_utils>)
#target_link_libraries(server_sofiia ${EXTRA_LIBS} melissa_stats)

#add_executable(ensight_server ${SERVER_SRC} ${SERVER_H} ensight_server.c $<TARGET_OBJECTS:melissa_utils>)
#target_link_libraries(ensight_server ${EXTRA_LIBS} melissa_stats)

#add_executable(convert_ensight ${SERVER_SRC} ${SERVER_H} convert_ensight.c $<TARGET_OBJECTS:melissa_utils>)
#target_link_libraries(convert_ensight ${EXTRA_LIBS} melissa_stats)
