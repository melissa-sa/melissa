
configure_file(master.py master.py COPYONLY)

if(BUILD_WITH_FLOWVR)
  add_executable(interface_flowvr ${SERVER_SRC} ${SERVER_H} interface_flowvr.cpp)
  target_link_libraries(interface_flowvr ${EXTRA_LIBS} melissa_utils melissa_stats)
endif(BUILD_WITH_FLOWVR)

if(BUILD_WITH_ZMQ)
  add_executable(server ${SERVER_SRC} ${SERVER_H} server.c)
  target_link_libraries(server ${EXTRA_LIBS} melissa_utils melissa_stats)
endif(BUILD_WITH_ZMQ)

add_executable(ensight_server ${SERVER_SRC} ${SERVER_H} ensight_server.c)
target_link_libraries(ensight_server ${EXTRA_LIBS} melissa_utils melissa_stats)
