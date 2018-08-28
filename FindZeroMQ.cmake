# Try to find ZeroMQ library and include path.
# Once done this will define:
#
# ZEROMQ_FOUND
# ZEROMQ_INCLUDE_DIR
# ZEROMQ_LIBRARIES

if(INSTALL_ZMQ)

  include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
  ExternalProject_Add(ZeroMQ
#    URL https://github.com/zeromq/zeromq4-1/releases/download/v4.1.6/zeromq-4.1.6.tar.gz
  URL https://github.com/zeromq/libzmq/archive/v4.2.2.tar.gz
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/ZeroMQ
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}/ZeroMQ
             -DCMAKE_INSTALL_LIBDIR:PATH=${CMAKE_INSTALL_PREFIX}/ZeroMQ/lib)
  set(ZEROMQ_INCLUDE_DIRS "${CMAKE_INSTALL_PREFIX}/ZeroMQ/include")
  set(ZEROMQ_LIBRARIES "${CMAKE_INSTALL_PREFIX}/ZeroMQ/lib/${CMAKE_SHARED_LIBRARY_PREFIX}zmq${CMAKE_SHARED_LIBRARY_SUFFIX}" )
  include_directories(${ZEROMQ_INCLUDE_DIRS})
  MESSAGE(STATUS "ZeroMQ will be installed")

else(INSTALL_ZMQ)
    FIND_PATH(ZEROMQ_INCLUDE_DIR
                zmq.h
            PATH_SUFFIXES
                include
            PATHS
                ${CMAKE_INSTALL_PREFIX}/ZeroMQ
                ${ZEROMQDIR}
                $ENV{ZEROMQDIR}
                ~/Library/Frameworks
                /Library/Frameworks
                /usr/local/
                /usr/
                /sw          # Fink
                /opt/local/  # DarwinPorts
                /opt/csw/    # Blastwave
                /opt/ )

    SET(ZEROMQ_FOUND TRUE)
    SET(FIND_ZEROMQ_LIB_PATHS
            ${CMAKE_INSTALL_PREFIX}/ZeroMQ
            ${ZEROMQDIR}
            $ENV{ZEROMQDIR}
            ~/Library/Frameworks
            /Library/Frameworks
            /usr/local
            /usr
            /sw
            /opt/local
            /opt/csw
            /opt )

    FIND_LIBRARY(ZEROMQ_LIBRARY_DEBUG
                NAMES
                    zmq
                    libzmq
                PATH_SUFFIXES
                    lib64
                    lib
                PATHS
                    ${FIND_ZEROMQ_LIB_PATHS}
                    ${CMAKE_INSTALL_PREFIX}/ZeroMQ/bin/Win32/Debug/v120/dynamic
                    ${ZEROMQDIR}/bin/Win32/Debug/v120/dynamic
                    $ENV{ZEROMQDIR}/bin/Win32/Debug/v120/dynamic )

    FIND_LIBRARY(ZEROMQ_LIBRARY_RELEASE
                NAMES
                    zmq
                    libzmq
                PATH_SUFFIXES
                    lib64
                    lib
                PATHS
                    ${FIND_ZEROMQ_LIB_PATHS}
                    ${CMAKE_INSTALL_PREFIX}/ZeroMQ/bin/Win32/Release/v120/dynamic
                    ${ZEROMQDIR}/bin/Win32/Release/v120/dynamic
                    $ENV{ZEROMQDIR}/bin/Win32/Release/v120/dynamic )

    IF (ZEROMQ_LIBRARY_DEBUG OR ZEROMQ_LIBRARY_RELEASE)
        SET(ZEROMQ_FOUND TRUE)

        IF (ZEROMQ_LIBRARY_DEBUG AND ZEROMQ_LIBRARY_RELEASE)
            SET(ZEROMQ_LIBRARY debug ${ZEROMQ_LIBRARY_DEBUG} optimized ${ZEROMQ_LIBRARY_RELEASE})
        ENDIF()

        IF (ZEROMQ_LIBRARY_DEBUG AND NOT ZEROMQ_LIBRARY_RELEASE)
            SET(ZEROMQ_LIBRARY_RELEASE ${ZEROMQ_LIBRARY_DEBUG})
            SET(ZEROMQ_LIBRARY         ${ZEROMQ_LIBRARY_DEBUG})
        ENDIF()
        IF (ZEROMQ_LIBRARY_RELEASE AND NOT ZEROMQ_LIBRARY_DEBUG)
            SET(ZEROMQ_LIBRARY_DEBUG ${ZEROMQ_LIBRARY_RELEASE})
            SET(ZEROMQ_LIBRARY       ${ZEROMQ_LIBRARY_RELEASE})
        ENDIF()
    ELSE()
        SET(ZEROMQ_FOUND FALSE)
        SET(ZEROMQ_FOUND FALSE)
        SET(ZEROMQ_LIBRARY "")
        SET(FIND_ZEROMQ_MISSING "${FIND_ZEROMQ_MISSING} ZEROMQ_LIBRARY")
    ENDIF()

    MARK_AS_ADVANCED(ZEROMQ_LIBRARY
                    ZEROMQ_LIBRARY_RELEASE
                    ZEROMQ_LIBRARY_DEBUG )

    SET(ZEROMQ_LIBRARIES ${ZEROMQ_LIBRARIES} "${ZEROMQ_LIBRARY}")

    IF (NOT ZEROMQ_FOUND)
        SET(FIND_ZEROMQ_ERROR "Could NOT find ZeroMQ (missing: ${FIND_ZEROMQ_MISSING})")
        IF(ZEROMQ_FIND_REQUIRED)
            MESSAGE(FATAL_ERROR ${FIND_ZEROMQ_ERROR})
        ELSEIF(NOT ZEROMQ_FIND_QUIETLY)
            MESSAGE("${FIND_ZEROMQ_ERROR}")
        ENDIF()
    ENDIF()

    IF(ZEROMQ_FOUND)
        MESSAGE(STATUS "ZeroMQ found")
        include_directories(${ZEROMQ_INCLUDE_DIRS})
    ENDIF() 
endif(INSTALL_ZMQ)
