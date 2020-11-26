###################################################################
#                            Melissa                              #
#-----------------------------------------------------------------#
#   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    #
#                                                                 #
# This source is covered by the BSD 3-Clause License.             #
# Refer to the  LICENSE file for further information.             #
#                                                                 #
#-----------------------------------------------------------------#
#  Original Contributors:                                         #
#    Theophile Terraz,                                            #
#    Bruno Raffin,                                                #
#    Alejandro Ribes,                                             #
#    Bertrand Iooss,                                              #
###################################################################

cmake_minimum_required(VERSION 3.7.2)

project(Melissa VERSION 0.6.0 LANGUAGES C Fortran)

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


# set up
include(CTest)
include(GNUInstallDirs)
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


set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wextra -Wall -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-sign-compare")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-discarded-qualifiers")

IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    # Set install directory to ../install
    get_filename_component(PARENT_DIR ${CMAKE_CURRENT_BINARY_DIR} DIRECTORY)
    set(CMAKE_INSTALL_PREFIX "${PARENT_DIR}/install"
        CACHE PATH "Install path prefix, prepended onto install directories." FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)


# Find Python3
find_package(PythonInterp 3 REQUIRED)
find_python_module(numpy REQUIRED)


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
    SOVERSION ${ZeroMQ_VERSION}
)



# Find MPI
find_package(MPI REQUIRED)

if(MPI_C_FOUND AND MPI_Fortran_FOUND)
  set(EXTRA_LIBS ${EXTRA_LIBS}
				 ${MPI_C_LIBRARIES})
  include_directories( ${MPI_INCLUDE_PATH} )
  add_definitions(${MPI_Fortran_COMPILE_FLAGS})
  include_directories(${MPI_Fortran_INCLUDE_PATH})
  link_directories(${MPI_Fortran_LIBRARIES})
  set(EXTRA_LIBS ${EXTRA_LIBS}
				 ${MPI_Fortran_LIBRARIES} )
endif()


# OpenMP #

option(BUILD_WITH_OpenMP "Build with OpenMP" OFF)

if(BUILD_WITH_OpenMP)
  find_package(OpenMP)
  if(OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    message(STATUS "OpenMP Found")
    add_definitions( -DBUILD_WITH_OPENMP )
  else(OPENMP_FOUND)
    # Disable  Option if missing dependencies
    set(BUILD_WITH_OpenMP FALSE CACHE BOOL "Build with OpenMP" FORCE)
    message(STATUS "OpenMP missing. BUILD_WITH_OpenMP option turned OFF")
  endif(OPENMP_FOUND)
else(BUILD_WITH_OpenMP)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-pragmas")
endif(BUILD_WITH_OpenMP)


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
    if(DOXYGEN_FOUND)# TODO: this if is obsolete if using find_package(... REQUIRED) !
        message(STATUS "Doxygen Found")
    else(DOXYGEN_FOUND)
        set(BUILD_DOCUMENTATION FALSE CACHE BOOL "Use Doxygen to create the documentation" FORCE)
        message(STATUS "Doxygen Not Found. BUILD_DOCUMENTATION option turned OFF")
    endif(DOXYGEN_FOUND)
endif(BUILD_DOCUMENTATION)

# If we run on a Omnipath cluster, we need to check simulation deconnection.

option(CHECK_SIMU_DECONNECTION "check simulation deconnection" OFF)

#set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--export-all-symbols")

# Load local include directories before. This ensures that no old installed headers are
# loaded.
include_directories(BEFORE "${PROJECT_SOURCE_DIR}/common")
include_directories(BEFORE "${CMAKE_BINARY_DIR}/api")
include_directories(BEFORE "${PROJECT_SOURCE_DIR}/server/stats")
include_directories(BEFORE "${PROJECT_SOURCE_DIR}/server/output")
include_directories(BEFORE "${PROJECT_SOURCE_DIR}/server")


add_subdirectory(api)
add_subdirectory(bin)
add_subdirectory(common)
add_subdirectory(melissa)
add_subdirectory(server)
add_subdirectory(tests)


set (EXAMPLES_DIR ${CMAKE_INSTALL_PREFIX}/share/melissa/examples)
add_subdirectory(examples)

set (UTILITY_DIR ${CMAKE_INSTALL_PREFIX}/share/melissa/utility)
add_subdirectory(utility)

# set environement variables
configure_file(melissa_set_env.sh.in melissa_set_env.sh @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/melissa_set_env.sh DESTINATION bin)

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

set(MELISSA_CMAKECONFIG_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/share/cmake/${PROJECT_NAME})

configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${MELISSA_CMAKECONFIG_INSTALL_DIR})
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
                                 VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
                                 COMPATIBILITY AnyNewerVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
              DESTINATION ${MELISSA_CMAKECONFIG_INSTALL_DIR})


# Melissa4py #

option(MELISSA4PY_INSTALL "Build with Melissa4py (Python server)" OFF)

if(MELISSA4PY_INSTALL)
  message("Installing melissa4py")
  add_subdirectory(utility/melissa4py)
endif(MELISSA4PY_INSTALL)
