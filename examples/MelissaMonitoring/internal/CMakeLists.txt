set(files __init__.py
          plots.py
	)

install(FILES ${files}
        DESTINATION ${EXAMPLES_DIR}/MelissaMonitoring/internal
        PERMISSIONS OWNER_WRITE
                    OWNER_READ
                    OWNER_EXECUTE
                    GROUP_READ
                    GROUP_EXECUTE
                    WORLD_READ
                    WORLD_EXECUTE)