
add_library(melissa_api ${ALL_API} $<TARGET_OBJECTS:melissa_utils>)
#target_link_libraries(melissa_api melissa_utils)
target_compile_options(melissa_api BEFORE PUBLIC -fPIC)
install(TARGETS melissa_api DESTINATION bin)
install(FILES melissa_api.h DESTINATION include)
install(FILES melissa_api_no_mpi.h DESTINATION include)

add_library(melissa_api_fortran melissa_api_fortran.f90 $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(melissa_api_fortran melissa_api)
install(TARGETS melissa_api_fortran DESTINATION bin)
