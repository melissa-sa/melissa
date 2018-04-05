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

from flowvrapp import *
from filters import *

class Simu(Composite):
    def __init__(self, cmdline, prefix, np):
      Composite.__init__(self)
      localhosts = ','.join("localhost" for i in range(np))
      putrun = FlowvrRunMPI(cmdline, hosts = localhosts, prefix = prefix, mpistack = "openmpi")

      self.processus = []
      self.beginIt = []
      self.endIt = []
      for i in range(np):
          self.processus.append(Module(prefix + "/" + str(i), run = putrun))
          self.processus[i].addPort("MelissaOut", direction = 'out')
          self.processus[i].addPort("MelissaIn", direction = 'in')
          self.beginIt.append(self.processus[i].getPort("beginIt"))
          self.endIt.append(self.processus[i].getPort("endIt"))

      self.ports["beginIt"] = list( self.beginIt)
      self.ports["endIt"] = list( self.endIt)

merge = [FilterMerge("group"+str(group_id)+"merge"+str(i)) for i in range(nb_proc_simu)]
merge_endit = FilterSignalAnd("group"+str(group_id)+"merge_endit")

group = [Simu(executable+" "+args[i], "group"+str(group_id)+"simu"+str(i), nb_proc_simu) for i in range(nb_parameters+2)]

presignal = FilterPreSignal("group"+str(group_id)+"presignal", nb = 1)
merge_endit.getPort("out").link(presignal.getPort('in'))

for j, simu in enumerate(group):
    if j == 0:
        for i, processus in enumerate(simu.processus):
            merge[i].getPort("out").link(processus.getPort("MelissaIn"))
            processus.getPort("endIt").link(merge_endit.newInputPort())
    else:
        for i, processus in enumerate(simu.processus):
            processus.getPort("MelissaOut").link(merge[i].newInputPort())
        presignal.getPort('out').link(simu.getPort("beginIt"))

app.generate_xml("group"+str(group_id))

