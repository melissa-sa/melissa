
"""
    Simulations and server jobs module
"""

import numpy
import os
import time
import subprocess
from threading import RLock
from socket import gethostname
from options import USER_FUNCTIONS as usr_func
from options import SERVER_OPTIONS as serv_opt
from options import STUDY_OPTIONS as stdy_opt
from options import MELISSA_STATS as ml_stats

# Jobs and executions status

NOT_SUBMITTED = -1
PENDING = 0
WAITING = 0
RUNNING = 1
FINISHED = 2
TIMEOUT = 4

class Job(object):
    """
        Job class
    """
    def __init__(self):
        self.job_status = NOT_SUBMITTED
        self.nb_restarts = 0
        self.job_id = 0
        self.cmd_opt = ''
        self.nproc = 1
        self.executable = ''
        self.mpi_options = ''
        self.lock = RLock()
        self.start_time = 0.0

    def cancel(self):
        """
            Cancels a job (mandatory)
        """
        if  usr_func['cancel_job']:
            return usr_func['cancel_job'](self)
        else:
            print 'Error: no \'cancel_job\' function provided'
            exit()

class Simulation(Job):
    """
        Simulation class
    """
    nb_simu = 0
    def __init__(self, param_set, executable, sobol_id=0):
        Job.__init__(self)
        self.status = WAITING
        self.rank = Simulation.nb_simu
        Simulation.nb_simu += 1
        self.param_set = numpy.copy(param_set)
        self.sobol_id = sobol_id
        self.executable = executable

    def create(self):
        """
            Creates a simulation environment
        """
        if usr_func['create_simulation']:
            usr_func['create_simulation'](self)
        else:
            pass

    def launch(self):
        """
            Launches the simulation (mandatory)
        """
        if usr_func['launch_simulation']:
            usr_func['launch_simulation'](self)
        else:
            print 'Error: no \'launch_simulation\' function provided'
            exit()

    def restart(self):
        """
            Kills and restarts the simulation (mandatory)
        """
        if usr_func['restart_simulation']:
            usr_func['restart_simulation'](self)
        else:
            print 'Error: no \'restart_simulation\' function provided'
        self.nb_restarts += 1

    def check_job(self):
        """
            Checks the simulation job status (mandatory)
        """
        if usr_func['check_simulation_job']:
            usr_func['check_simulation_job'](self)
        else:
            print 'Error: no \'check_simulation_job\' function provided'
            exit()


class SobolCoupledGroup(Job):
    """
        Sobol coupled group class
    """
    nb_groups = 0
    def __init__(self, param_set_a, param_set_b, executable):
        Job.__init__(self)
        self.status = WAITING
        self.rank = SobolCoupledGroup.nb_groups
        SobolCoupledGroup.nb_groups += 1
        self.simulations = list()
        self.param_set = list()
        self.param_set.append(numpy.copy(param_set_a))
        self.simulations.append(Simulation(param_set=param_set_a,
                                           executable=None,
                                           sobol_id=0))
        self.simulations[-1].rank = self.rank
        self.param_set.append(numpy.copy(param_set_b))
        self.simulations.append(Simulation(param_set=param_set_b,
                                           executable=None,
                                           sobol_id=1))
        self.simulations[-1].rank = self.rank
        for i in range(len(param_set_b)):
            self.param_set.append(numpy.copy(param_set_a))
            self.param_set[i+2][i] = param_set_b[i]
        self.executable = executable
        for i in range(len(param_set_a)):
            temp_param_set = numpy.copy(param_set_a)
            temp_param_set[i] = numpy.copy(param_set_b[i])
            self.simulations.append(Simulation(param_set=temp_param_set,
                                               executable=None,
                                               sobol_id=i+2))
            self.simulations[-1].rank = self.rank

    def create(self):
        """
            Creates a Sobol group environment
        """
        if usr_func['create_group']:
            usr_func['create_group'](self)
        else:
            pass

    def launch(self):
        """
            Launches the Sobol group (mandatory)
        """
        if usr_func['launch_group']:
            return usr_func['launch_group'](self)
        else:
            print 'Error: no \'launch_group\' function provided'
            exit()

    def restart(self):
        """
            Ends and restarts the Sobol group (mandatory)
        """
        if usr_func['restart_group']:
            usr_func['restart_group'](self)
        else:
            pass

    def check_job(self):
        """
            Checks the Sobol group job status (mandatory)
        """
        if usr_func['check_simulation_job']:
            usr_func['check_simulation_job'](self)
        else:
            print 'Error: no \'check_simulation_job\' function provided'
            exit()

