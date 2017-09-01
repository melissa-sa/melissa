
if(BUILD_WITH_FLOWVR)
  add_executable(interface_flowvr ${SERVER_SRC} ${SERVER_H} interface_flowvr.cpp $<TARGET_OBJECTS:melissa_utils>)
  target_link_libraries(interface_flowvr ${EXTRA_LIBS} melissa_stats)
endif(BUILD_WITH_FLOWVR)

add_executable(server ${SERVER_SRC} ${SERVER_H} server.c $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(server ${EXTRA_LIBS} melissa_stats)

#add_executable(server_sofiia ${SERVER_SRC} ${SERVER_H} server_sofiia.c $<TARGET_OBJECTS:melissa_utils>)
#target_link_libraries(server_sofiia ${EXTRA_LIBS} melissa_stats)

#add_executable(ensight_server ${SERVER_SRC} ${SERVER_H} ensight_server.c $<TARGET_OBJECTS:melissa_utils>)
#target_link_libraries(ensight_server ${EXTRA_LIBS} melissa_stats)

#add_executable(convert_ensight ${SERVER_SRC} ${SERVER_H} convert_ensight.c $<TARGET_OBJECTS:melissa_utils>)
#target_link_libraries(convert_ensight ${EXTRA_LIBS} melissa_stats)
