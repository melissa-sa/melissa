
"""
    Study main module jobs module
"""

from threading import Thread
import os
import time
import imp
import numpy as np
from ctypes import cdll, create_string_buffer
from options import GLOBAL_OPTIONS as glob_opt
from options import SERVER_OPTIONS as serv_opt
from options import STUDY_OPTIONS as stdy_opt
from options import SIMULATIONS_OPTIONS as simu_opt
from options import MELISSA_STATS as ml_stats
from options import USER_FUNCTIONS as usr_func
imp.load_source('simulation', '@CMAKE_BINARY_DIR@/launcher_templates/simulation.py')
from simulation import Simulation
from simulation import Server
from simulation import SobolCoupledGroup
from simulation import SobolMultiJobsGroup

get_message = cdll.LoadLibrary('@CMAKE_BINARY_DIR@/utils/libget_message.so')

# Jobs and executions status
from simulation import NOT_SUBMITTED
from simulation import WAITING
from simulation import PENDING
from simulation import RUNNING
from simulation import FINISHED
from simulation import TIMEOUT

simulations = list()
server = Server(glob_opt['working_directory'],
                serv_opt['mpi_options'],
                serv_opt['nb_proc'])
output = ''

class StateChecker(Thread):
    """
        Thread in charge of recieving Server messages
    """
    def __init__(self):
        Thread.__init__(self)
        self.running_study = True

    def run(self):
        global simulations
        global server
        while self.running_study:
            time.sleep(1)
            with server.lock:
                server.check_job()
            for simu in simulations:
                if (simu.job_status < FINISHED and
                        simu.job_status > NOT_SUBMITTED):
                    old_stat = simu.job_status
                    with simu.lock:
                        simu.check_job()
                        if old_stat == PENDING and simu.job_status == RUNNING:
                            simu.start_time = time.time()
        print 'closing state checker process'

class Messenger(Thread):
    """
        Thread in charge of checking job status
    """
    def __init__(self):
        Thread.__init__(self)
        self.running_study = True

    def run(self):
        global simulations
        global server
        global output
        last_server = 0
        get_message.init_message()
        while self.running_study:
            buff = create_string_buffer('\000' * 256)
            get_message.wait_message(buff)
            if buff.value != 'nothing':
                print 'message: '+buff.value
            last_server = time.time()
            message = buff.value.split()
            if message[0] == 'stop':
                with server.lock:
                    server.status = FINISHED # finished
            elif message[0] == 'timeout':
                if message[1] != '-1':
                    simu_id = int(message[1])
                    output += 'Simulation ' + simu_id + 'timeout\n'
                    with simulations[simu_id].lock:
                        simulations[simu_id].restart()
            elif message[0] == 'simu_state':
                with simulations[int(message[1])].lock:
                    simulations[int(message[1])].status = int(message[2])
            elif message[0] == 'server':
                with server.lock:
                    server.status = RUNNING
                    server.node_name = message[1]
                    print 'server node name: '+message[1]
                    print 'server job id: '+str(server.job_id)
                    output += 'Melissa Server node name: ' + \
                              str(server.node_name) + '\n' +\
                              'Melissa Server job id: ' + \
                              str(server.job_id) + '\n'

            if last_server > 0:
                if (time.time() - last_server) > serv_opt['timeout']:
                    output += 'Server ' + simu_id + 'timeout\n'
                    with server.lock:
                        server.status = TIMEOUT

        get_message.close_message()
        print 'closing messenger thread'


class Study(object):
    """
        Study class, containing instances of the threads
    """
    def __init__(self):
        self.groups = list()
        self.nb_groups = stdy_opt['sampling_size']
        if ml_stats['sobol_indices']:
            self.sobol = True
        else:
            self.sobol = False
        self.messenger = Messenger()
        self.state_checker = StateChecker()

    def run(self):
        """
            Main study method
        """
        global server
        global simulations
        os.chdir(glob_opt['working_directory'])
        if check_options() > 0:
            return -1
        self.create_job_lists()
        create_study()
        server.launch()
        self.messenger.start()
        server.wait_start()
        self.state_checker.start()
        for simu in simulations:
            fault_tolerance()
            check_scheduler_load()
            simu.launch()
        while (not server.status == FINISHED
               and not all([i.status == FINISHED for i in simulations])):
            fault_tolerance()
            time.sleep(1)
        self.messenger.running_study = False
        self.state_checker.running_study = False
        self.messenger.join()
        self.state_checker.join()
        postprocessing()
        finalize()

    def create_job_lists(self):
        """
            Job list creation
        """
        global simulations
        if self.sobol and simu_opt['coupling']:
            while len(self.groups) < stdy_opt['sampling_size']:
                self.groups.append(SobolCoupledGroup(
                    draw_parameter_set(),
                    draw_parameter_set(),
                    simu_opt['executable']))
                self.groups[-1].create()
            simulations = self.groups
        elif self.sobol:
            while len(self.groups) < stdy_opt['sampling_size']:
                self.groups.append(SobolMultiJobsGroup(
                    draw_parameter_set(),
                    draw_parameter_set(),
                    simu_opt['executable']))
                self.groups[-1].create()
                for simu in self.groups[-1].simulations:
                    simulations.append(simu)
                    simu.create()
        else:
            while len(simulations) < stdy_opt['sampling_size']:
                simulations.append(Simulation(
                    draw_parameter_set(),
                    simu_opt['executable'],
                    0))
            simulations[-1].create()

