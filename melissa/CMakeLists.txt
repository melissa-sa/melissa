# Copyright (c) 2017, Institut National de Recherche en Informatique et en Automatique (https://www.inria.fr/)
#               2017, EDF (https://www.edf.fr/)
#               2020, Institut National de Recherche en Informatique et en Automatique (https://www.inria.fr/)
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


set(python_files
    __init__.py
    launcher/__init__.py
    launcher/__main__.py
    launcher/options.py
    launcher/simulation.py
    launcher/study.py
)


foreach(file IN LISTS python_files)
    get_filename_component(directory ${file} DIRECTORY)

    # copy files for testing
    configure_file(${file} ${file} COPYONLY)

    install(
        FILES ${file}
        DESTINATION ${MELISSA_INSTALL_PYTHONDIR}/melissa/${directory}
    )
endforeach()


# config.py for installation
configure_file(config.py.in config-install.py @ONLY)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/config-install.py
    DESTINATION ${MELISSA_INSTALL_PYTHONDIR}/melissa
    RENAME config.py
)


# config.py for testing
set(CMAKE_INSTALL_FULL_LIBDIR ${CMAKE_BINARY_DIR}/common)
configure_file(config.py.in config.py @ONLY)
