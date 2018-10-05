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

file(GLOB
     STATS_H
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     *.h
     )

file(GLOB
     STATS_C
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     *.c
     )

add_library(melissa_stats ${STATS_C} ${STATS_H} $<TARGET_OBJECTS:melissa_utils>)
set_target_properties(melissa_stats PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR} VERSION ${PROJECT_VERSION})
target_link_libraries(melissa_stats m ${MPI_LIBRARIES})
target_compile_options(melissa_stats BEFORE PUBLIC -fPIC -Wall)
install(TARGETS melissa_stats DESTINATION lib)
install(FILES ${STATS_H} DESTINATION include)

add_library(melissa_stats_no_mpi ${STATS_C} ${STATS_H} $<TARGET_OBJECTS:melissa_utils>)
set_target_properties(melissa_stats_no_mpi PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR} VERSION ${PROJECT_VERSION})
target_link_libraries(melissa_stats_no_mpi m)
target_compile_definitions(melissa_stats_no_mpi PRIVATE BUILD_WITH_MPI=0)
target_compile_options(melissa_stats_no_mpi BEFORE PUBLIC -fPIC -Wall)
install(TARGETS melissa_stats_no_mpi DESTINATION lib)