def fault_tolerance():
    """
        Compares job status and study status, restart crashed jobs
    """
    global simulations
    global server
    global output
    sleep = False
    with server.lock:
        if server.status != RUNNING:
            sleep = True
    if sleep == True:
        time.sleep(5)
        sleep = False

    if (server.status != RUNNING
            and not all([i.status == FINISHED for i in simulations])):
        with server.lock:
            server.restart()
        for simu in [x for x in simulations if (x.status < FINISHED and
                                                x.job_status > NOT_SUBMITTED)]:
            with simu.lock:
                simu.restart()

    for simu in [x for x in simulations if x.job_status > NOT_SUBMITTED]:
        with simu.lock:
            if simu.status <= RUNNING and simu.job_status == FINISHED:
                sleep = True
        if sleep == True:
            time.sleep(10)
            sleep = False
            with simu.lock:
                if simu.status <= RUNNING:
                   simu.restart()
        with simu.lock:
            if simu.status == WAITING and simu.job_status == RUNNING:
                if time.time() - simu.start_time > simu_opt['timeout']:
                    print "elapsed time : "+str(time.time() - simu.start_time)
                    simu.restart()

def check_options():
    """
        Validates user provided options
    """
    errors = 0
    nb_parameters = stdy_opt['nb_parameters']
    if not os.path.isdir(glob_opt['home_path']):
        print 'error bad option: home_path: no such directory'
        errors += 1
    if not ml_stats['sobol_indices'] and nb_parameters < 1:
        print 'error bad option: nb_parameters too small'
        errors += 1
    if ml_stats['sobol_indices'] and nb_parameters < 2:
        print 'error bad option: nb_parameters too small'
        errors += 1
    if len(stdy_opt['range_min_param']) != nb_parameters:
        print 'error bad option: wrong dimension for range_min_param'
        errors += 1
    if len(stdy_opt['range_max_param']) != nb_parameters:
        print 'error bad option: wrong dimension for range_max_param'
        errors += 1
    if stdy_opt['sampling_size'] < 2:
        print 'error bad option: sample_size not big enough'
        errors += 1
    if not os.path.isdir(serv_opt['path']):
        print 'error bad option: server_path: no such directory'
        errors += 1
    elif not os.path.isfile(serv_opt['path']+'/server'):
        print 'error bad option: server_path: wrong directory'
        errors += 1
    return errors


def create_study():
    """
        Creates study environment
    """
    if usr_func['create_study']:
        usr_func['create_study']()
    else:
        pass

def draw_parameter_set():
    """
        Draws a set of parameters using user defined function
    """
    param_set = np.zeros(stdy_opt['nb_parameters'])
    draw_param = np.random.uniform
    if usr_func['draw_parameter']:
        draw_param = usr_func['draw_parameter']
    for i in range(stdy_opt['nb_parameters']):
        param_set[i] = draw_param(stdy_opt['range_min_param'][i],
                                  stdy_opt['range_max_param'][i])
    return param_set


def check_scheduler_load():
    """
        Waits if the load is full
    """
    if usr_func['check_scheduler_load']:
        usr_func['check_scheduler_load']()
    else:
        pass

# Study end #

def postprocessing():
    """
        User defined postprocessing
    """
    if usr_func['postprocessing']:
        usr_func['postprocessing']()
    else:
        pass

def finalize(file_name='melissa_launcher.out'):
    """
        User defined final step
    """
    global output
    if usr_func['finalize']:
        usr_func['finalize']()
    file = open(file_name, 'w')
    file.write(output)
    file.close()

