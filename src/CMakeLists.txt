
add_library(stats ${ALL_SRCS})
target_link_libraries(stats m)
install(TARGETS stats DESTINATION bin)
install(FILES stats.h DESTINATION include)
install(FILES server.h DESTINATION include)

if(BUILD_WITH_FLOWVR)
  add_executable(interface_flowvr interface_flowvr.cpp)
  target_link_libraries(interface_flowvr ${EXTRA_LIBS} stats)
endif(BUILD_WITH_FLOWVR)

if(BUILD_WITH_ZMQ)
  add_executable(server server.c)
  target_link_libraries(server ${EXTRA_LIBS} stats)
endif(BUILD_WITH_ZMQ)

#if(BUILD_WITH_MPI)
#  add_executable(mpi_server mpi_server.c)
#  target_link_libraries(mpi_server ${EXTRA_LIBS} stats)
#endif(BUILD_WITH_MPI)

add_executable(ensight_server ensight_server.c)
target_link_libraries(ensight_server ${EXTRA_LIBS} stats)
