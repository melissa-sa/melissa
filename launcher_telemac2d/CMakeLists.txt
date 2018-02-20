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
     LAUNCHER_TELEMAC2D
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     *.py
     *.json
     *.f
     *.txt
     geo_gouttedo.cli
     geo_gouttedo.slf
     t2d_gouttedo.cas
     user_fortran/*
     )

foreach( _file ${LAUNCHER_TELEMAC2D})
    configure_file(${_file} ${_file} @ONLY)
endforeach()
