# Copyright 2020 Inria (https://www.inria.fr)

#
# let pkg-config find the package because it also gives version information
#
find_package(PkgConfig REQUIRED QUIET)

if(ZeroMQ_ROOT)
    set(_ZeroMQ_ORIGINAL_PKG_CONFIG_PATH $ENV{PKG_CONFIG_PATH})
    set(ENV{PKG_CONFIG_PATH} ${ZeroMQ_ROOT}/lib/pkgconfig)
endif()

# usually pkg-config omits standard search paths so PACKAGE_LIBARIES and
# PACKAGE_INCLUDE_DIRS may be empty even if the package was found
# we want to rely exclusively on pkg-config and ask it to print all paths
if(DEFINED ENV{PKG_CONFIG_ALLOW_SYSTEM_CFLAGS})
    set(_ZeroMQ_ORIGINAL_PKG_CONFIG_ALLOW_SYSTEM_CFLAGS
        $ENV{PKG_CONFIG_ALLOW_SYSTEM_CFLAGS})
endif()

if(DEFINED ENV{PKG_CONFIG_ALLOW_SYSTEM_LIBS})
    set(_ZeroMQ_ORIGINAL_PKG_CONFIG_ALLOW_SYSTEM_LIBS
        $ENV{PKG_CONFIG_ALLOW_SYSTEM_LIBS})
endif()

set(ENV{PKG_CONFIG_ALLOW_SYSTEM_CFLAGS} 1)
set(ENV{PKG_CONFIG_ALLOW_SYSTEM_LIBS} 1)


pkg_check_modules(_PC_ZeroMQ QUIET libzmq)


# restore environment variables
if(ZeroMQ_ROOT)
    set(ENV{PKG_CONFIG_PATH} ${_ZeroMQ_ORIGINAL_PKG_CONFIG_PATH})
endif()

if(DEFINED _ZeroMQ_ORIGINAL_PKG_CONFIG_ALLOW_SYSTEM_CFLAGS)
    set(ENV{PKG_CONFIG_ALLOW_SYSTEM_CFLAGS}
        ${_ZeroMQ_ORIGINAL_PKG_CONFIG_ALLOW_SYSTEM_CFLAGS})
else()
    unset(ENV{PKG_CONFIG_ALLOW_SYSTEM_CFLAGS})
endif()

if(DEFINED _ZeroMQ_ORIGINAL_PKG_CONFIG_ALLOW_SYSTEM_LIBS)
    set(ENV{PKG_CONFIG_ALLOW_SYSTEM_LIBS}
        ${_ZeroMQ_ORIGINAL_PKG_CONFIG_ALLOW_SYSTEM_LIBS})
else()
    unset(ENV{PKG_CONFIG_ALLOW_SYSTEM_LIBS})
endif()


#
# set variables for caller
# suppress default paths to ensure the software found by pkg-config is used
#
find_path(ZeroMQ_INCLUDE_DIR
          NAMES zmq.h
          PATHS ${_PC_ZeroMQ_INCLUDE_DIRS}
          NO_DEFAULT_PATH)

find_library(ZeroMQ_LIBRARY
             NAMES zmq
             PATHS ${_PC_ZeroMQ_LIBRARY_DIRS}
             NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZeroMQ
                                  FOUND_VAR ZeroMQ_FOUND
                                  REQUIRED_VARS ZeroMQ_INCLUDE_DIR ZeroMQ_LIBRARY
                                  VERSION_VAR _PC_ZeroMQ_VERSION)

if(ZeroMQ_FOUND)
    set(ZeroMQ_VERSION ${_PC_ZeroMQ_VERSION} CACHE INTERNAL "ZeroMQ version")
endif()

mark_as_advanced(ZeroMQ_INCLUDE_DIR ZeroMQ_LIBRARY)