class SobolMultiJobsGroup(object):
    """
        Sobol multi job group class (not coupled)
    """
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
        """
            Creates a Sobol group environment
        """
        if usr_func['create_group']:
            usr_func['create_group'](self)
        for simu in self.simulations:
            simu.create()

    def launch(self):
        """
            Launches the simulations in the group
        """
        for simu in self.simulations:
            simu.launch()

class Server(Job):
    """
        Server class
    """
    def __init__(self, work_dir='./', mpi_opt='', nproc=1):
        """
            Server constructor
        """
        Job.__init__(self)
        self.status = WAITING
        self.node_name = ''
        self.first_job_id = ''
        self.directory = work_dir
        self.mpi_options = mpi_opt
        self.nproc = nproc
        self.executable = 'server'
        self.create_options()

    def create_options(self):
        """
            Melissa Server command line options
        """
        op_str = ':'.join([x for x in ml_stats if ml_stats[x]])
        if op_str == '':
            print 'error bad option: no operation given'
            return
        self.cmd_opt = ' '.join(('-o', op_str,
                                 '-p', str(stdy_opt['nb_parameters']),
                                 '-s', str(stdy_opt['sampling_size']),
                                 '-t', str(stdy_opt['nb_time_steps']),
                                 '-e', str(stdy_opt['threshold_value']),
                                 '-n', str(gethostname())))

    def launch(self):
        """
            Launches server job
        """
        os.chdir(self.directory)
        if usr_func['launch_server']:
            usr_func['launch_server'](self)
        else:
            print 'launch server : '+'mpirun '+self.mpi_options + \
                    ' -n '+str(self.nproc) + \
                    ' ' + serv_opt['path'] + \
                    '/server ' + self.cmd_opt + ' &'
            self.job_id = subprocess.Popen(
                ('mpirun '+self.mpi_options +
                 ' -n '+str(self.nproc) +
                 ' ' + serv_opt['path'] +
                 '/server ' +
                 self.cmd_opt +
                 ' &').split()).pid
            self.first_job_id = self.job_id
            self.job_status = PENDING

    def wait_start(self):
        """
            Waits for the server to start
        """
        with self.lock:
            status = self.status
        while status < 1:
            time.sleep(1)
            with self.lock:
                status = self.status
        if status > RUNNING:
            print 'Server crashed'
        else:
            print 'Server running'

    def restart(self):
        """
            Restarts the server and the running and pennding simulations
        """
        self.cmd_opt += ' -r ' + self.directory
        os.chdir(self.directory)
        if usr_func['restart_server']:
            usr_func['restart_server'](self)
        else:
            self.job_id = subprocess.Popen(
                ('mpirun ' + self.mpi_options +
                 ' -n ' + str(self.nproc) + ' ' +
                 serv_opt['path'] + '/server ' +
                 self.cmd_opt +
                 ' &').split()).pid
            with self.lock:
                self.status = WAITING
                self.job_status = PENDING
        self.wait_start()

    def check_job(self):
        """
            Checks server job status
        """
        if usr_func['check_server_job']:
            usr_func['check_server_job'](self)
        else:
            print 'Error: no \'check_server_job\' function provided'
            exit()
