find_package(MPI)

foreach(_language IN ITEMS C CXX Fortran)
    if(NOT CMAKE_${_language}_COMPILER_WORKS)
        continue()
    endif()

    if(MelissaMPI_FIND_REQUIRED AND NOT MPI_${_language}_FOUND)
        message(SEND_FATAL "MPI_${_language} not found")
    endif()

    # fix compilation on CentOS 8 with CMake 3.11
    separate_arguments(MPI_${_language}_COMPILE_FLAGS)

    # remove leading, trailing whitespace when CMake 3.7 + MPICH is used
    if(MPI_${_language}_LINK_FLAGS)
        string(STRIP ${MPI_${_language}_LINK_FLAGS} MPI_${_language}_LINK_FLAGS)
    endif()

    # use an interface instead of an imported library because MPI_X_LIBRARIES
    # may contain several libraries, e.g., on Debian 9 with OpenMPI 2 for
    # Fortran.
    set(_target_name mpi_${_language})
    add_library(${_target_name} INTERFACE)
    target_include_directories(
        ${_target_name} INTERFACE ${MPI_${_language}_INCLUDE_PATH}
    )
    target_compile_options(
        ${_target_name} INTERFACE ${MPI_${_language}_COMPILE_FLAGS}
    )
    target_link_libraries(
        ${_target_name}
        INTERFACE ${MPI_${_language}_LINK_FLAGS} ${MPI_${_language}_LIBRARIES}
    )
endforeach()
