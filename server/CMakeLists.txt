
set(SERVER_SRC compute_stats.c
               melissa_data.c
               melissa_io.c
               melissa_options.c
               server_helper.c)
set(ALL_SRCS ${ALL_SRCS} ${SERVER_SRC})
set(SERVER_H compute_stats.h
             melissa_data.h
             melissa_io.h
             melissa_options.h
             server.h)
set(ALL_INCS ${ALL_INCS} ${SERVER_H})
configure_file(master.py master.py COPYONLY)

#add_library(melissa_server ${ALL_SRC} ${ALL_INCS})

if(BUILD_WITH_FLOWVR)
  add_executable(interface_flowvr ${UTILS_C} ${UTILS_H} ${SERVER_SRC} ${SERVER_H} interface_flowvr.cpp)
  target_link_libraries(interface_flowvr ${EXTRA_LIBS} melissa_utils melissa_stats)
endif(BUILD_WITH_FLOWVR)

if(BUILD_WITH_ZMQ)
  add_executable(server ${UTILS_C} ${UTILS_H} ${SERVER_SRC} ${SERVER_H} server.c)
  target_link_libraries(server ${EXTRA_LIBS} melissa_stats)
endif(BUILD_WITH_ZMQ)

#if(BUILD_WITH_MPI)
#  add_executable(mpi_server mpi_server.c)
#  target_link_libraries(mpi_server ${EXTRA_LIBS} melissa_utils melissa_stats melissa_server)
#endif(BUILD_WITH_MPI)

add_executable(ensight_server ${UTILS_C} ${UTILS_H} ${SERVER_SRC} ${SERVER_H} ensight_server.c)
target_link_libraries(ensight_server ${EXTRA_LIBS} melissa_stats)
