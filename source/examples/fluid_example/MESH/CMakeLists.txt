 

foreach( _file downcomer.des
               fdc.des
               pic.des)
    configure_file(${_file} ${_file} COPYONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${_file} DESTINATION ${EXAMPLES_DIR}/fluid_example/MESH)
endforeach()
