
add_library(melissa_stats ${STATS_C} ${STATS_H} $<TARGET_OBJECTS:melissa_utils>)
target_link_libraries(melissa_stats m)
target_compile_options(melissa_stats BEFORE PUBLIC -fPIC -Wall)
install(TARGETS melissa_stats DESTINATION bin)
install(FILES ${STATS_H} DESTINATION include)
