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


configure_file(heat.c heat.c COPYONLY)
install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example/src/heat.c DESTINATION share/examples/heat_example/src)
configure_file(heat.f90 heat.f90 COPYONLY)
install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example/src/heat.f90 DESTINATION share/examples/heat_example/src)
configure_file(heat_no_mpi.c heat_no_mpi.c COPYONLY)
install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example/src/heat_no_mpi.c DESTINATION share/examples/heat_example/src)
configure_file(heat_no_mpi.f90 heat_no_mpi.f90 COPYONLY)
install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example/src/heat_no_mpi.f90 DESTINATION share/examples/heat_example/src)

if(BUILD_WITH_MPI)
  add_executable(heatf heat.f90 ../../heat_example_base/heat_utils.f90)
  target_link_libraries(heatf  melissa_api)
  install(TARGETS heatf RUNTIME DESTINATION share/examples/heat_example/bin)

  add_executable(heatc heat.c)
  target_link_libraries(heatc melissa_api heat_utils)
  install(TARGETS heatc RUNTIME DESTINATION share/examples/heat_example/bin)
endif(BUILD_WITH_MPI)

add_executable(heatf_no_mpi heat_no_mpi.f90 ../../heat_example_base/heat_utils_no_mpi.f90)
target_link_libraries(heatf_no_mpi melissa_api)
install(TARGETS heatf_no_mpi RUNTIME DESTINATION share/examples/heat_example/bin)

add_executable(heatc_no_mpi heat_no_mpi.c)
target_link_libraries(heatc_no_mpi melissa_api heat_utils_no_mpi)
install(TARGETS heatc_no_mpi RUNTIME DESTINATION share/examples/heat_example/bin)

