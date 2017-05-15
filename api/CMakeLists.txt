
add_library(melissa_api ${ALL_API} $<TARGET_OBJECTS:melissa_utils>)
#target_link_libraries(melissa_api melissa_utils)
target_compile_options(melissa_api BEFORE PUBLIC -fPIC)
install(TARGETS melissa_api DESTINATION bin)
if(BUILD_WITH_MPI)
    install(FILES melissa_api.h DESTINATION include)
endif(BUILD_WITH_MPI)
install(FILES melissa_api_no_mpi.h DESTINATION include)

configure_file(melissa_api.f90 melissa_api.f90 COPYONLY)
install(FILES melissa_api.f90 DESTINATION include)
