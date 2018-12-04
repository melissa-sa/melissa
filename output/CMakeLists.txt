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

add_library(melissa_output SHARED melissa_output.c melissa_output_parflow_netcdf.c)
set_target_properties(melissa_output PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR} VERSION ${PROJECT_VERSION})
#find_package(netCDF REQUIRED)

target_link_libraries(melissa_output ${EXTRA_LIBS} -lnetcdf)
target_compile_options(melissa_output BEFORE PUBLIC -fPIC)
install(TARGETS melissa_output LIBRARY DESTINATION lib)
