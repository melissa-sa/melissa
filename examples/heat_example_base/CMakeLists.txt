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

add_library(heat_utils STATIC heat_utils.f90)
target_link_libraries(heat_utils PRIVATE mpi_Fortran)

add_executable(heatc_base heat_base.c)
target_link_libraries(heatc_base heat_utils mpi_C)

add_executable(heatf_base heat_base.f90)
target_link_libraries(heatf_base heat_utils mpi_Fortran)
