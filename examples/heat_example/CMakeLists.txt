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

set (SCHED_VERSIONS "local" "OAR" "cluster" "Slurm" "python_server")  ## TODO: remove Slurm

foreach(sched_version ${SCHED_VERSIONS})
    add_subdirectory(${sched_version})
endforeach(sched_version in ${SCHED_VERSIONS})

add_subdirectory(src)
