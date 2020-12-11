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

set(destination ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/examples/TELEMAC-2D)

foreach(file IN ITEMS
    README.txt
    geo_gouttedo.cli
    geo_gouttedo.slf
    t2d_gouttedo.cas
    user_fortran/condin.f
    user_fortran/href.f
)
    get_filename_component(directory ${file} DIRECTORY)
    install(
        FILES ${file}
        DESTINATION ${destination}/${directory}
    )
endforeach()

configure_file(options.py.in options.py @ONLY)
configure_file(create_group.py.in create_group.py @ONLY)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/options.py
        ${CMAKE_CURRENT_BINARY_DIR}/create_group.py
    DESTINATION ${destination}
)


add_executable(melissa_to_serafin melissa_to_serafin.f)
target_compile_options(melissa_to_serafin BEFORE PUBLIC "-fconvert=big-endian")
install(TARGETS melissa_to_serafin RUNTIME DESTINATION ${destination})
