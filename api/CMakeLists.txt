###################################################################
#                            Melissa                              #
#-----------------------------------------------------------------#
#   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    #
#                                                                 #
# This source is covered by the BSD 3-Clause License.             #
# Refer to the  LICENCE file for further information.             #
#                                                                 #
#-----------------------------------------------------------------#
#  Original Contributors:                                         #
#    Theophile Terraz,                                            #
#    Bruno Raffin,                                                #
#    Alejandro Ribes,                                             #
#    Bertrand Iooss,                                              #
###################################################################



set(MELISSA_MACROS
"#define MELISSA_COUPLING_NONE 0
#define MELISSA_COUPLING_DEFAULT 0
#define MELISSA_COUPLING_ZMQ 0
#define MELISSA_COUPLING_MPI 1
#define MELISSA_COUPLING_FLOWVR 2")

file(GLOB
     ALL_API
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     *.h
     *.c
     )

if(BUILD_WITH_FLOWVR)
    set(ALL_API ${ALL_API}
                flowvr_group.cpp)
endif(BUILD_WITH_FLOWVR)

if(BUILD_WITH_MPI)
    configure_file(melissa_api.h.in ${CMAKE_CURRENT_BINARY_DIR}/melissa_api.h @ONLY)
    install(FILES ${CMAKE_BINARY_DIR}/api/melissa_api.h DESTINATION include)
endif(BUILD_WITH_MPI)
configure_file(melissa_api_no_mpi.h.in ${CMAKE_CURRENT_BINARY_DIR}/melissa_api_no_mpi.h @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/api/melissa_api_no_mpi.h DESTINATION include)

configure_file(melissa_api.f90 melissa_api.f90 @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/api/melissa_api.f90 DESTINATION include)

configure_file(melissa_api.f melissa_api.f @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/api/melissa_api.f DESTINATION include)

add_library(melissa_api SHARED ${ALL_API} $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(melissa_api ${ZEROMQ_LIBRARIES} ${EXTRA_LIBS})
target_compile_options(melissa_api BEFORE PUBLIC -fPIC)
install(TARGETS melissa_api LIBRARY DESTINATION lib)
