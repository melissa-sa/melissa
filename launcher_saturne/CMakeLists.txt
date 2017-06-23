
file(GLOB
     LAUNCHER_SATURNE
     RELATIVE
     ${CMAKE_CURRENT_SOURCE_DIR}
     *.py
     *.json
     )

foreach( _file ${LAUNCHER_SATURNE})
    configure_file(${_file} ${_file} @ONLY)
endforeach()
