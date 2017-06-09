
add_library(get_message SHARED get_message.c $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(get_message ${EXTRA_LIBS})
target_compile_options(get_message BEFORE PUBLIC -fPIC -Wall)


file(GLOB LAUNCHER_SRC
     "*.py"
     )

foreach( _file ${LAUNCHER_SRC})
    configure_file(${_file} ${_file} COPYONLY)
endforeach()
