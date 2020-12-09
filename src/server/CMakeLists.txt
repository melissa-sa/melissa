# Copyright (c) 2017, Institut National de Recherche en Informatique et en Automatique (Inria)
#               2017, EDF (https://www.edf.fr/)
#               2020, Institut National de Recherche en Informatique et en Automatique (Inria)
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

set(MELISSA_OUTPUT_MODULE "txt"
    CACHE STRING "Choose which output module will be used to write statistics"
)
set_property(CACHE MELISSA_OUTPUT_MODULE PROPERTY STRINGS "text;netcdf")


add_executable(melissa-server
    compute_stats.c
    fault_tolerance.c
    fields.c
    data.c
    io.c
    options.c
    server.c
    server_helper.c
    output/output.c
    output/output_${MELISSA_OUTPUT_MODULE}.c
)
target_link_libraries(melissa-server melissa-stats)


if(${MELISSA_OUTPUT_MODULE} STREQUAL "netcdf")
    find_path(NETCDF_INCLUDE_DIR netcdf.h)
    find_library(NETCDF_LIBRARY netcdf)

    if(NETCDF_INCLUDE_DIR AND NETCDF_LIBRARY)
        target_include_directories(melissa-server ${NETCDF_INCLUDES})
        target_link_libraries(melissa-server ${NETCDF_LIBRARY})
    else()
        message(SEND_ERROR "netCDF output requested but netCDF not found")
    endif()
endif()


install(
    TARGETS melissa-server
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
