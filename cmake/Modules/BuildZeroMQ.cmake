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
        URL_HASH SHA256=bcbabe1e2c7d0eec4ed612e10b94b112dd5f06fcefa994a0c79a45d835cd21eb
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ZeroMQ
        CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
                   -DCMAKE_INSTALL_LIBDIR:PATH=${CMAKE_INSTALL_PREFIX}/lib
                   -DZMQ_BUILD_TESTS=OFF)
endif()
