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


if(BUILD_WITH_MPI)
  configure_file(run_mpi_zmq_clients.sh run_clients.sh COPYONLY)
else(BUILD_WITH_MPI)
  configure_file(run_zmq_clients.sh run_clients.sh COPYONLY)
endif(BUILD_WITH_MPI)

add_executable(client client.c)
target_link_libraries(client ${EXTRA_LIBS} melissa_api)
