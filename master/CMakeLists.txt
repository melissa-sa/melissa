
add_library(get_message SHARED get_message.c $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(get_message ${EXTRA_LIBS})
target_compile_options(get_message BEFORE PUBLIC -fPIC -Wall)

configure_file(master.py master.py COPYONLY)
configure_file(fault_tolerance.py fault_tolerance.py COPYONLY)
configure_file(matrix_sobol.py matrix_sobol.py COPYONLY)
configure_file(batch_scripts.py batch_scripts.py COPYONLY)
configure_file(call_bash.py call_bash.py COPYONLY)
configure_file(options.py options.py COPYONLY)
