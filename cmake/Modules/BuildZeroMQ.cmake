# Copyright (c) 2020, Institut National de Recherche en Informatique et en Automatique (Inria)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

option(INSTALL_ZMQ "Build ZeroMQ" OFF)

include(GNUInstallDirs)

# ZeroMQ is written in C++ and must be linked against a C++ standard library.
# This build script cannot determine the C++ standard library implementation
# used by ZeroMQ and this is a problem when building a *static* ZeroMQ library.

if(INSTALL_ZMQ)
    set(ZeroMQ_VERSION 4.3.1 CACHE INTERNAL "ZeroMQ version")
    set(ZeroMQ_INCLUDE_DIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}"
        CACHE PATH "ZeroMQ include directory")
    set(ZeroMQ_LIBRARY "${CMAKE_INSTALL_FULL_LIBDIR}/${CMAKE_SHARED_LIBRARY_PREFIX}zmq${CMAKE_SHARED_LIBRARY_SUFFIX}"
        CACHE FILEPATH "ZeroMQ library")

    mark_as_advanced(ZeroMQ_VERSION ZeroMQ_INCLUDE_DIR ZeroMQ_LIBRARY)

    include(ExternalProject)
    ExternalProject_Add(
        ZeroMQ
        URL https://github.com/zeromq/libzmq/releases/download/v4.3.1/zeromq-4.3.1.tar.gz
        URL_HASH SHA256=bcbabe1e2c7d0eec4ed612e10b94b112dd5f06fcefa994a0c79a45d835cd21eb
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ZeroMQ
        CMAKE_ARGS
            -DBUILD_SHARED:BOOL=ON
            -DBUILD_STATIC:BOOL=OFF
            -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
            -DCMAKE_INSTALL_LIBDIR:PATH=${CMAKE_INSTALL_FULL_LIBDIR}
            -DWITH_PERF_TOOL:BOOL=OFF
            -DWITH_DOCS:BOOL=OFF
            -DZMQ_BUILD_TESTS:BOOL=OFF
    )
endif()
