
set(EXAMPLES_LIBS ${EXTRA_LIBS}
                  m )

add_subdirectory(heat_example)

add_executable(create_file_server_name create_file_server_name.c)

add_executable(create_file_master_name create_file_master_name.c)
