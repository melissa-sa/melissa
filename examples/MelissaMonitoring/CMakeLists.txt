set(files README.md
      __init__.py
	  interface.py
	  main.py
      utils.py
	  )
add_subdirectory("internal")

install(FILES ${files}
        DESTINATION ${EXAMPLES_DIR}/MelissaMonitoring
        PERMISSIONS OWNER_WRITE
                    OWNER_READ
                    OWNER_EXECUTE
                    GROUP_READ
                    GROUP_EXECUTE
                    WORLD_READ
                    WORLD_EXECUTE)