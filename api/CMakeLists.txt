###################################################################
#                            Melissa                              #
#-----------------------------------------------------------------#
#   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    #
#                                                                 #
# This source is covered by the BSD 3-Clause License.             #
# Refer to the  LICENCE file for further information.             #
#                                                                 #
#-----------------------------------------------------------------#
#  Original Contributors:                                         #
#    Theophile Terraz,                                            #
#    Bruno Raffin,                                                #
#    Alejandro Ribes,                                             #
#    Bertrand Iooss,                                              #
###################################################################


add_library(melissa_api ${ALL_API} $<TARGET_OBJECTS:melissa_utils>)
#target_link_libraries(melissa_api melissa_utils)
target_compile_options(melissa_api BEFORE PUBLIC -fPIC)
install(TARGETS melissa_api DESTINATION lib)
if(BUILD_WITH_MPI)
    install(FILES melissa_api.h DESTINATION include)
endif(BUILD_WITH_MPI)
install(FILES melissa_api_no_mpi.h DESTINATION include)

configure_file(melissa_api.f90 melissa_api.f90 COPYONLY)
install(FILES melissa_api.f90 DESTINATION include)
