# Copyright (c) 2021, Institut National de Recherche en Informatique et en Automatique (Inria)
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

if(NOT Python3_EXECUTABLE)
    message(FATAL_ERROR "A Python3 interpreter must be found before searching for NumPy (variable Python3_EXECUTABLE is NOT set)")
    return()
endif()

set(_NumPy_python_code
"import sys\n\
try:\n\
  import numpy\n\
  print(\"{:s};{:s}\".format(numpy.__version__, numpy.get_include()))\n\
except Exception as e:\n\
  print(e.msg, file=sys.stderr)\n\
  sys.exit(1)")

execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "${_NumPy_python_code}"
    RESULT_VARIABLE _NumPy_status
    OUTPUT_VARIABLE _NumPy_out
    ERROR_VARIABLE _NumPy_err
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if(_NumPy_status EQUAL 0)
    list(GET _NumPy_out 0 NumPy_VERSION)
    list(GET _NumPy_out 1 NumPy_INCLUDE_DIR)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    NumPy
    VERSION_VAR NumPy_VERSION
    REQUIRED_VARS NumPy_INCLUDE_DIR
)
