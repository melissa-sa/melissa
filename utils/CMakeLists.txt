
set(UTILS_C melissa_utils.c)
set(ALL_SRCS ${ALL_SRCS} ${UTILS_C})
set(UTILS_H melissa_utils.h)
set(ALL_INCS ${ALL_INCS} ${UTILS_H})

add_library(melissa_utils ${UTILS_C} ${UTILS_H})
target_compile_options(melissa_utils BEFORE PUBLIC -fPIC)
install(TARGETS melissa_utils DESTINATION bin)
install(FILES ${UTILS_H} DESTINATION include)
