
configure_file(master.py master.py COPYONLY)

if(BUILD_WITH_FLOWVR)
  add_executable(interface_flowvr ${SERVER_SRC} ${SERVER_H} interface_flowvr.cpp $<TARGET_OBJECTS:melissa_utils>)
  target_link_libraries(interface_flowvr ${EXTRA_LIBS} melissa_stats)
endif(BUILD_WITH_FLOWVR)

if(BUILD_WITH_ZMQ)
  add_executable(server ${SERVER_SRC} ${SERVER_H} server.c $<TARGET_OBJECTS:melissa_utils>)
  target_link_libraries(server ${EXTRA_LIBS} melissa_stats)
endif(BUILD_WITH_ZMQ)

add_executable(ensight_server ${SERVER_SRC} ${SERVER_H} ensight_server.c $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(ensight_server ${EXTRA_LIBS} melissa_stats)

add_executable(convert_ensight ${SERVER_SRC} ${SERVER_H} convert_ensight.c $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(convert_ensight ${EXTRA_LIBS} melissa_stats)

add_executable(test_getoptions test_getoptions.c melissa_options.c melissa_options.h)
target_link_libraries(test_getoptions ${EXTRA_LIBS})
add_test(TestGetOptions ${EXECUTABLE_OUTPUT_PATH}/test_getoptions -p 3 -s 1000 -g 3000 -t 100 -o mean:variance:min:max:threshold:sobol -e 0.4)
