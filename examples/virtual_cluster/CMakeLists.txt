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

# Just copy content
set(clusterfiles README.md
          oardocker.sh
          slurm.conf
	  Dockerfile.base
	  Dockerfile.server
	  Dockerfile.node
	  )
	
install(FILES ${clusterfiles}  DESTINATION ${EXAMPLES_DIR}/virtual_cluster)




