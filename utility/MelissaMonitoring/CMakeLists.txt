set(files
  Melissa-monitoring-notebook.ipynb
  README.md
	  )
add_subdirectory("MelissaMonitoring")

install(FILES ${files}
  DESTINATION ${UTILITY_DIR}/MelissaMonitoring
        PERMISSIONS OWNER_WRITE
                    OWNER_READ
                    OWNER_EXECUTE
                    GROUP_READ
                    GROUP_EXECUTE
                    WORLD_READ
                    WORLD_EXECUTE)
