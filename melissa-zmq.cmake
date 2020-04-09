# ZeroMQ #

option(INSTALL_ZMQ OFF)
if(INSTALL_ZMQ)
  include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
  ExternalProject_Add(ZeroMQ
#  URL https://github.com/zeromq/zeromq4-1/releases/download/v4.1.6/zeromq-4.1.6.tar.gz
#  URL https://github.com/zeromq/libzmq/archive/v4.2.2.tar.gz
  URL https://github.com/zeromq/libzmq/releases/download/v4.3.1/zeromq-4.3.1.tar.gz
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ZeroMQ
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
             -DCMAKE_INSTALL_LIBDIR:PATH=${CMAKE_INSTALL_PREFIX}/lib
             -DZMQ_BUILD_TESTS=OFF)
  set(ZeroMQ_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")
  set(ZeroMQ_LIBRARY "${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}zmq${CMAKE_SHARED_LIBRARY_SUFFIX}" )
#  include_directories(${ZeroMQ_INCLUDE_DIR})
  MESSAGE(STATUS "ZeroMQ will be installed")
else(INSTALL_ZMQ)
  #find_package(ZeroMQ NO_MODULE REQUIRED)

  ## credit to https://stackoverflow.com/questions/41251474/how-to-import-zeromq-libraries-in-cmake
  ## load in pkg-config support
  find_package(PkgConfig)
  ## use pkg-config to get hints for 0mq locations
  pkg_check_modules(PC_ZeroMQ QUIET zmq)

  ## use the hint from above to find where 'zmq.hpp' is located
  find_path(ZeroMQ_INCLUDE_DIR
          NAMES zmq.h
          PATHS ${PC_ZeroMQ_INCLUDE_DIRS} "${ZeroMQ_ROOT}/include"
          )

  ## use the hint from about to find the location of libzmq
  find_library(ZeroMQ_LIBRARY
          NAMES zmq
          PATHS ${PC_ZeroMQ_LIBRARY_DIRS} "${ZeroMQ_ROOT}/lib"
          )
  # TODO: ensure that zmq is the correct version!
endif(INSTALL_ZMQ)
