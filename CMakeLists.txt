project(Melissa C CXX Fortran)

cmake_minimum_required(VERSION 3.0)
include(CTest)

#function(find_python_module module)
#        string(TOUPPER ${module} module_upper)
#        if(NOT PY_${module_upper})
#                if(ARGC GREATER 1 AND ARGV1 STREQUAL "REQUIRED")
#                        set(${module}_FIND_REQUIRED TRUE)
#                endif()
#                # A module's location is usually a directory, but for binary modules
#                # it's a .so file.
#                execute_process(COMMAND "${PYTHON_EXEC}" "-c"
#                        "import re, ${module}; print re.compile('/__init__.py.*').sub('',${module}.__file__)"
#                        RESULT_VARIABLE _${module}_status
#                        OUTPUT_VARIABLE _${module}_location
#                        ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
#                if(NOT _${module}_status)
#                        set(PY_${module_upper} ${_${module}_location} CACHE STRING
#                                "Location of Python module ${module}")
#                endif(NOT _${module}_status)
#        endif(NOT PY_${module_upper})
#        find_package_handle_standard_args(PY_${module} DEFAULT_MSG PY_${module_upper})
#endfunction(find_python_module)

set(ALL_SRCS )
set(ALL_INCS )

option(BUILD_SHARED_LIBS ON)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  # Set install directory to ../install
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/../install" CACHE PATH "CMAKE_INSTALL_PREFIX: Install path prefix, prepended onto install directories." FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

find_package(PkgConfig REQUIRED)

find_package(PythonLibs)

option(BUILD_EXAMPLES "Build examples" ON)

# ZeroMQ #

if(NOT ZMQ_DIR)
  message(STATUS "ZeroMQ dir not found:" ${ZMQ_DIR})
else()
  message(STATUS "ZeroMQ directory:" ${ZMQ_DIR})
endif()

if(EXISTS ${ZMQ_DIR}/lib/pkgconfig)
  set(INSTALL_ZMQ OFF CACHE BOOL "Download and install ZeroMQ" FORCE)
  message(STATUS "ZeroMQ Found")
else(EXISTS ${ZMQ_DIR}/lib/pkgconfig)
  option(INSTALL_ZMQ "Download and install ZeroMQ" ON)
  message(STATUS "ZeroMQ not found")
endif(EXISTS ${ZMQ_DIR}/lib/pkgconfig)

