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

project(Melissa C CXX Fortran)

cmake_minimum_required(VERSION 3.2.2)
include(CTest)

function(find_python_module module)
        string(TOUPPER ${module} module_upper)
        if(NOT PY_${module_upper})
                if(ARGC GREATER 1 AND ARGV1 STREQUAL "REQUIRED")
                        set(${module}_FIND_REQUIRED TRUE)
                endif()
                # A module's location is usually a directory, but for binary modules
                # it's a .so file.
                execute_process(COMMAND "${PYTHON_EXECUTABLE}" "-c"
                        "import re, ${module}; print re.compile('/__init__.py.*').sub('',${module}.__file__)"
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

set(ALL_SRCS )
set(ALL_INCS )

option(INSTALL_ZMQ OFF)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  # Set install directory to ../install
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/../install" CACHE PATH "CMAKE_INSTALL_PREFIX: Install path prefix, prepended onto install directories." FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

find_package(PkgConfig REQUIRED)

#find_package(PythonLibs)
find_package(PythonInterp REQUIRED)
message(STATUS "Python found")
find_python_module(numpy REQUIRED)

# BLAS #

#find_package(BLAS REQUIRED)
#find_path(BLAS_INCLUDE_DIRS cblas.h
#  $ENV{BLAS_HOME}/include)
#message(STATUS "Blas ${BLAS_FOUND} in ${BLAS_INCLUDE_DIRS} ${BLAS_LIBRARIES}")
#include_directories( ${BLAS_INCLUDE_DIR} )
#set(EXTRA_LIBS ${EXTRA_LIBS}
#                 ${BLAS_LIBRARIES} )

option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
  if(NOT CMAKE_Fortran_COMPILER)
    set(BUILD_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)
    message(STATUS "Fortran compiler not found. BUILD_EXAMPLES option turned OFF")
  endif(NOT CMAKE_Fortran_COMPILER)
endif(BUILD_EXAMPLES)

# ZeroMQ #


#if(EXISTS ${ZMQ_DIR}/lib/pkgconfig)
#  set(INSTALL_ZMQ OFF CACHE BOOL "Download and install ZeroMQ" FORCE)
#  message(STATUS "ZeroMQ Found")
#else(EXISTS ${ZMQ_DIR}/lib/pkgconfig)
#  option(INSTALL_ZMQ "Download and install ZeroMQ" ON)
#  message(STATUS "ZeroMQ not found")
#endif(EXISTS ${ZMQ_DIR}/lib/pkgconfig)

#set(INSTALL_ZMQ OFF CACHE BOOL "Download and install ZeroMQ" FORCE)

if(INSTALL_ZMQ)
  include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
  ExternalProject_Add(ZeroMQ
#    URL https://github.com/zeromq/zeromq4-1/releases/download/v4.1.6/zeromq-4.1.6.tar.gz
    URL https://github.com/zeromq/libzmq/releases/download/v4.2.1/zeromq-4.2.1.tar.gz
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ZeroMQ
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}/ZeroMQ
  )
  set(ZMQ_DIR ${CMAKE_INSTALL_PREFIX}/ZeroMQ CACHE FORCE " ")
