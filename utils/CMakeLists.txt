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
     UTILS_H
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     melissa_utils.h
     vector.h
     )

file(GLOB
     UTILS_C
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     melissa_utils.c
     vector.c
     )

add_library(get_message SHARED get_message.c $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(get_message PUBLIC ${EXTRA_LIBS} ${ZEROMQ_LIBRARIES})
target_compile_options(get_message BEFORE PUBLIC -fPIC -Wall)
install(TARGETS get_message LIBRARY DESTINATION share/launcher)

add_library(melissa_utils OBJECT ${UTILS_C} ${UTILS_H})
target_compile_options(melissa_utils BEFORE PUBLIC -fPIC)
install(FILES ${UTILS_H} DESTINATION include)

add_executable(create_file_master_name create_file_master_name.c)
install(TARGETS create_file_master_name RUNTIME DESTINATION share/launcher)