if(INSTALL_ZMQ)
  include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
  ExternalProject_Add(ZeroMQ
    URL https://github.com/zeromq/zeromq4-1/releases/download/v4.1.6/zeromq-4.1.6.tar.gz
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ZeroMQ
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
  )
  set(ZMQ_DIR ${CMAKE_CURRENT_BINARY_DIR}/ZeroMQ CACHE FORCE " ")
  mark_as_advanced(FORCE ZMQ_DIR)
  set(ZMQ_LIBRARY ${ZMQ_DIR}/lib/libzmq.so CACHE INTERNAL " ")
  set(ZMQ_INCLUDE_DIR ${ZMQ_DIR}/include CACHE INTERNAL " ")
  include_directories( ${ZMQ_INCLUDE_DIR} )
  set(EXTRA_LIBS ${EXTRA_LIBS}
                 ${ZMQ_LIBRARY} )
  set( ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${ZMQ_DIR}/lib/pkgconfig" )
else(INSTALL_ZMQ)
  find_path(ZMQ_DIR NAMES include/zmq.h)
  find_path(ZMQ_INCLUDE_DIR NAMES zmq.h HINTS ${ZMQ_DIR}/include CACHE INTERNAL " ")
  find_library(ZMQ_LIBRARY NAMES zmq libzmq HINTS ${ZMQ_DIR}/lib CACHE INTERNAL " ")
  include_directories( ${ZMQ_INCLUDE_DIR} )
  set(EXTRA_LIBS ${EXTRA_LIBS}
                 ${ZMQ_LIBRARY} )
  set( ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${ZMQ_DIR}/lib/pkgconfig" )
  pkg_check_modules(ZMQ REQUIRED libzmq>=4.1.5)
endif(INSTALL_ZMQ)

option(BUILD_WITH_MPI "Build with MPI" ON)

if(BUILD_WITH_MPI)
  find_package(MPI)
  if(MPI_FOUND)
    set(EXTRA_LIBS ${EXTRA_LIBS}
                   ${MPI_LIBRARIES})
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
    if(BUILD_EXAMPLES)
      option(BUILD_EXAMPLES_WITH_MPI "Build examples with MPI" ON)
      mark_as_advanced(CLEAR BUILD_EXAMPLES_WITH_MPI)
    else(BUILD_EXAMPLES)
      option(BUILD_EXAMPLES_WITH_MPI "Build examples with MPI" OFF)
      mark_as_advanced(FORCE BUILD_EXAMPLES_WITH_MPI)
    endif(BUILD_EXAMPLES)
  else(MPI_FOUND)
    # Disable  Option if missing dependencies
    set(BUILD_WITH_MPI OFF CACHE BOOL "Build with MPI" FORCE)
    set(BUILD_EXAMPLES_WITH_MPI OFF CACHE BOOL "Build examples with MPI" FORCE)
    mark_as_advanced(FORCE BUILD_EXAMPLES_WITH_MPI)
    message(STATUS "MPI missing. BUILD_WITH_MPI option turned OFF")
  endif(MPI_FOUND)
else(BUILD_WITH_MPI)
  set(BUILD_EXAMPLES_WITH_MPI OFF CACHE BOOL "Build examples with MPI" FORCE)
  mark_as_advanced(FORCE BUILD_EXAMPLES_WITH_MPI)
endif(BUILD_WITH_MPI)

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
                   ${FLOWVR_LIBS} )
  else(FLOWVR_FOUND)
    # Disable  Option if missing dependencies
    set(BUILD_WITH_FLOWVR FALSE CACHE BOOL "Build with FlowVR interface" FORCE)
    message(STATUS "FlowVR Not Found. BUILD_WITH_FLOWVR option turned OFF")
  endif(FLOWVR_FOUND)
endif(BUILD_WITH_FLOWVR)

find_package(Doxygen)
if(DOXYGEN_FOUND)
  option(BUILD_DOCUMENTATION "Use Doxygen to create the documentation" ON)
  message(STATUS "Doxygen Found")
else(DOXYGEN_FOUND)
  option(BUILD_DOCUMENTATION "Use Doxygen to create the documentation" OFF)
endif(DOXYGEN_FOUND)

option(BUILD_WITH_PROBES "Set probes for bench" ON)
if(BUILD_WITH_PROBES)
    add_definitions( -DBUILD_WITH_PROBES )
endif(BUILD_WITH_PROBES)

#set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--export-all-symbols")

set(UTILS_C ${PROJECT_SOURCE_DIR}/utils/melissa_utils.c )
set(ALL_SRCS ${ALL_SRCS} ${UTILS_C})
set(UTILS_H ${PROJECT_SOURCE_DIR}/utils/melissa_utils.h )
set(ALL_INCS ${ALL_INCS} ${UTILS_H})
set(DOX_SOURCES "${DOX_SOURCES} ${PROJECT_SOURCE_DIR}/utils/melissa_utils.c ${PROJECT_SOURCE_DIR}/utils/melissa_utils.h")

include_directories("${PROJECT_SOURCE_DIR}/utils")

add_subdirectory(utils)

set(STATS_C covariance.c
            variance.c
            mean.c
            min_max.c
            sobol.c
            threshold.c
            quantile.c)
set(ALL_SRCS ${ALL_SRCS} ${STATS_C})
set(STATS_H covariance.h
            variance.h
            mean.h
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
               melissa_data.c
               melissa_io.c
               melissa_options.c
               server_helper.c)
set(ALL_SRCS ${ALL_SRCS} ${SERVER_SRC})
set(SERVER_H compute_stats.h
             melissa_data.h
             melissa_io.h
             melissa_options.h
             server.h)
set(ALL_INCS ${ALL_INCS} ${SERVER_H})
foreach( _file ${SERVER_SRC} ${SERVER_H})
  set(DOX_SOURCES "${DOX_SOURCES} ${PROJECT_SOURCE_DIR}/server/${_file}" )
endforeach()

include_directories(AFTER "${PROJECT_SOURCE_DIR}/server")

add_subdirectory(server)

add_subdirectory(launcher.old)

add_subdirectory(launcher_templates)

add_subdirectory(launcher_saturne)

add_subdirectory(gui)

set(ALL_API melissa_api.c
            melissa_api.h
            melissa_api_no_mpi.h)
foreach( _file ${ALL_API})
  set(DOX_SOURCES "${DOX_SOURCES} ${PROJECT_SOURCE_DIR}/api/${_file}" )
endforeach()

include_directories(AFTER "${PROJECT_SOURCE_DIR}/api")

add_subdirectory(api)

add_subdirectory(tests)

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif(BUILD_EXAMPLES)

if(BUILD_DOCUMENTATION)
  if(NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Doxygen is needed to build the documentation. Please install it correctly")
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
