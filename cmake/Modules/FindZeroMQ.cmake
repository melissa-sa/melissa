# Copyright 2020 Inria (https://www.inria.fr)

# let pkg-config find the package because it also gives version information
find_package(PkgConfig REQUIRED QUIET)

if(ZeroMQ_ROOT)
    set(_ZeroMQ_ORIGINAL_PKG_CONFIG_PATH ENV{PKG_CONFIG_PATH})
    set(ENV{PKG_CONFIG_PATH} ${ZeroMQ_ROOT}/lib/pkgconfig)
endif()

pkg_check_modules(_PC_ZeroMQ QUIET libzmq)

if(ZeroMQ_ROOT)
    set(ENV{PKG_CONFIG_PATH} _ZeroMQ_ORIGINAL_PKG_CONFIG_PATH)
endif()


# check pkg-config results
if(NOT _PC_ZeroMQ_FOUND)
    message(FATAL_ERROR "Could NOT find ZeroMQ")
endif()

set(_ZeroMQ_MINIMUM_REQUIRED_VERSION 3.0)

if(_PC_ZeroMQ_VERSION VERSION_LESS _ZeroMQ_MINIMUM_REQUIRED_VERSION)
    message(FATAL_ERROR "ZeroMQ ${_ZeroMQ_MINIMUM_REQUIRED_VERSION} or newer required")
endif()


# set variables for caller
# suppress default paths to ensure the software found by pkg-config is used
find_path(ZeroMQ_INCLUDE_DIR
          NAMES zmq.h
          PATHS ${_PC_ZeroMQ_INCLUDE_DIRS}
          NO_DEFAULT_PATH)

find_library(ZeroMQ_LIBRARY
             NAMES zmq
             PATHS ${_PC_ZeroMQ_LIBRARY_DIRS}
             NO_DEFAULT_PATH)

set(ZeroMQ_VERSION ${_PC_ZeroMQ_VERSION} CACHE INTERNAL "ZeroMQ version")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZeroMQ
                                  FOUND_VAR ZeroMQ_FOUND
                                  REQUIRED_VARS ZeroMQ_INCLUDE_DIR ZeroMQ_LIBRARY
                                  VERSION_VAR _PC_ZeroMQ_VERSION)

mark_as_advanced(ZeroMQ_INCLUDE_DIR ZeroMQ_LIBRARY)
