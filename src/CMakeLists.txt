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

# make gethostname() declaration accessible
set_source_files_properties(
    comm4py.c PROPERTIES COMPILE_DEFINITIONS _DEFAULT_SOURCE
)
# make gethostname(), struct timespec, clock_gettime() declaration available
set_source_files_properties(
    utils.c PROPERTIES COMPILE_DEFINITIONS "_BSD_SOURCE;_DEFAULT_SOURCE;_POSIX_C_SOURCE=199309L"
)


add_library(melissa
    messages.c
    utils.c
    vector.c
    api/melissa_api.c
    stats/covariance.c
    stats/general_moments.c
    stats/mean.c
    stats/min_max.c
    stats/quantile.c
    stats/sobol.c
    stats/threshold.c
    stats/variance.c
)
target_include_directories(
    melissa
    INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}>
    PUBLIC ${MPI_C_INCLUDE_PATH}

)
target_compile_options(melissa PUBLIC ${MPI_C_COMPILE_FLAGS})
target_link_libraries(
    melissa
    PRIVATE m zmq
    PUBLIC ${MPI_C_LINK_FLAGS} ${MPI_C_LIBRARIES}
)
set_target_properties(melissa PROPERTIES VERSION ${PROJECT_VERSION})

install(
    TARGETS melissa
    EXPORT melissa-export
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    EXPORT melissa-export
    FILE melissa-config.cmake
    DESTINATION ${MELISSA_INSTALL_CMAKEDIR}
)


add_library(melissa_comm4py SHARED comm4py.c)
target_link_libraries(melissa_comm4py PUBLIC melissa mpi_C zmq)
set_target_properties(melissa_comm4py PROPERTIES VERSION ${PROJECT_VERSION})
install(
    TARGETS melissa_comm4py
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)


if(MELISSA_ENABLE_NO_MPI_API)
    # TODO: fix filenames
    install(
        CODE "execute_process( \
            COMMAND ${CMAKE_COMMAND} -E create_symlink \
                libmelissa.so \
                ${CMAKE_INSTALL_FULL_LIBDIR}/libmelissa_api.so \
        )"
    )
endif()


add_subdirectory(server)
