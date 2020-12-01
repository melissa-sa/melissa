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

add_library(melissa_utils OBJECT melissa_utils.c vector.c)
# make gethostname() declaration accessible
set_source_files_properties(
  melissa_utils.c PROPERTIES COMPILE_DEFINITIONS _DEFAULT_SOURCE)
add_dependencies(melissa_utils zmq)
target_include_directories(melissa_utils PUBLIC ${ZeroMQ_INCLUDE_DIR})
target_compile_options(melissa_utils BEFORE PUBLIC -fPIC)
install(FILES melissa_utils.h vector.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_library(melissa_messages SHARED melissa_messages.c $<TARGET_OBJECTS:melissa_utils>)
target_compile_options(melissa_messages BEFORE PUBLIC -fPIC)
target_link_libraries(melissa_messages PUBLIC ${EXTRA_LIBS} zmq)
install(TARGETS melissa_messages LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES melissa_messages.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_library(melissa_comm4py SHARED melissa_comm4py.c $<TARGET_OBJECTS:melissa_utils> melissa_messages.c)
# make gethostname() declaration accessible
set_source_files_properties(
  melissa_comm4py.c PROPERTIES COMPILE_DEFINITIONS _DEFAULT_SOURCE)
target_link_libraries(melissa_comm4py PUBLIC ${EXTRA_LIBS} zmq)
target_compile_options(melissa_comm4py BEFORE PUBLIC -fPIC -Wall)
install(TARGETS melissa_comm4py LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