#  mark_as_advanced(FORCE ZMQ_DIR)
  set(ZMQ_STATIC_LIBRARY ${ZMQ_DIR}/lib/libzmq-static.a CACHE INTERNAL " ")
  set(ZMQ_LIBRARY ${ZMQ_DIR}/lib/libzmq.so CACHE INTERNAL " ")
  set(ZMQ_INCLUDE_DIR ${ZMQ_DIR}/include CACHE INTERNAL " ")
  include_directories( ${ZMQ_INCLUDE_DIR} )
  set( ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${ZMQ_DIR}/lib/pkgconfig" )
else(INSTALL_ZMQ)
    if(NOT ZMQ_DIR)
        pkg_check_modules(PC_ZeroMQ QUIET libzmq>=4.1.5)
        if(NOT PC_ZeroMQ_FOUND)
            message(FATAL_ERROR "ZeroMQ not found")
        else()
            set (ZMQ_DIR ${PC_ZeroMQ_LIBRARY_DIR})
            set (ZMQ_INCLUDE_DIR ${PC_ZeroMQ_INCLUDE_DIR})
            find_library(ZMQ_STATIC_LIBRARY NAMES libzmq-static.a HINTS ${ZMQ_DIR}/lib ${ZMQ_DIR}/../lib CACHE INTERNAL " ")
            find_library(ZMQ_LIBRARY NAMES libzmq.so HINTS ${ZMQ_DIR}/lib ${ZMQ_DIR}/../lib CACHE INTERNAL " ")
            include_directories( ${ZMQ_INCLUDE_DIR} )
            message(STATUS "ZeroMQ Found")
        endif()
    else(NOT ZMQ_DIR)
        find_path(ZMQ_DIR NAMES include/zmq.h)
        find_path(ZMQ_INCLUDE_DIR NAMES zmq.h HINTS ${ZMQ_DIR}/include ${ZMQ_DIR}/../include CACHE INTERNAL " ")
        find_library(ZMQ_STATIC_LIBRARY NAMES libzmq-static.a HINTS ${ZMQ_DIR}/lib ${ZMQ_DIR}/../lib CACHE INTERNAL " ")
        find_library(ZMQ_LIBRARY NAMES libzmq.so HINTS ${ZMQ_DIR}/lib ${ZMQ_DIR}/../lib CACHE INTERNAL " ")
        include_directories( ${ZMQ_INCLUDE_DIR} )
        set( ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${ZMQ_DIR}/pkgconfig" )
        set( ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${ZMQ_DIR}/lib/pkgconfig" )
        pkg_check_modules(ZMQ QUIET libzmq>=4.1.5)
        if(NOT ZMQ_FOUND)
            message(FATAL_ERROR "Provided ZeroMQ dir not found:" ${ZMQ_DIR})
        endif(NOT ZMQ_FOUND)
    endif(NOT ZMQ_DIR)
endif(INSTALL_ZMQ)

# MPI #

option(BUILD_WITH_MPI "Build with MPI" ON)

if(BUILD_WITH_MPI)
  find_package(MPI)
  if(MPI_FOUND)
    set(EXTRA_LIBS ${EXTRA_LIBS}
                   ${MPI_LIBRARIES} mpi)
    add_definitions( -DBUILD_WITH_MPI )
    include_directories( ${MPI_INCLUDE_PATH} )
    add_definitions(${MPI_Fortran_COMPILE_FLAGS})
    include_directories(${MPI_Fortran_INCLUDE_PATH})
    link_directories(${MPI_Fortran_LIBRARIES})
    set(EXTRA_LIBS ${EXTRA_LIBS}
                   ${MPI_Fortran_LIBRARIES} )
    message(STATUS "MPI Found")
    if(NOT MPI_Fortran_FOUND)
        find_package(MPI REQUIRED)
    endif(NOT MPI_Fortran_FOUND)
  else(MPI_FOUND)
    # Disable  Option if missing dependencies
    set(BUILD_WITH_MPI OFF CACHE BOOL "Build with MPI" FORCE)
    message(STATUS "MPI missing. BUILD_WITH_MPI option turned OFF")
  endif(MPI_FOUND)
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

# Probes #

option(BUILD_WITH_PROBES "Set probes for bench" ON)
if(BUILD_WITH_PROBES)
    add_definitions( -DBUILD_WITH_PROBES )
endif(BUILD_WITH_PROBES)

#set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--export-all-symbols")

set(UTILS_C ${PROJECT_SOURCE_DIR}/utils/melissa_utils.c
            ${PROJECT_SOURCE_DIR}/utils/vector.c )
set(ALL_SRCS ${ALL_SRCS} ${UTILS_C})
set(UTILS_H ${PROJECT_SOURCE_DIR}/utils/melissa_utils.h
            ${PROJECT_SOURCE_DIR}/utils/vector.h )
set(ALL_INCS ${ALL_INCS} ${UTILS_H})
set(DOX_SOURCES "${DOX_SOURCES} ${PROJECT_SOURCE_DIR}/utils/melissa_utils.c ${PROJECT_SOURCE_DIR}/utils/melissa_utils.h")

include_directories("${PROJECT_SOURCE_DIR}/utils")

add_subdirectory(utils)

set(ALL_API melissa_api.c
            melissa_api.h
            melissa_api_no_mpi.h)
foreach( _file ${ALL_API})
  set(DOX_SOURCES "${DOX_SOURCES} ${PROJECT_SOURCE_DIR}/api/${_file}" )
endforeach()

include_directories(AFTER "${PROJECT_SOURCE_DIR}/api")

add_subdirectory(api)

set(STATS_C covariance.c
            variance.c
            mean.c
            general_moments.c
            min_max.c
            sobol.c
            threshold.c
            quantile.c)
set(ALL_SRCS ${ALL_SRCS} ${STATS_C})
set(STATS_H covariance.h
            variance.h
            mean.h
            general_moments.h
            min_max.h
            sobol.h
            threshold.h
            quantile.h)
set(ALL_INCS ${ALL_INCS} ${STATS_H})
foreach( _file ${STATS_C} ${STATS_H})
  set(DOX_SOURCES "${DOX_SOURCES} ${PROJECT_SOURCE_DIR}/stats/${_file}" )
endforeach()

include_directories(AFTER "${PROJECT_SOURCE_DIR}/stats")

add_subdirectory(stats)

set(SERVER_SRC compute_stats.c
               melissa_fields.c
               melissa_data.c
               melissa_io.c
               melissa_options.c
               server_helper.c
               fault_tolerance.c)
set(ALL_SRCS ${ALL_SRCS} ${SERVER_SRC})
set(SERVER_H compute_stats.h
             melissa_fields.h
             melissa_data.h
             melissa_io.h
             melissa_options.h
             server.h
             fault_tolerance.h)
set(ALL_INCS ${ALL_INCS} ${SERVER_H})
foreach( _file ${SERVER_SRC} ${SERVER_H})
  set(DOX_SOURCES "${DOX_SOURCES} ${PROJECT_SOURCE_DIR}/server/${_file}" )
endforeach()

include_directories(AFTER "${PROJECT_SOURCE_DIR}/server")

add_subdirectory(server)

#add_subdirectory(launcher.old)

add_subdirectory(launcher)

add_subdirectory(launcher_saturne)

add_subdirectory(gui)

add_subdirectory(tests)

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif(BUILD_EXAMPLES)

# set environement variables
configure_file(melissa_set_env.sh.in melissa_set_env.sh @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/melissa_set_env.sh DESTINATION ${CMAKE_INSTALL_PREFIX})

if(BUILD_DOCUMENTATION)
  if(NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Doxygen needed to build the documentation. Please install it correctly")
  endif(NOT DOXYGEN_FOUND)

  # Configure the Template Doxyfile for our project
  set(DOX_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/Doxyfile.in
                 ${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen/Doxyfile )
  # Add a custom target to run Doxygen when ever the project is built
  add_custom_target(docs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen/Doxyfile
    SOURCES ${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen/Doxyfile )
endif(BUILD_DOCUMENTATION)
