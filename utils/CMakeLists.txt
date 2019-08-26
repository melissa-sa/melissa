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

add_library(melissa_utils OBJECT ${UTILS_C} ${UTILS_H})
target_compile_options(melissa_utils BEFORE PUBLIC -fPIC)
#configure_file(melissa_utils.h ${CMAKE_CURRENT_BINARY_DIR}/melissa_utils.h @ONLY)
install(FILES ${UTILS_H} DESTINATION include)

add_library(melissa_messages SHARED melissa_messages.c melissa_messages.h melissa_utils.h $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(melissa_messages PUBLIC ${EXTRA_LIBS} ${ZeroMQ_LIBRARY})
target_compile_options(melissa_messages BEFORE PUBLIC -fPIC)
install(TARGETS melissa_messages LIBRARY DESTINATION lib)
install(FILES melissa_messages.h DESTINATION include)

add_library(melissa_comm4py SHARED melissa_comm4py.c $<TARGET_OBJECTS:melissa_utils> melissa_messages.c melissa_messages.h )
target_link_libraries(melissa_comm4py PUBLIC ${EXTRA_LIBS} ${ZeroMQ_LIBRARY})
target_compile_options(melissa_comm4py BEFORE PUBLIC -fPIC -Wall)
install(TARGETS melissa_comm4py LIBRARY DESTINATION lib)



