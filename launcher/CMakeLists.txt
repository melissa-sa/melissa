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
     LAUNCHER_SRC
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     *.py
     )

foreach( _file ${LAUNCHER_SRC})
    configure_file(${_file} ${_file} @ONLY)
    install(FILES ${CMAKE_BINARY_DIR}/launcher/${_file} DESTINATION launcher)
endforeach()
