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


add_subdirectory(src)
add_subdirectory(scripts)

#find_python_module(matplotlib)
#if (PY_MATPLOTLIB)
#  set(HEAT_VISU "heat_visu")
#else (PY_MATPLOTLIB)
set(HEAT_VISU "None")
#endif (PY_MATPLOTLIB)

configure_file(options.py options.py @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example/options.py DESTINATION share/examples/heat_example)


add_test(TestHeatc "melissa_launcher")
