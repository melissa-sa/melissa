# Copyright 2020 Inria (https://www.inria.fr/)

option(INSTALL_ZMQ "Build ZeroMQ" OFF)

if(INSTALL_ZMQ)
    set(ZeroMQ_VERSION 4.3.1 CACHE INTERNAL "ZeroMQ version")
    set(ZeroMQ_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include"
        CACHE PATH "ZeroMQ include directory")
    set(ZeroMQ_LIBRARY "${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}zmq${CMAKE_SHARED_LIBRARY_SUFFIX}"
        CACHE FILEPATH "ZeroMQ library")

    mark_as_advanced(ZeroMQ_VERSION ZeroMQ_INCLUDE_DIR ZeroMQ_LIBRARY)

    include(ExternalProject)
    ExternalProject_Add(
        ZeroMQ
        URL https://github.com/zeromq/libzmq/releases/download/v4.3.1/zeromq-4.3.1.tar.gz
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ZeroMQ
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
                   -DCMAKE_INSTALL_LIBDIR:PATH=${CMAKE_INSTALL_PREFIX}/lib
                   -DZMQ_BUILD_TESTS=OFF)
endif()
