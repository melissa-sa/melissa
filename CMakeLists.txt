# Copyright (c) 2017, Institut National de Recherche en Informatique et en Automatique (https://www.inria.fr/)
#               2017, EDF (https://www.edf.fr/)
#               2020, Institut National de Recherche en Informatique et en Automatique (https://www.inria.fr/)
#               2021, Institut National de Recherche en Informatique et en Automatique (Inria)
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

project(melissa VERSION 0.7.0 LANGUAGES C)

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
option(MELISSA_INSTALL_PYTHON_PACKAGE "Install Melissa Python package" OFF)
option(MELISSA_ENABLE_NO_MPI_API "Enable deprecated *_no_mpi functions" OFF)



# set up
include(CTest)
include(GNUInstallDirs)

# move compiled binaries directly into bin, lib dir
# this way they can be found easier for testing
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})


# Python configuration
# ensure all current Python3 versions are found
if(CMAKE_VERSION VERSION_LESS_EQUAL 3.11)
    set(Python_ADDITIONAL_VERSIONS 3.9 3.8)
elseif(CMAKE_VERSION VERSION_LESS_EQUAL 3.16)
    set(Python_ADDITIONAL_VERSIONS 3.9)
endif()
# PythonInterp has been deprecated in CMake 3.12 (November 2018)
find_package(PythonInterp 3.5.3 REQUIRED)
# The newer FindPython3 module sets the variable Python3_EXECUTABLE
set(Python3_EXECUTABLE ${PYTHON_EXECUTABLE})
find_package(NumPy REQUIRED)


# determine Python3 package installation directory
if(MELISSA_INSTALL_PYTHON_PACKAGE)
    if(CMAKE_INSTALL_PREFIX STREQUAL $ENV{HOME})
        set(python3_site_command "import site; print(site.getusersitepackages())")
    else()
        set(python3_site_command "import site; print(site.getsitepackages()[0])")
    endif()

    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} -c "${python3_site_command}"
        TIMEOUT 10
        OUTPUT_VARIABLE melissa_python_dir_init
        ERROR_VARIABLE python3_stderr
        RESULT_VARIABLE python3_exit_status
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT ${python3_exit_status} EQUAL 0)
        message(
            SEND_ERROR
            "error when querying Python3 for package locations:\n${python3_stderr}"
        )
    endif()
else()
    set(melissa_python_dir_init ${CMAKE_INSTALL_FULL_DATAROOTDIR}/melissa)
endif()

set(MELISSA_INSTALL_PYTHONDIR ${melissa_python_dir_init}
    CACHE PATH "Melissa Python package install path"
)

unset(melissa_python_dir_init)


# C language options
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra -Wall -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-sign-compare")

include_directories(include)
include_directories(${CMAKE_BINARY_DIR}/include)


# Find ZeroMQ
include(BuildZeroMQ)

if(INSTALL_ZMQ)
    if(ZeroMQ_ROOT)
        message(WARNING "INSTALL_ZMQ and ZeroMQ_ROOT both set. ZeroMQ_ROOT will be ignored.")
    endif()
else()
    find_package(ZeroMQ 4.1.5 REQUIRED)
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



# Doxygen

option(BUILD_DOCUMENTATION "Build Doxygen documentation" OFF)

if(BUILD_DOCUMENTATION)
    # do not use `find_package(Doxygen REQUIRED dot)` because if Doxygen is
    # installed but Graphviz is not, then CMake 3.13 prints an error message
    # where a user must look closely to see that Graphviz is missing and not
    # Doxygen:
    # CMake Error at /usr/share/cmake-3.13/Modules/FindPackageHandleStandardArgs.cmake:137 (message):
    #   Could NOT find Doxygen (missing: dot) (found version "1.8.13")
    # Call Stack (most recent call first):
    #   /usr/share/cmake-3.13/Modules/FindPackageHandleStandardArgs.cmake:378 (_FPHSA_FAILURE_MESSAGE)
    #   /usr/share/cmake-3.13/Modules/FindDoxygen.cmake:615 (find_package_handle_standard_args)
    #   CMakeLists.txt:191 (find_package)
    find_package(Doxygen)
    if(NOT DOXYGEN_FOUND)
        message(SEND_ERROR "Doxygen not found. Cannot build documentation.")
    endif()
    if(NOT DOXYGEN_DOT_FOUND)
        message(
            WARNING
            "Graphviz not found. Documentation will be built without dependency graphs.")
    endif()
    configure_file(doc/doxygen/Doxyfile.in doc/doxygen/Doxyfile @ONLY)
    # Add a custom target to run Doxygen when ever the project is built
    add_custom_target(
        doc ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/doc/doxygen/Doxyfile
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/doc
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
