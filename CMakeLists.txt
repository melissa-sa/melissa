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

cmake_minimum_required(VERSION 3.7.2)

project(melissa VERSION 0.7.0 LANGUAGES C Fortran)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)


# Disable in-source builds
#
# CMAKE_SOURCE_DIR, CMAKE_BINARY_DIR cannot be reliably compared directly
# because there might be symbolic links in the path
get_filename_component(srcdir "${CMAKE_SOURCE_DIR}" REALPATH)
get_filename_component(bindir "${CMAKE_BINARY_DIR}" REALPATH)

if(${srcdir} STREQUAL ${bindir})
    message("You are attempting to build in your source Directory.")
    message("You must run cmake from a build directory.")
endif(${srcdir} STREQUAL ${bindir})

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    # Set install directory to ../install
    get_filename_component(PARENT_DIR ${CMAKE_CURRENT_BINARY_DIR} DIRECTORY)
    set(CMAKE_INSTALL_PREFIX "${PARENT_DIR}/install"
        CACHE PATH "Install path prefix, prepended onto install directories." FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)


# options
option(BUILD_SHARED_LIBS "Build shared libraries" ON)



# set up
include(CTest)
include(GNUInstallDirs)

set(MELISSA_INSTALL_PYTHONDIR ${CMAKE_INSTALL_LIBDIR}/python3/dist-packages
    CACHE PATH "Python3 packages"
)
set(MELISSA_INSTALL_FULL_PYTHONDIR
    ${CMAKE_INSTALL_PREFIX}/${MELISSA_INSTALL_PYTHONDIR}
)

# move compiled binaries directly into bin, lib dir
# this way they can be found easier for testing
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})


# Python configuration
include(FindPackageHandleStandardArgs)

function(find_python_module module)
    string(TOUPPER ${module} module_upper)
    if(NOT PY_${module_upper})
        if(ARGC GREATER 1 AND ARGV1 STREQUAL "REQUIRED")
            set(${module}_FIND_REQUIRED TRUE)
        endif()
        # A module's location is usually a directory, but for binary modules
        # it's a .so file.
        execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c"
                        "import re, ${module}; print(re.compile('/__init__.py.*').sub('',${module}.__file__))"
                        RESULT_VARIABLE _${module}_status
                        OUTPUT_VARIABLE _${module}_location
                        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(NOT _${module}_status)
            set(PY_${module_upper} ${_${module}_location} CACHE STRING
                "Location of Python module ${module}")
        endif(NOT _${module}_status)
    endif(NOT PY_${module_upper})
    find_package_handle_standard_args(PY_${module} DEFAULT_MSG PY_${module_upper})
endfunction(find_python_module)

find_package(PythonInterp 3.5.3 REQUIRED)
find_python_module(numpy REQUIRED)


# C language options
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra -Wall -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-sign-compare")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-discarded-qualifiers")

include_directories(include)
include_directories(${CMAKE_BINARY_DIR}/include)


# Find ZeroMQ
include(BuildZeroMQ)

if(INSTALL_ZMQ)
    if(ZeroMQ_ROOT)
        message(WARNING "INSTALL_ZMQ and ZeroMQ_ROOT both set. ZeroMQ_ROOT will be ignored.")
    endif()
else()
    find_package(ZeroMQ REQUIRED)
endif()

add_library(zmq IMPORTED SHARED GLOBAL)

if(INSTALL_ZMQ)
    add_dependencies(zmq ZeroMQ)
    # CMake stops with an error if INTERFACE_INCLUDE_DIRECTORIES do not exist
    # (see the call to set_target_properties() below)
    # Create the directory now to avoid the error message
    file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/include)
endif()

set_target_properties(
    zmq
    PROPERTIES
    IMPORTED_LOCATION ${ZeroMQ_LIBRARY}
    INTERFACE_INCLUDE_DIRECTORIES ${ZeroMQ_INCLUDE_DIR}
    NO_SYSTEM_FROM_IMPORTED ON
    SOVERSION ${ZeroMQ_VERSION}
)



# MPI
find_package(MelissaMPI REQUIRED)


# FlowVR #

option(BUILD_WITH_FLOWVR "Build with FlowVR interface" OFF)

if(BUILD_WITH_FLOWVR)
  find_package(FlowVR)
  if(FLOWVR_FOUND)
    message(STATUS "FlowVR Found")
    add_definitions( -DBUILD_WITH_FLOWVR )
    include_directories(${FLOWVR_INCLUDE_DIR})
    set(FLOWVR_LIBS ${EXTRA_LIBS}
                    ${FLOWVR_base_LIBRARY}
                    ${FLOWVR_ftl_LIBRARY}
                    ${FLOWVR_mod_LIBRARY}
                    ${CMAKE_THREAD_LIBS_INIT} )
    set(EXTRA_LIBS ${EXTRA_LIBS}
                   ${FLOWVR_LIBS})
  else(FLOWVR_FOUND)
    # Disable  Option if missing dependencies
    set(BUILD_WITH_FLOWVR FALSE CACHE BOOL "Build with FlowVR interface" FORCE)
    message(STATUS "FlowVR Not Found. BUILD_WITH_FLOWVR option turned OFF")
  endif(FLOWVR_FOUND)
endif(BUILD_WITH_FLOWVR)

# Doxygen #

option(BUILD_DOCUMENTATION "Use Doxygen to create the documentation" OFF)
if (BUILD_DOCUMENTATION)
    find_package(Doxygen)
    if(NOT DOXYGEN_FOUND)
        message(SEND_ERROR "Doxygen not found. Cannot build documentation.")
    endif(DOXYGEN_FOUND)
    configure_file(doc/doxygen/Doxyfile.in doc/doxygen/Doxyfile @ONLY)
    # Add a custom target to run Doxygen when ever the project is built
    add_custom_target(
        doc ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/doc/doxygen/Doxyfile
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
    install(DIRECTORY doc/html DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif(BUILD_DOCUMENTATION)


#
set (EXAMPLES_DIR ${CMAKE_INSTALL_PREFIX}/share/melissa/examples)
set (UTILITY_DIR ${CMAKE_INSTALL_PREFIX}/share/melissa/utility)
set(MELISSA_INSTALL_CMAKEDIR
    ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)


add_subdirectory(bin)
add_subdirectory(examples)
add_subdirectory(include)
add_subdirectory(melissa)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(utility)


# Melissa4py #

option(MELISSA4PY_INSTALL "Build with Melissa4py (Python server)" OFF)

if(MELISSA4PY_INSTALL)
  message("Installing melissa4py")
  add_subdirectory(utility/melissa4py)
endif(MELISSA4PY_INSTALL)
