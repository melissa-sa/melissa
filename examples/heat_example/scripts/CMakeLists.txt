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


find_python_module(matplotlib)
#if (PY_MATPLOTLIB)
#  set(HEAT_VISU "heat_visu")
#else (PY_MATPLOTLIB)
  set(HEAT_VISU "None")
#endif (PY_MATPLOTLIB)
file(GLOB
     SCRIPTS_SRC
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     *.py
     )
foreach( _file ${SCRIPTS_SRC})
    configure_file(${_file} ${_file} @ONLY)
    install(FILES ${CMAKE_BINARY_DIR}/examples/heat_example/scripts/${_file} DESTINATION share/examples/heat_example/scripts)
endforeach()
