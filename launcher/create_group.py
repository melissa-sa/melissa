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


"""
    Group creation for Sobol' indices
"""
import sys

BUILD_WITH_FLOWVR = '@BUILD_WITH_FLOWVR@'.upper()
if BUILD_WITH_FLOWVR == "ON":
    from flowvrapp import *
    from filters import *

class Simu(Composite):
    def __init__(self, cmdline, prefix, np):
      Composite.__init__(self)
      putrun = FlowvrRunMPI(cmdline, hosts = "", prefix = prefix, mpistack = "openmpi")

      self.processus = []
      for i in range(np):
          self.processus.append(Module(prefix + "/" + str(i), run = putrun))
          self.processus[i].addPort("output", direction = 'out')
          self.processus[i].addPort("input", direction = 'in')

def create_flowvr_group(executable, group_id, nb_proc_simu, nb_parameters):
    merge = [FilterMerge("merge"+str(i)) for i in range(nb_proc_simu)]

    group = [Simu(executable, "simu"+str(i), nb_proc_simu) for i in range(nb_parameters+2)]

    for j, simu in enumerate(group):
        if j == 0:
            for i, processus in enumerate(simu.processus):
                merge[i].getPort("out").link(processus.getPort("input"))
        else:
            for i, processus in enumerate(simu.processus):
                processus.getPort("output").link(merge[i].newInputPort())

    app.generate_xml("group"+str(group_id))

if __name__ == '__main__':
    if len(sys.argv) <= 4:
        print "Not enough parameters"
    else:
        create_flowvr_group(sys.argv[1], *[int(sys.argv[i]) for i in range(2,5)])
