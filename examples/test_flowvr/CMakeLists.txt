
configure_file(test_flowvr.py test_flowvr.py COPYONLY)

add_executable(data_generator data_generator.cpp)
target_link_libraries(data_generator ${EXTRA_LIBS} stats)
