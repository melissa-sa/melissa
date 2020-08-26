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

if(INSTALL_ZMQ)
  add_dependencies(melissa_utils ZeroMQ)
endif(INSTALL_ZMQ)

target_compile_options(melissa_utils BEFORE PUBLIC -fPIC)
install(FILES melissa_utils.h vector.h DESTINATION include)

add_library(melissa_messages SHARED melissa_messages.c $<TARGET_OBJECTS:melissa_utils>)

if(INSTALL_ZMQ)
  add_dependencies(melissa_messages ZeroMQ)
endif(INSTALL_ZMQ)

target_link_libraries(melissa_messages PUBLIC ${EXTRA_LIBS} ${ZeroMQ_LIBRARY})
target_compile_options(melissa_messages BEFORE PUBLIC -fPIC)
install(TARGETS melissa_messages LIBRARY DESTINATION lib)
install(FILES melissa_messages.h DESTINATION include)

add_library(melissa_comm4py SHARED melissa_comm4py.c $<TARGET_OBJECTS:melissa_utils> melissa_messages.c)
# make gethostname() declaration accessible
set_source_files_properties(
  melissa_comm4py.c PROPERTIES COMPILE_DEFINITIONS _DEFAULT_SOURCE)

if(INSTALL_ZMQ)
  add_dependencies(melissa_comm4py ZeroMQ)
endif(INSTALL_ZMQ)

target_link_libraries(melissa_comm4py PUBLIC ${EXTRA_LIBS} ${ZeroMQ_LIBRARY})
target_compile_options(melissa_comm4py BEFORE PUBLIC -fPIC -Wall)
install(TARGETS melissa_comm4py LIBRARY DESTINATION lib)



