
import numpy
from threading import RLock

class Job:
    def __init__(self):
        self.job_status = 0
        self.nb_restarts = 0
        self.job_id = 0
        self.command_line_options = ''
        self.nproc = 1
        self.executable = ''
        self.mpi_options = ''
        self.lock = RLock()

#    def launch(self, options):
#        pass

#    def wait(self, options):
#        pass

#    def kill(self, options):
#        pass

#    def restart(self, options):
#        pass

#    def check(self, options):
#        pass

class Simulation(Job):
    nb_simu = 0
    def __init__(self, param_set, executable, sobol_id=0):
        Job.__init__(self)
        self.status = 'waiting'
        self.rank = Simulation.nb_simu
        Simulation.nb_simu += 1
        self.param_set = numpy.copy(param_set)
        self.sobol_id = sobol_id
        self.executable = executable

class SobolCoupledGroup(Job):
    nb_groups = 0
    def __init__(self, param_set_a, param_set_b, executable):
        Job.__init__(self)
        self.status = 0
        self.rank = SobolCoupledGroup.nb_groups
        SobolCoupledGroup.nb_groups += 1
        self.param_set = list()
        self.param_set.append(numpy.copy(param_set_a))
        self.param_set.append(numpy.copy(param_set_b))
        for i in range(len(param_set_b)):
            self.param_set.append(numpy.copy(param_set_a))
            self.param_set[i+2][i] = param_set_b[i]
        self.executable = executable

class SobolMultiJobsGroup():
    nb_groups = 0
    def __init__(self, param_set_a, param_set_b, executable):
        self.rank = SobolMultiJobsGroup.nb_groups
        SobolMultiJobsGroup.nb_groups += 1
        self.simulations = list()
        self.param_set_a = numpy.copy(param_set_a)
        self.simulations.append(Simulation(param_set=param_set_a,
                                           executable=executable,
                                           sobol_id=0))
        self.param_set_b = numpy.copy(param_set_b)
        self.simulations.append(Simulation(param_set=param_set_b,
                                           executable=executable,
                                           sobol_id=1))
        for i in range(len(param_set_a)):
            temp_param_set = numpy.copy(param_set_a)
            temp_param_set[i] = numpy.copy(param_set_b[i])
            self.simulations.append(Simulation(param_set=temp_param_set,
                                               executable=executable,
                                               sobol_id=i+2))

class Server(Job):
    def __init__(self, work_dir='./', mpi_opt='', nproc=1):
        Job.__init__(self)
        self.status = 0
        self.node_name = ''
        self.first_job_id = ''
        self.directory = work_dir
        self.mpi_options = mpi_opt
        self.nproc = nproc
        self.executable = 'server'
