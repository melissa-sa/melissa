
set(EXAMPLES_LIBS ${EXTRA_LIBS}
                  m )

if(BUILD_WITH_MPI)
  add_subdirectory(heat_example)
endif(BUILD_WITH_MPI)

add_subdirectory(heat_example_no_mpi)

add_executable(create_file_server_name create_file_server_name.c)

add_executable(create_file_master_name create_file_master_name.c)
