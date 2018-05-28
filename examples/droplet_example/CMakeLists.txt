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
     create_group.py
     *.json
     *.txt
     geo_gouttedo.cli
     geo_gouttedo.slf
     t2d_gouttedo.cas
     )

foreach( _file ${LAUNCHER_TELEMAC2D})
    configure_file(${_file} ${_file} COPYONLY)
    install(FILES ${CMAKE_BINARY_DIR}/examples/droplet_example/${_file} DESTINATION share/examples/droplet_example)
endforeach()

configure_file(options.py options.py @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/examples/droplet_example/options.py DESTINATION share/examples/droplet_example)

file(GLOB
     USR_F
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     user_fortran/*
     )

foreach( _file ${USR_F})
    install(FILES ${_file} DESTINATION share/examples/droplet_example/user_fortran)
endforeach()

add_executable(melissa_to_serafin melissa_to_serafin.f)
target_compile_options(melissa_to_serafin BEFORE PUBLIC "-fconvert=big-endian")
install(TARGETS melissa_to_serafin RUNTIME DESTINATION share/examples/droplet_example)