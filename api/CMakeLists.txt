
add_library(stats_api ${ALL_API} $<TARGET_OBJECTS:melissa_utils>)
#target_link_libraries(stats_api melissa_utils)
target_compile_options(stats_api BEFORE PUBLIC -fPIC)
install(TARGETS stats_api DESTINATION bin)
install(FILES stats_api.h DESTINATION include)
install(FILES stats_api_no_mpi.h DESTINATION include)

add_library(stats_api_fortran stats_api_fortran.f90 $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(stats_api_fortran stats_api)
install(TARGETS stats_api_fortran DESTINATION bin)
