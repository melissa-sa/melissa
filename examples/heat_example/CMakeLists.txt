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


configure_file(options.py options.py @ONLY)

if(BUILD_EXAMPLES_WITH_MPI)

  add_executable(heatf heat.f90 ../heat_example_base/heat_utils.f90)
  target_link_libraries(heatf  melissa_api)

  add_executable(heatc heat.c)
  target_link_libraries(heatc melissa_api heat_utils)

endif(BUILD_EXAMPLES_WITH_MPI)

add_executable(heatf_no_mpi heat_no_mpi.f90 ../heat_example_base/heat_utils_no_mpi.f90)
target_link_libraries(heatf_no_mpi melissa_api)

add_executable(heatc_no_mpi heat_no_mpi.c)
target_link_libraries(heatc_no_mpi melissa_api heat_utils_no_mpi)

add_test(TestHeatc python ../../launcher/melissa_launcher.py .)
