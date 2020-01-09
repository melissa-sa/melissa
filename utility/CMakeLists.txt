
add_subdirectory(MelissaMonitoring)

install(FILES Melissa-monitoring-notebook.ipynb
        DESTINATION ${UTILITY_DIR}
        PERMISSIONS OWNER_WRITE
                    OWNER_READ
                    OWNER_EXECUTE
                    GROUP_READ
                    GROUP_EXECUTE
                    WORLD_READ
                    WORLD_EXECUTE)
