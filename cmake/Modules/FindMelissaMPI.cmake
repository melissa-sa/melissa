# Copyright (c) 2020, Institut National de Recherche en Informatique et en Automatique (https://www.inria.fr/)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

find_package(MPI)

foreach(_language IN ITEMS C CXX Fortran)
    if(NOT CMAKE_${_language}_COMPILER_WORKS)
        continue()
    endif()

    if(MelissaMPI_FIND_REQUIRED AND NOT MPI_${_language}_FOUND)
        message(SEND_ERROR "MPI_${_language} not found")
    endif()

    # fix compilation on CentOS 8 with CMake 3.11.4, OpenMPI 4.0.2
    separate_arguments(MPI_${_language}_COMPILE_FLAGS)

    # remove leading, trailing whitespace when CMake 3.7 + MPICH is used
    if(MPI_${_language}_LINK_FLAGS)
        string(STRIP ${MPI_${_language}_LINK_FLAGS} MPI_${_language}_LINK_FLAGS)
    endif()

    # use an interface instead of an imported library because MPI_X_LIBRARIES
    # may contain several libraries, e.g., on Debian 9 with OpenMPI 2 for
    # Fortran.
    set(_target_name mpi_${_language})
    add_library(${_target_name} INTERFACE)
    target_include_directories(
        ${_target_name} INTERFACE ${MPI_${_language}_INCLUDE_PATH}
    )
    target_compile_options(
        ${_target_name} INTERFACE ${MPI_${_language}_COMPILE_FLAGS}
    )
    target_link_libraries(
        ${_target_name}
        INTERFACE ${MPI_${_language}_LINK_FLAGS} ${MPI_${_language}_LIBRARIES}
    )
endforeach()
