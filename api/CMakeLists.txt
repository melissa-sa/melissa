
set(ALL_API stats_api.c
            stats_api.h
            stats_api_no_mpi.h
            ../utils/melissa_utils.c
            ../utils/melissa_utils.h)
add_library(stats_api ${ALL_API})
target_compile_options(stats_api BEFORE PUBLIC -fPIC)
install(TARGETS stats_api DESTINATION bin)
install(FILES stats_api.h DESTINATION include)
install(FILES stats_api_no_mpi.h DESTINATION include)

add_library(stats_api_fortran stats_api_fortran.f90)
target_link_libraries(stats_api_fortran stats_api)
install(TARGETS stats_api_fortran DESTINATION bin)
