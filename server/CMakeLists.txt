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

add_definitions(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

set(MELISSA_CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/cmake/${PROJECT_NAME}" CACHE STRING "install path for MelissaConfig.cmake")

file(GLOB
     ALL_SERVER
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     *.c
     )

add_executable(melissa_server ${ALL_SERVER} $<TARGET_OBJECTS:melissa_utils>)
if (BUILD_WITH_MPI)
    target_link_libraries(melissa_server ${EXTRA_LIBS} ${ZEROMQ_LIBRARIES} melissa_stats melissa_output dl)
else (BUILD_WITH_MPI)
    target_link_libraries(melissa_server ${EXTRA_LIBS} ${ZEROMQ_LIBRARIES} melissa_stats_no_mpi melissa_output dl)
endif (BUILD_WITH_MPI)
INSTALL(TARGETS melissa_server RUNTIME DESTINATION bin RENAME melissa_server)

add_library(melissa_server4py SHARED ${ALL_SERVER} $<TARGET_OBJECTS:melissa_utils>)
set_target_properties(melissa_server4py PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR} VERSION ${PROJECT_VERSION})
if (BUILD_WITH_MPI)
    target_link_libraries(melissa_server4py ${EXTRA_LIBS} ${ZEROMQ_LIBRARIES} melissa_stats melissa_output dl)
else (BUILD_WITH_MPI)
    target_link_libraries(melissa_server4py ${EXTRA_LIBS} ${ZEROMQ_LIBRARIES} melissa_stats_no_mpi melissa_output dl)
endif (BUILD_WITH_MPI)
INSTALL(TARGETS melissa_server4py DESTINATION lib)
