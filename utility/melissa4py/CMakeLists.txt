find_program(PIP "pip3")

option(MELISSA4PY_DEVELOP_MODE "Install melissa4py on develop mode" ON)
option(MELISSA4PY_USER_MODE "Install package on user mode" ON)
option(MELISSA4PY_REQUIREMENTS "Install requirements for melissa4py" OFF)

if(PIP)
    # Install requirements
    if (MELISSA4PY_REQUIREMENTS)
        set(REQUIREMENTS "${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt")
        install(CODE "execute_process(COMMAND ${PIP} install -r ${REQUIREMENTS})")
    endif(MELISSA4PY_REQUIREMENTS)
    # TODO: use a list or array of options
    set(PIP_USER_FLAG "")
    if (MELISSA4PY_USER_MODE)
        set(PIP_USER_FLAG "--user")
    endif(MELISSA4PY_USER_MODE)
    set(PIP_DEVELOP_FLAG "")
    if (MELISSA4PY_DEVELOP_MODE)
        set(PIP_DEVELOP_FLAG "-e")
    endif(MELISSA4PY_DEVELOP_MODE)
    # Install modules
    install(CODE "execute_process(COMMAND ${PIP} install ${PIP_USER_FLAG} ${PIP_DEVELOP_FLAG} ${CMAKE_CURRENT_SOURCE_DIR})")
else()
    message("Python package manager pip was not found.")
endif(PIP)