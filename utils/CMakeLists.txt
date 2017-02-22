
add_library(melissa_utils ${UTILS_C} ${UTILS_H})
target_compile_options(melissa_utils BEFORE PUBLIC -fPIC)
install(TARGETS melissa_utils DESTINATION bin)
install(FILES ${UTILS_H} DESTINATION include)
