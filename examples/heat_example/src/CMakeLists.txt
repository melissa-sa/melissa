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


foreach(sched_version ${SCHED_VERSIONS})
    configure_file(heat.c heat.c COPYONLY)
    install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example/src/heat.c DESTINATION ${EXAMPLES_DIR}/heat_example/${sched_version}/src)
    configure_file(heat.f90 heat.f90 COPYONLY)
    install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example/src/heat.f90 DESTINATION ${EXAMPLES_DIR}/heat_example/${sched_version}/src)
    configure_file(heat_no_mpi.c heat_no_mpi.c COPYONLY)
    install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example/src/heat_no_mpi.c DESTINATION ${EXAMPLES_DIR}/heat_example/${sched_version}/src)
    configure_file(heat_no_mpi.f90 heat_no_mpi.f90 COPYONLY)
    install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example/src/heat_no_mpi.f90 DESTINATION ${EXAMPLES_DIR}/heat_example/${sched_version}/src)
endforeach(sched_version ${SCHED_VERSIONS})

if(BUILD_WITH_MPI)
    add_executable(heatf heat.f90 ../../heat_example_base/heat_utils.f90)
    target_link_libraries(heatf  melissa_api)
    foreach(sched_version ${SCHED_VERSIONS})
        install(TARGETS heatf RUNTIME DESTINATION ${EXAMPLES_DIR}/heat_example/${sched_version}/bin)
    endforeach(sched_version ${SCHED_VERSIONS})

    add_executable(heatc heat.c)
    target_link_libraries(heatc melissa_api heat_utils)
    foreach(sched_version ${SCHED_VERSIONS})
        install(TARGETS heatc RUNTIME DESTINATION ${EXAMPLES_DIR}/heat_example/${sched_version}/bin)
    endforeach(sched_version ${SCHED_VERSIONS})
endif(BUILD_WITH_MPI)

add_executable(heatf_no_mpi heat_no_mpi.f90 ../../heat_example_base/heat_utils_no_mpi.f90)
target_link_libraries(heatf_no_mpi melissa_api)
foreach(sched_version ${SCHED_VERSIONS})
    install(TARGETS heatf_no_mpi RUNTIME DESTINATION ${EXAMPLES_DIR}/heat_example/${sched_version}/bin)
endforeach(sched_version ${SCHED_VERSIONS})

add_executable(heatc_no_mpi heat_no_mpi.c)
target_link_libraries(heatc_no_mpi melissa_api heat_utils_no_mpi)
foreach(sched_version ${SCHED_VERSIONS})
    install(TARGETS heatc_no_mpi RUNTIME DESTINATION ${EXAMPLES_DIR}/heat_example/${sched_version}/bin)
endforeach(sched_version ${SCHED_VERSIONS})

