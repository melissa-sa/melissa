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
    melissa_utils.c PROPERTIES COMPILE_DEFINITIONS _DEFAULT_SOURCE
)
set_source_files_properties(
    melissa_comm4py.c PROPERTIES COMPILE_DEFINITIONS _DEFAULT_SOURCE
)


add_library(melissa
    api/melissa_api.c
    melissa_utils.c
    melissa_messages.c
    vector.c
)
target_link_libraries(melissa PUBLIC mpic zmq)
set_target_properties(melissa PROPERTIES VERSION ${PROJECT_VERSION})


add_library(melissa_comm4py SHARED melissa_comm4py.c)
target_link_libraries(melissa_comm4py PUBLIC melissa mpic zmq)
set_target_properties(melissa_comm4py PROPERTIES VERSION ${PROJECT_VERSION})
install(TARGETS melissa_comm4py LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})


add_subdirectory(stats)
add_subdirectory(server)
