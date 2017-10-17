
"""
    Simulations and server jobs module
"""

import numpy
import os
import time
import subprocess
import logging
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
        """
            Job constructor
        """
        self.job_status = NOT_SUBMITTED
        self.job_id = 0
        self.cmd_opt = ''
        self.mpi_options = ''
        self.start_time = 0.0

    def cancel(self):
        """
            Cancels a job (mandatory)
        """
        if  usr_func['cancel_job']:
            return usr_func['cancel_job'](self)
        else:
            logging.error('Error: no \'cancel_job\' function provided')
            exit()

class Group(Job):
    """
        Group class
    """
    nb_groups = -1
    def __init__(self):
        """
            Group constructor
        """
        Job.__init__(self)
        self.nb_restarts = 0
        self.status = NOT_SUBMITTED
        self.lock = RLock()
        self.rank = Group.nb_groups
        Group.nb_groups += 1

    def create(self):
        """
            Creates a group environment
        """
        if usr_func['create_group']:
            usr_func['create_group'](self)
        logging.warning('Warning: no \'create_group\''
                        +' function provided')

    def launch(self):
        """
            Launches the group (mandatory)
        """
        if usr_func['launch_group']:
            usr_func['launch_group'](self)
        else:
            logging.error('Error: no \'launch_group\' function provided')
            exit()
        self.job_status = PENDING
        self.status = WAITING

    def restart(self):
        """
            Kills and restarts the group (mandatory)
        """
        if usr_func['restart_group']:
            usr_func['restart_group'](self)
        else:
            logging.warning('warning: no \'restart_group\''
                            +' function provided,'
                            +' using \'launch_group\' instead')
            self.launch()
        self.start_time = 0.0
        self.job_status = PENDING

    def check_job(self):
        """
            Checks the group job status (mandatory)
        """
        if usr_func['check_group_job']:
            usr_func['check_group_job'](self)
        else:
            logging.error('Error: no \'check_group_job\''
                          +' function provided')
            exit()

class SingleSimuGroup(Group):
    """
    Single simulation group class
    """
    def __init__(self, param_set):
        """
            SingleSimuGroup constructor
        """
        Group.__init__(self)
        self.rank = Group.nb_groups
        self.param_set = numpy.copy(param_set)

    def restart(self):
        """
            Ends and restarts the simulation (mandatory)
        """
        self.cancel()
        self.nb_restarts += 1
        if self.nb_restarts > 4:
            logging.warning('Simulation ' + self.rank +
                            'crashed 5 times, drawing new parameter set')
            logging.info('old parameter set: ' + str(self.param_set))
            self.param_set = usr_func['draw_parameter_set']()
            logging.info('new parameter set: ' + str(self.param_set))
            self.nb_restarts = 0
        self.status = WAITING


class SobolGroup(Group):
    """
        Sobol coupled group class
    """
    def __init__(self, param_set_a, param_set_b):
        """
            SobolCoupledGroup constructor
        """
        Group.__init__(self)
        self.rank = Group.nb_groups
        self.param_set = list()
        self.param_set.append(numpy.copy(param_set_a))
        self.param_set.append(numpy.copy(param_set_b))
        for i in range(len(param_set_a)):
            self.param_set.append(numpy.copy(self.param_set[0]))
            self.param_set[i+2][i] = numpy.copy(self.param_set[1][i])
    def restart(self):
        """
            Ends and restarts the Sobol group (mandatory)
        """
        self.cancel()
        self.nb_restarts += 1
        if self.nb_restarts > 4:
            logging.warning('Group ' +
                            self.rank +
                            'crashed 5 times, drawing new parameter sets')
            logging.debug('old parameter set A: ' + str(self.param_set[0]))
            logging.debug('old parameter set B: ' + str(self.param_set[1]))
            self.param_set[0] = usr_func['draw_parameter_set']()
            self.param_set[1] = usr_func['draw_parameter_set']()
            logging.info('new parameter set A: ' + str(self.param_set[0]))
            logging.info('new parameter set B: ' + str(self.param_set[1]))
            for i in range(len(self.param_set[0])):
                self.param_set[i+2] = numpy.copy(self.param_set[0])
                self.param_set[i+2][i] = numpy.copy(self.param_set[1][i])
            self.nb_restarts = 0
        self.simulation.restart()
        self.status = WAITING

class Server(Job):
    """
        Server class
    """
    def __init__(self, work_dir='./'):
        """
            Server constructor
        """
        Job.__init__(self)
        self.status = WAITING
        self.node_name = ''
        self.first_job_id = ''
        self.directory = work_dir
        self.create_options()
        self.lock = RLock()
        self.path = '@CMAKE_BINARY_DIR@/server'

    def write_node_name(self):
        fichier=open("server_name.txt", "w")
        fichier.write(self.node_name)
        fichier.close()
        os.system("chmod 744 server_name.txt")

    def create_options(self):
        """
            Melissa Server command line options
        """
        op_str = ':'.join([x for x in ml_stats if ml_stats[x]])
        if op_str == '':
            logging.error('error bad option: no operation given')
            return
        field_str = ':'.join([x for x in stdy_opt['field_names']])
        if field_str == '':
            logging.error('error bad option: no field name given')
            return
        self.cmd_opt = ' '.join(('-o', op_str,
                                 '-p', str(stdy_opt['nb_parameters']),
                                 '-s', str(stdy_opt['sampling_size']),
                                 '-t', str(stdy_opt['nb_time_steps']),
                                 '-e', str(stdy_opt['threshold_value']),
                                 '-f', field_str,
                                 '-n', str(gethostname())))

    def launch(self):
        """
            Launches server job
        """
        os.chdir(self.directory)
        logging.info('launch server')
        if usr_func['launch_server']:
            usr_func['launch_server'](self)
        else:
            logging.error('Error: no \'launch_server\' function provided')
            exit()
        self.first_job_id = self.job_id
        self.job_status = PENDING

    def wait_start(self):
        """
            Waits for the server to start
        """
        with self.lock:
            status = self.status
        while status < RUNNING:
            time.sleep(1)
            with self.lock:
                status = self.status
        if status > RUNNING:
            logging.warning('Server crashed')
        else:
            logging.info('Server running')
        self.start_time = time.time()

    def restart(self):
        """
            Restarts the server
        """
        self.cmd_opt += ' -r ' + self.directory
        os.chdir(self.directory)
        if usr_func['restart_server']:
            usr_func['restart_server'](self)
        else:
            logging.warning('Warning: no \'restart_server\' function provided'
                            +' using launch_server instead')
            exit()
        with self.lock:
            self.status = WAITING
            self.job_status = PENDING
        self.start_time = 0.0
        self.wait_start()

    def check_job(self):
        """
            Checks server job status
        """
        if usr_func['check_server_job']:
            usr_func['check_server_job'](self)
        else:
            logging.error('Error: no \'check_server_job\' function provided')
            exit()
