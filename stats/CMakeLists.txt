
set(STATS_C covariance.c
            variance.c
            mean.c
            min_max.c
            sobol.c
            threshold.c
            ../utils/melissa_utils.c)
set(ALL_SRCS ${ALL_SRCS} ${STATS_C})
set(STATS_H covariance.h
            variance.h
            mean.h
            min_max.h
            sobol.h
            threshold.h
            ../utils/melissa_utils.h)
set(ALL_INCS ${ALL_INCS} ${STATS_H})
add_library(melissa_stats ${STATS_C} ${STATS_H})
target_link_libraries(melissa_stats m)
target_compile_options(melissa_stats BEFORE PUBLIC -fPIC)
install(TARGETS melissa_stats DESTINATION bin)
install(FILES ${STATS_H} DESTINATION include)
