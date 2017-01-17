
add_library(stats_api ${ALL_API})
target_compile_options(stats_api BEFORE PUBLIC -fPIC)
target_link_libraries(stats_api stats)
install(TARGETS stats_api DESTINATION bin)
install(FILES stats_api.h DESTINATION include)
install(FILES stats_api_no_mpi.h DESTINATION include)

add_library(stats_api_fortran stats_api_fortran.f90)
target_link_libraries(stats_api_fortran stats_api
                                        stats)
install(TARGETS stats_api_fortran DESTINATION bin)
