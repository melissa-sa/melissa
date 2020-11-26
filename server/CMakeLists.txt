###################################################################
#                            Melissa                              #
#-----------------------------------------------------------------#
#   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    #
#                                                                 #
# This source is covered by the BSD 3-Clause License.             #
# Refer to the  LICENSE file for further information.             #
#                                                                 #
#-----------------------------------------------------------------#
#  Original Contributors:                                         #
#    Theophile Terraz,                                            #
#    Bruno Raffin,                                                #
#    Alejandro Ribes,                                             #
#    Bertrand Iooss,                                              #
###################################################################

add_subdirectory(stats)
add_subdirectory(output)

add_executable(melissa_server
    compute_stats.c
    fault_tolerance.c
    melissa_fields.c
    melissa_data.c
    melissa_io.c
    melissa_options.c
    server.c
    server_helper.c
    $<TARGET_OBJECTS:melissa_utils>
)

target_link_libraries(melissa_server ${EXTRA_LIBS}
                                     ${OPENTURNS_LIBRARIES}
                                     melissa_stats
                                     melissa_output
                                     melissa_messages
                                     dl)
INSTALL(TARGETS melissa_server RUNTIME DESTINATION bin RENAME melissa_server)
