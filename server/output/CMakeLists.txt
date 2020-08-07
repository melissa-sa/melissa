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
     OUTPUT_H
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     *.h
     )

set(MELISSA_OUTPUT_MODULE "txt" CACHE STRING "Choose which output module will be used to write statistics")

set(melissa_output_modules "txt;ensight;netcdf")

set_property(CACHE MELISSA_OUTPUT_MODULE PROPERTY STRINGS ${melissa_output_modules})

if(${MELISSA_OUTPUT_MODULE} STREQUAL "netcdf")
  find_library(NETCDF_LIBRARY netcdf)
  find_path(NETCDF_INCLUDES netcdf.h)
  if ((EXISTS ${NETCDF_INCLUDES}) AND (EXISTS ${NETCDF_LIBRARY}))
    include_directories(${NETCDF_INCLUDES})
    link_libraries(${NETCDF_LIBRARY})
  else()
    message(SEND_ERROR "netCDF needed to be installed to compile with netcdf writing!. Please install it correctly")
  endif()
endif()

set(OUTPUT_C "melissa_output.c;melissa_output_${MELISSA_OUTPUT_MODULE}.c")

add_library(melissa_output SHARED ${OUTPUT_C} ${OUTPUT_H})
set_target_properties(melissa_output PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR} VERSION ${PROJECT_VERSION})
#find_package(netCDF REQUIRED)

target_link_libraries(melissa_output ${EXTRA_LIBS})
target_compile_options(melissa_output BEFORE PUBLIC -fPIC)
install(TARGETS melissa_output LIBRARY DESTINATION lib)
install(FILES ${OUTPUT_H} DESTINATION include)
