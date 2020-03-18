###################################################################
#                            Melissa                              #
#-----------------------------------------------------------------#
#   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    #
#                                                                 #
# This source is covered by the BSD 3-Clause License.             #
# Refer to the  LICENCE file for further information.             #
#                                                                 #
#-----------------------------------------------------------------#
#  Original Contributors:                                         #
#    Theophile Terraz,                                            #
#    Bruno Raffin,                                                #
#    Alejandro Ribes,                                             #
#    Bertrand Iooss,                                              #
###################################################################

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

# disable in-source builds
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
if(${srcdir} STREQUAL ${bindir})
    message("You are attempting to build in your source Directory.")
    message("You must run cmake from a build directory.")
endif(${srcdir} STREQUAL ${bindir})

project(Melissa VERSION 0.5.0 LANGUAGES C Fortran CXX)

cmake_minimum_required(VERSION 3.2.2)

get_filename_component(srcdir "${CMAKE_SOURCE_DIR}" REALPATH)
get_filename_component(bindir "${CMAKE_BINARY_DIR}" REALPATH)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

include(CTest)
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

# The project version number.
SET(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

option(INSTALL_ZMQ OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wpedantic -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wpedantic -Wextra")

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  # Set install directory to ../install
  get_filename_component(PARENT_DIR ${CMAKE_CURRENT_BINARY_DIR} DIRECTORY)
  set(CMAKE_INSTALL_PREFIX "${PARENT_DIR}/install" CACHE PATH "CMAKE_INSTALL_PREFIX: Install path prefix, prepended onto install directories." FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

#find_package(PkgConfig REQUIRED)

#find_package(PythonLibs)
find_package(PythonInterp 3 REQUIRED)

message(STATUS "Python version ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR} found")
#if(NOT ${PYTHON_VERSION_MAJOR} STREQUAL "3")
#    message(ERROR "Melissa needs Python 3")
#endif(NOT ${PYTHON_VERSION_MAJOR} STREQUAL "3")
find_python_module(numpy REQUIRED)
#find_python_module(openturns REQUIRED)

# OpenTURNS #

#find_package(OpenTURNS REQUIRED)
#message(STATUS "OpenTURNS includes: " ${OPENTURNS_INCLUDE_DIR})
#include_directories(${OPENTURNS_INCLUDE_DIR} ${OPENTURNS_INCLUDE_DIRS})
#add_definitions(${OPENTURNS_DEFINITIONS})
#link_directories(${OPENTURNS_LIBRARY})

# BLAS #

#find_package(BLAS REQUIRED)
#find_path(BLAS_INCLUDE_DIRS cblas.h
#  $ENV{BLAS_HOME}/include)
#message(STATUS "Blas ${BLAS_FOUND} in ${BLAS_INCLUDE_DIRS} ${BLAS_LIBRARIES}")
#include_directories( ${BLAS_INCLUDE_DIR} )
#set(EXTRA_LIBS ${EXTRA_LIBS}
#                 ${BLAS_LIBRARIES} )

#option(BUILD_EXAMPLES "Build examples" ON)
#if(BUILD_EXAMPLES)
#  if(NOT CMAKE_Fortran_COMPILER)
#    set(BUILD_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)
#    message(STATUS "Fortran compiler not found. BUILD_EXAMPLES option turned OFF")
#  endif(NOT CMAKE_Fortran_COMPILER)
#endif(BUILD_EXAMPLES)

# ZeroMQ #

if(INSTALL_ZMQ)
  include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
  ExternalProject_Add(ZeroMQ
#  URL https://github.com/zeromq/zeromq4-1/releases/download/v4.1.6/zeromq-4.1.6.tar.gz
#  URL https://github.com/zeromq/libzmq/archive/v4.2.2.tar.gz
  URL https://github.com/zeromq/libzmq/releases/download/v4.3.1/zeromq-4.3.1.tar.gz
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ZeroMQ
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
             -DCMAKE_INSTALL_LIBDIR:PATH=${CMAKE_INSTALL_PREFIX}/lib)
  set(ZeroMQ_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
  set(ZeroMQ_LIBRARY "${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}zmq${CMAKE_SHARED_LIBRARY_SUFFIX}" )
#  include_directories(${ZeroMQ_INCLUDE_DIR})
  MESSAGE(STATUS "ZeroMQ will be installed")
else(INSTALL_ZMQ)
  find_package(ZeroMQ NO_MODULE REQUIRED)
endif(INSTALL_ZMQ)

include_directories(${ZeroMQ_INCLUDE_DIR})

# MPI #

option(BUILD_WITH_MPI "Build with MPI" ON)

if(BUILD_WITH_MPI)
  find_package(MPI QUIET)
  if(MPI_C_FOUND AND MPI_Fortran_FOUND AND MPI_CXX_FOUND)
      set(EXTRA_LIBS ${EXTRA_LIBS}
                     ${MPI_C_LIBRARIES})
      add_definitions( -DBUILD_WITH_MPI )
      include_directories( ${MPI_INCLUDE_PATH} )
      add_definitions(${MPI_Fortran_COMPILE_FLAGS})
      include_directories(${MPI_Fortran_INCLUDE_PATH})
      link_directories(${MPI_Fortran_LIBRARIES})
      set(EXTRA_LIBS ${EXTRA_LIBS}
                     ${MPI_Fortran_LIBRARIES} )
      message(STATUS "MPI Found")
  else(MPI_C_FOUND AND MPI_Fortran_FOUND AND MPI_CXX_FOUND)
      # Disable  Option if missing dependencies
      set(BUILD_WITH_MPI OFF CACHE BOOL "Build with MPI" FORCE)
      if(NOT MPI_Fortran_FOUND)
          set(BUILD_WITH_MPI OFF CACHE BOOL "Build with MPI" FORCE)
          message(STATUS "MPI Fortran missing. BUILD_WITH_MPI option turned OFF")
      endif(NOT MPI_Fortran_FOUND)
      if(NOT MPI_C_FOUND)
          set(BUILD_WITH_MPI OFF CACHE BOOL "Build with MPI" FORCE)
          message(STATUS "MPI C missing. BUILD_WITH_MPI option turned OFF")
      endif(NOT MPI_C_FOUND)
      if(NOT MPI_CXX_FOUND)
          set(BUILD_WITH_MPI OFF CACHE BOOL "Build with MPI" FORCE)
          message(STATUS "MPI C++ missing. BUILD_WITH_MPI option turned OFF")
      endif(NOT MPI_CXX_FOUND)
  endif(MPI_C_FOUND AND MPI_Fortran_FOUND AND MPI_CXX_FOUND)
endif(BUILD_WITH_MPI)

# OpenMP #

option(BUILD_WITH_OpenMP "Build with OpenMP" OFF)

if(BUILD_WITH_OpenMP)
  find_package(OpenMP)
  if(OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    message(STATUS "OpenMP Found")
    add_definitions( -DBUILD_WITH_OPENMP )
  else(OPENMP_FOUND)
    # Disable  Option if missing dependencies
    set(BUILD_WITH_OpenMP FALSE CACHE BOOL "Build with OpenMP" FORCE)
    message(STATUS "OpenMP missing. BUILD_WITH_OpenMP option turned OFF")
  endif(OPENMP_FOUND)
endif(BUILD_WITH_OpenMP)

## HDF5 #

#include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
#ExternalProject_Add(HDF5
#  URL http://www.hdfgroup.org/ftp/HDF5/current/src/hdf5-1.10.1.tar.gz
#  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/HDF5
#  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DHDF5_ENABLE_PARALLEL:BOOL=ON -DHDF5_BUILD_CPP_LIB:BOOL=OFF
#)
#set(HDF5_DIR ${CMAKE_CURRENT_BINARY_DIR}/HDF5 CACHE FORCE " ")
#set(HDF5_LIBRARY ${HDF5_DIR}/lib/libhdf5-shared.so CACHE INTERNAL " ")
#set(HDF5_INCLUDE_DIR ${HDF5_DIR}/include CACHE INTERNAL " ")
#include_directories( ${HDF5_INCLUDE_DIR} )
#set(EXTRA_LIBS ${EXTRA_LIBS}
#              ${HDF5_LIBRARY} )
#set( ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${HDF5_DIR}/lib/pkgconfig" )

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

find_package(Doxygen)
if(DOXYGEN_FOUND)
  option(BUILD_DOCUMENTATION "Use Doxygen to create the documentation" ON)
  message(STATUS "Doxygen Found")
else(DOXYGEN_FOUND)
  option(BUILD_DOCUMENTATION "Use Doxygen to create the documentation" OFF)
endif(DOXYGEN_FOUND)

# If we run on a Omnipath cluster, we need to check simulation deconnection.

option(CHECK_SIMU_DECONNECTION "check simulation deconnection" OFF)

#set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--export-all-symbols")

# Load local include directories before. This ensures that no old installed headers are
# loaded.
include_directories(BEFORE "${PROJECT_SOURCE_DIR}/common")

add_subdirectory(common)

include_directories(BEFORE "${CMAKE_BINARY_DIR}/api")

add_subdirectory(api)

include_directories(BEFORE "${PROJECT_SOURCE_DIR}/server/stats")

include_directories(BEFORE "${PROJECT_SOURCE_DIR}/server/output")

include_directories(BEFORE "${PROJECT_SOURCE_DIR}/server")

add_subdirectory(server)

add_subdirectory(launcher)

add_subdirectory(tests)

set (EXAMPLES_DIR ${CMAKE_INSTALL_PREFIX}/share/melissa/examples)
add_subdirectory(examples)

set (UTILITY_DIR ${CMAKE_INSTALL_PREFIX}/share/melissa/utility)
add_subdirectory(utility)

# set environement variables
#if(INSTALL_ZMQ)
#  set(ZeroMQ_DIR "export ZeroMQ=${CMAKE_INSTALL_PREFIX}/share/cmake/ZeroMQ")
#endif(INSTALL_ZMQ)
configure_file(melissa_set_env.sh.in melissa_set_env.sh @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/melissa_set_env.sh DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    PERMISSIONS OWNER_WRITE
                OWNER_READ
                OWNER_EXECUTE
                GROUP_READ
                GROUP_EXECUTE
                WORLD_READ
                WORLD_EXECUTE)

if(BUILD_DOCUMENTATION)
  if(NOT DOXYGEN_FOUND)
    message(SEND_ERROR "Doxygen needed to build the documentation. Please install it correctly")
  endif(NOT DOXYGEN_FOUND)

  # Configure the Template Doxyfile for our project
  set(DOX_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/Doxyfile.in
                 ${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen/Doxyfile )
  # Add a custom target to run Doxygen when ever the project is built
  add_custom_target(docs ALL
                    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen/Doxyfile
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Generating API documentation with Doxygen"
                    VERBATIM )
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs/html DESTINATION share/melissa/doc)
endif(BUILD_DOCUMENTATION)

include(CMakePackageConfigHelpers)

configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${MELISSA_CMAKECONFIG_INSTALL_DIR})
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
                                 VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                                 COMPATIBILITY AnyNewerVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
              DESTINATION ${MELISSA_CMAKECONFIG_INSTALL_DIR})
