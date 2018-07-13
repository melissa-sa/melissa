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
target_link_libraries(get_message PUBLIC ${EXTRA_LIBS} ${ZMQ_LIBRARY})
target_compile_options(get_message BEFORE PUBLIC -fPIC -Wall)
install(TARGETS get_message LIBRARY DESTINATION share/launcher)

add_library(melissa_utils OBJECT ${UTILS_C} ${UTILS_H})
target_compile_options(melissa_utils BEFORE PUBLIC -fPIC)
install(FILES ${UTILS_H} DESTINATION include)

add_executable(create_file_master_name create_file_master_name.c)
install(TARGETS create_file_master_name RUNTIME DESTINATION share/launcher)
