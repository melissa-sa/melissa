find_program(PIP "pip")

option(MELISSA4PY_DEVELOP "Install melissa4py on develop mode" ON)
option(MELISSA4PY_REQUIREMENTS "Install requirements for melissa4py" ON)

if(PIP)
    # Install requirements
    if (MELISSA4PY_REQUIREMENTS)
        set(REQUIREMENTS "${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt")
        install(CODE "execute_process(COMMAND ${PIP} install -r ${REQUIREMENTS})")
    endif(MELISSA4PY_REQUIREMENTS)
    # Install modules
    if (MELISSA4PY_DEVELOP)
        install(CODE "execute_process(COMMAND ${PIP} install -e ${CMAKE_CURRENT_SOURCE_DIR})")
    else()
        install(CODE "execute_process(COMMAND ${PIP} install ${CMAKE_CURRENT_SOURCE_DIR})")
    endif(MELISSA4PY_DEVELOP)
else()
    message("Python package manager pip was not found.")
endif(PIP)