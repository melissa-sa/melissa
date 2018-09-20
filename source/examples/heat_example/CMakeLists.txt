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

set (SCHED_VERSIONS "local" "OAR" "Slurm")

foreach(sched_version ${SCHED_VERSIONS})
    add_subdirectory(${sched_version})
endforeach(sched_version in ${SCHED_VERSIONS})

add_subdirectory(src)

#find_python_module(matplotlib)
#if (PY_MATPLOTLIB)
#  set(HEAT_VISU "heat_visu")
#else (PY_MATPLOTLIB)
set(HEAT_VISU "None")
#endif (PY_MATPLOTLIB)

foreach(sched_version ${SCHED_VERSIONS})
#    configure_file(options.py ${sched_version}/options.py @ONLY)
    install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example/${sched_version}/options.py DESTINATION ${EXAMPLES_DIR}/heat_example/${sched_version})
endforeach(sched_version in ${SCHED_VERSIONS})

add_test(NAME TestHeatc COMMAND melissa_launcher ${EXAMPLES_DIR}/heat_example/local)
