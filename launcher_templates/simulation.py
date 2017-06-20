
import numpy
import os
import time
import subprocess
from threading import RLock
from socket import gethostname
from options import *

class Job:
    def __init__(self):
        self.job_status = 0
        self.nb_restarts = 0
        self.job_id = 0
        self.cmd_opt = ''
        self.nproc = 1
        self.executable = ''
        self.mpi_options = ''
        self.lock = RLock()

#    def launch(self):
#        pass

#    def wait(self):
#        pass

    def cancel(self):
        if  USER_FUNCTIONS['cancel_job']:
            return USER_FUNCTIONS['cancel_job'](self)
        else:
            pass

#    def restart(self):
#        pass

#    def check(self):
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

    def create(self):
        if USER_FUNCTIONS['create_simulation']:
            USER_FUNCTIONS['create_simulation'](self)
        else:
            pass

    def launch(self):
        if USER_FUNCTIONS['launch_simulation']:
            USER_FUNCTIONS['launch_simulation'](self)
        else:
            pass

    def restart(self):
        if USER_FUNCTIONS['restart_simulation']:
            USER_FUNCTIONS['restart_simulation'](self)
        else:
            pass

    def check_job(self):
        if USER_FUNCTIONS['check_simulation_job']:
            USER_FUNCTIONS['check_simulation_job'](self)
        else:
            pass


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

    def create(self):
        if USER_FUNCTIONS['create_group']:
            USER_FUNCTIONS['create_group'](self)
        else:
            pass

    def launch(self):
        if USER_FUNCTIONS['launch_group']:
            return USER_FUNCTIONS['launch_group'](self)
        else:
            pass

    def restart(self):
        if USER_FUNCTIONS['restart_simulation']:
            USER_FUNCTIONS['restart_simulation'](self)
        else:
            pass

    def check_job(self):
        if USER_FUNCTIONS['check_simulation_job']:
            USER_FUNCTIONS['check_simulation_job'](self)
        else:
            pass

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

    def create(self):
        for simu in self.simulations:
            simu.create()

    def launch(self):
        for simu in self.simulations:
            simu.launch()

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
        self.create_options()

    def create_options(self):
        op_str = ':'.join([x for x in MELISSA_STATS if MELISSA_STATS[x]])
        if op_str == '':
            print 'error bad option: no operation given'
            return
        self.cmd_opt = ' '.join(('-o', op_str,
                                 '-p', str(STUDY_OPTIONS['nb_parameters']),
                                 '-s', str(STUDY_OPTIONS['sampling_size']),
                                 '-t', str(STUDY_OPTIONS['nb_time_steps']),
                                 '-e', str(STUDY_OPTIONS['threshold_value']),
                                 '-n', str(gethostname())))

    def launch(self):
        os.chdir(self.directory)
        if USER_FUNCTIONS['launch_server']:
           USER_FUNCTIONS['launch_server'](self)
        else:
            print 'launch server : '+'mpirun '+self.mpi_options + \
                    ' -n '+str(self.nproc) + \
                    ' ' + SERVER_OPTIONS['path'] + \
                    '/server ' + \
                    self.cmd_opt + ' &'
            self.job_id = subprocess.Popen(
                ('mpirun '+self.mpi_options +
                 ' -n '+str(self.nproc) +
                 ' ' + SERVER_OPTIONS['path'] +
                 '/server ' +
                 self.cmd_opt +
                 ' &').split()).pid
            self.first_job_id = self.job_id

    def wait_start(self):
        with self.lock:
            status = self.status
        while status < 1:
            time.sleep(1)
            with self.lock:
                status = self.status
        if status > 1:
            print 'Server crashed'
        else:
            print 'Server running'

    def restart(self):
        self.cmd_opt += ' -r ' + self.directory
        os.chdir(self.directory)
        if USER_FUNCTIONS['restart_server']:
            USER_FUNCTIONS['restart_server'](self)
        else:
            self.job_id = subprocess.Popen(
                ('mpirun ' + self.mpi_options +
                 ' -n ' + str(self.nproc) + ' ' +
                 SERVER_OPTIONS['path'] + '/server ' +
                 self.cmd_opt +
                 ' &').split()).pid
            with self.lock:
                self.status = 0
                self.job_status = 0
        self.wait_start()

    def check_job(self):
        if USER_FUNCTIONS['check_server_job']:
            USER_FUNCTIONS['check_server_job'](self)
        else:
            pass
