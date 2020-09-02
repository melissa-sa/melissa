###################################################################
#                            Melissa                              #
#-----------------------------------------------------------------#
#   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    #
#                                                                 #
# This source is covered by the BSD 3-Clause License.             #
# Refer to the  LICENSE file for further information.             #
#                                                                 #
#-----------------------------------------------------------------#
#  Original Contributors:                                         #
#    Theophile Terraz,                                            #
#    Bruno Raffin,                                                #
#    Alejandro Ribes,                                             #
#    Bertrand Iooss,                                              #
###################################################################

file(GLOB
     ALL_API
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     *.c
     )

if(BUILD_WITH_FLOWVR)
    set(ALL_API ${ALL_API}
                flowvr_group.cpp)
endif(BUILD_WITH_FLOWVR)

configure_file(melissa_api.h ${CMAKE_CURRENT_BINARY_DIR}/melissa_api.h @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/api/melissa_api.h DESTINATION include)
configure_file(melissa_api.py ${CMAKE_CURRENT_BINARY_DIR}/melissa_api.py COPYONLY)
install(FILES ${CMAKE_BINARY_DIR}/api/melissa_api.py DESTINATION lib)

configure_file(melissa_api.f90 melissa_api.f90 COPYONLY)
install(FILES ${CMAKE_BINARY_DIR}/api/melissa_api.f90 DESTINATION include)

configure_file(melissa_api.f melissa_api.f COPYONLY)
install(FILES ${CMAKE_BINARY_DIR}/api/melissa_api.f DESTINATION include)

add_library(melissa_api SHARED ${ALL_API} $<TARGET_OBJECTS:melissa_utils>)

set_target_properties(melissa_api PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR} VERSION ${PROJECT_VERSION})
target_link_libraries(melissa_api ${EXTRA_LIBS} melissa_messages zmq)
target_compile_options(melissa_api BEFORE PUBLIC -fPIC)
install(TARGETS melissa_api LIBRARY DESTINATION lib)
