 
file(GLOB LAUNCHER_SRC
     "*.py"
     )

foreach( _file ${LAUNCHER_SRC})
    configure_file(${_file} ${_file} COPYONLY)
endforeach()
