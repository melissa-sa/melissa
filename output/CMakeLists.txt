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
     OUTPUT_H
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     *.h
     )

 file(GLOB
      OUTPUT_C
      RELATIVE
      ${CMAKE_CURRENT_SOURCE_DIR}
      *.c
      )

add_library(melissa_output SHARED ${OUTPUT_C} ${OUTPUT_H})
set_target_properties(melissa_output PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR} VERSION ${PROJECT_VERSION})
target_link_libraries(melissa_output ${EXTRA_LIBS})
target_compile_options(melissa_output BEFORE PUBLIC -fPIC)
install(TARGETS melissa_output LIBRARY DESTINATION lib)
install(FILES ${OUTPUT_H} DESTINATION include)
