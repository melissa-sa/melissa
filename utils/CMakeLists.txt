
add_library(melissa_utils OBJECT ${UTILS_C} ${UTILS_H})
add_dependencies(melissa_utils ZeroMQ)
target_compile_options(melissa_utils BEFORE PUBLIC -fPIC)
#install(TARGETS melissa_utils DESTINATION bin)
install(FILES ${UTILS_H} DESTINATION include)

add_executable(create_file_server_name create_file_server_name.c)

add_executable(create_file_master_name create_file_master_name.c)
