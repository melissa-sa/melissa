
 configure_file(options.py options.py @ONLY)
 configure_file(heat_base.c heat_base.c COPYONLY)
 configure_file(heat_base.f90 heat_base.f90 COPYONLY)

 if(BUILD_WITH_MPI)
   add_executable(heatf_base heat_base.f90)
   target_link_libraries(heatf_base ${EXAMPLES_LIBS} heat_utils)

   add_executable(heatc_base heat_base.c)
   target_link_libraries(heatc_base ${EXAMPLES_LIBS} heat_utils)
 endif(BUILD_WITH_MPI)

 add_library(heat_utils_no_mpi heat_utils_no_mpi.f90)
 target_compile_options(heat_utils_no_mpi BEFORE PUBLIC -fPIC)
 target_link_libraries(heat_utils_no_mpi ${EXAMPLES_LIBS})

 add_test(TestHeatcBase ./heatc_base 5)
 add_test(TestHeatfBase ./heatf_base 5)
 add_test(TestHeatc python ../../launcher/melissa_launcher.py .)
