project(stats C CXX Fortran)

cmake_minimum_required(VERSION 2.8)

set(ALL_SRCS stats_base.c
             cond_mean.c
             cond_var.c
             covariance.c
             sobol.c
             get_options.c
             stats_data.c
             compute_stats.c
             server.c
             stats.h)

set(ALL_API stats_api.c
            stats_api.h
            stats_api_no_mpi.h)
set(ALL_API_fortran stats_api_fortran.f90)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  # Set install directory to ../install
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/../install" CACHE PATH "CMAKE_INSTALL_PREFIX: Install path prefix, prepended onto install directories." FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

option(BUILD_WITH_ZMQ "Build with ZeroMQ" ON)

if(BUILD_WITH_ZMQ)
  find_path(ZMQ_INCLUDE_DIR zmq.h)
  find_library(ZMQ_LIBRARY zmq)
  set(ZMQ_INCLUDE_DIRS ${ZMQ_INCLUDE_DIR})
  include_directories( ${ZMQ_INCLUDE_DIR} )
  if(ZMQ_LIBRARY)
    set(EXTRA_LIBS ${EXTRA_LIBS}
                   ${ZMQ_LIBRARY} )
  endif(ZMQ_LIBRARY)
  if(ZMQ_LIBRARY AND ZMQ_INCLUDE_DIRS)
    set(ZMQ_FOUND TRUE)
    message(STATUS "ZeroMQ Found")
    add_definitions( -DBUILD_WITH_ZMQ )
  else(ZMQ_LIBRARY AND ZMQ_INCLUDE_DIRS)
    # Disable  Option if missing dependencies
    set(BUILD_WITH_ZMQ FALSE CACHE BOOL "Build with ZeroMQ" FORCE)
    message(STATUS "ZeroMQ missing. BUILD_WITH_ZMQ option turned OFF")
  endif(ZMQ_LIBRARY AND ZMQ_INCLUDE_DIRS)
endif(BUILD_WITH_ZMQ)

option(BUILD_WITH_MPI "Build with MPI" ON)

if(BUILD_WITH_MPI)
  find_package(MPI)
  if(MPI_FOUND)
    set(EXTRA_LIBS ${EXTRA_LIBS}
                   ${MPI_LIBRARIES} )
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
    set(BUILD_WITH_MPI FALSE CACHE BOOL "Build with MPI" FORCE)
    message(STATUS "MPI missing. BUILD_WITH_MPI option turned OFF")
  endif(MPI_FOUND)
endif(BUILD_WITH_MPI)

option(BUILD_WITH_OpenMP "Build with OpenMP" OFF)

if(BUILD_WITH_OpenMP)
  find_package(OpenMP)
  if(OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    message(STATUS "OpenMP Found")
  else(OPENMP_FOUND)
    # Disable  Option if missing dependencies
    set(BUILD_WITH_OpenMP FALSE CACHE BOOL "Build with OpenMP" FORCE)
    message(STATUS "OpenMP missing. BUILD_WITH_OpenMP option turned OFF")
  endif(OPENMP_FOUND)
endif(BUILD_WITH_OpenMP)

option(BUILD_WITH_FLOWVR "Build with FlowVR interface" ON)

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

if(BUILD_DOCUMENTATION)
  if(NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Doxygen is needed to build the documentation. Please install it correctly")
  endif(NOT DOXYGEN_FOUND)

  set(DOX_SOURCES )
  foreach( _file ${ALL_SRCS} )
    set(DOX_SOURCES "${DOX_SOURCES} ${PROJECT_SOURCE_DIR}/src/${_file}" )
  endforeach()

  # Configure the Template Doxyfile for our project
  set(DOX_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/Doxyfile.in
                 ${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen/Doxyfile )
  # Add a custom target to run Doxygen when ever the project is built
  add_custom_target(docs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen/Doxyfile
    SOURCES ${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen/Doxyfile )
endif(BUILD_DOCUMENTATION)

option(BUILD_WITH_PROBES "Set probes for bench" ON)
if(BUILD_WITH_PROBES)
    add_definitions( -DBUILD_WITH_PROBES )
endif(BUILD_WITH_PROBES)

include_directories("${PROJECT_SOURCE_DIR}/src")
add_subdirectory(src)

include_directories("${PROJECT_SOURCE_DIR}/api")
add_subdirectory(api)

option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif(BUILD_EXAMPLES)