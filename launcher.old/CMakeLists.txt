
file(GLOB
     LAUNCHER_SRC
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     *.py
     )


foreach( _file ${LAUNCHER_SRC})
    configure_file(${_file} ${_file} COPYONLY)
endforeach()
