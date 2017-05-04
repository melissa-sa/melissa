
if(BUILD_WITH_ZMQ)
  if(BUILD_WITH_MPI)
    configure_file(run_mpi_zmq_clients.sh run_clients.sh COPYONLY)
  else(BUILD_WITH_MPI)
    configure_file(run_zmq_clients.sh run_clients.sh COPYONLY)
  endif(BUILD_WITH_MPI)

  add_executable(client client.c)
  target_link_libraries(client ${EXTRA_LIBS} melissa_api)
endif(BUILD_WITH_ZMQ)
