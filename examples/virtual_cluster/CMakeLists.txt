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
	  slurm_start.sh
	  custom_setup.base.sh
	  custom_setup.server.sh
	  custom_setup.node.sh
	  )
	
install(FILES ${clusterfiles}  DESTINATION ${EXAMPLES_DIR}/virtual_cluster)




