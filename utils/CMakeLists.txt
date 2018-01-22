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


add_library(get_message SHARED get_message.c $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(get_message ${EXTRA_LIBS})
target_compile_options(get_message BEFORE PUBLIC -fPIC -Wall)

add_library(melissa_utils OBJECT ${UTILS_C} ${UTILS_H})
if(INSTALL_ZMQ)
  add_dependencies(melissa_utils ZeroMQ)
endif(INSTALL_ZMQ)
target_compile_options(melissa_utils BEFORE PUBLIC -fPIC)
#install(TARGETS melissa_utils DESTINATION bin)
install(FILES ${UTILS_H} DESTINATION include)

add_executable(create_file_server_name create_file_server_name.c)

add_executable(create_file_master_name create_file_master_name.c)
