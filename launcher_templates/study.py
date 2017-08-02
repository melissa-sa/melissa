
"""
    Study main module jobs module
"""

from threading import Thread
import os
import time
import copy
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
output_str = 'month/day/hour/min/sec\n'

def output(out=''):
    global output_str
    date = time.localtime
    print out
    output_str += '['+time.asctime(time.localtime())+'] '+out+'\n'

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
                    old_stat = copy.deepcopy(simu.job_status)
                    with simu.lock:
                        simu.check_job()
                        if old_stat <= PENDING and simu.job_status == RUNNING:
                            output('start simulation '+str(simu.rank))
                            simu.start_time = time.time()
                        elif old_stat <= RUNNING and simu.job_status == FINISHED:
                            output('end simulation '+str(simu.rank))
        output('closing state checker process')

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
                    output('end study')
            elif message[0] == 'timeout':
                if message[1] != '-1':
                    simu_id = int(message[1])
                    output('restarting simulation '+str(simu_id)+' (server detected timeout)')
                    with simulations[simu_id].lock:
                        simulations[simu_id].restart()
                        simulations[simu_id].job_status = PENDING
            elif message[0] == 'simu_state':
                with simulations[int(message[1])].lock:
                    simulations[int(message[1])].status = int(message[2])
            elif message[0] == 'server':
                with server.lock:
                    server.status = RUNNING
                    server.node_name = message[1]
                    output('Melissa Server node name: ' +
                           str(server.node_name) + '; '+
                           'Melissa Server job id: ' +
                           str(server.job_id))

            if last_server > 0:
                if (time.time() - last_server) > serv_opt['timeout']:
                    output('server timeout\n')
                    with server.lock:
                        server.status = TIMEOUT

        get_message.close_message()
        output('closing messenger thread')


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
        output('submit server')
        server.launch()
        self.messenger.start()
        server.wait_start()
        output('start server')
        time.sleep(2)
        self.state_checker.start()
        for simu in simulations:
            fault_tolerance()
            check_scheduler_load()
            output('submit simulation '+str(simu.rank))
            simu.launch()
            simu.job_status = PENDING
        while (server.status != FINISHED
               or any([i.status != FINISHED for i in simulations])):
            fault_tolerance()
            time.sleep(1)
        time.sleep(2)
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
    sleep = False
    with server.lock:
        if server.status != RUNNING or server.job_status != RUNNING:
            sleep = True
    if sleep == True:
        time.sleep(5)
        sleep = False

    if ((server.status != RUNNING or server.job_status != RUNNING)
            and not all([i.status == FINISHED for i in simulations])):
        for simu in [x for x in simulations if (x.status < FINISHED and
                                                x.job_status > NOT_SUBMITTED)]:
            with simu.lock:
                simu.cancel()
#                simu.job_status = NOT_SUBMITTED
        with server.lock:
            output('resubmit server job')
            server.restart()
            server.wait_start()
            output('server start')
            time.sleep(1)
        for simu in [x for x in simulations if (x.status < FINISHED and
                                                x.job_status > NOT_SUBMITTED)]:
            with simu.lock:
                output('resubmit simulation '+str(simu.rank)+' (server crash)')
                simu.restart()
                simu.job_status = PENDING
        time.sleep(2)

    for simu in [x for x in simulations if x.job_status > NOT_SUBMITTED]:
        with simu.lock:
            if simu.status <= RUNNING and simu.job_status == FINISHED:
                sleep = True
        if sleep == True:
            time.sleep(10)
            sleep = False
            with simu.lock:
                if simu.status <= RUNNING:
                    output("resubmit simulation "+str(simu.rank)+" (simulation crashed)")
                    simu.restart()
                    simu.job_status = PENDING
        with simu.lock:
            if simu.status == WAITING and simu.job_status == RUNNING:
                if time.time() - simu.start_time > simu_opt['timeout']:
                    output("resubmit simulation "+str(simu.rank)+" (launcher detected timeout)")
#                    output("elapsed time : "+str(time.time() - simu.start_time))
                    simu.restart()
                    simu.job_status = PENDING
    time.sleep(1)

def check_options():
    """
        Validates user provided options
    """
    errors = 0
    nb_parameters = stdy_opt['nb_parameters']
    if not os.path.isdir(glob_opt['home_path']):
        output('error bad option: home_path: no such directory')
        errors += 1
    if not ml_stats['sobol_indices'] and nb_parameters < 1:
        output('error bad option: nb_parameters too small')
        errors += 1
    if ml_stats['sobol_indices'] and nb_parameters < 2:
        output('error bad option: nb_parameters too small')
        errors += 1
    if len(stdy_opt['range_min_param']) != nb_parameters:
        output('error bad option: wrong dimension for range_min_param')
        errors += 1
    if len(stdy_opt['range_max_param']) != nb_parameters:
        output('error bad option: wrong dimension for range_max_param')
        errors += 1
    if stdy_opt['sampling_size'] < 2:
        output('error bad option: sample_size not big enough')
        errors += 1
    if not os.path.isdir(serv_opt['path']):
        output('error bad option: server_path: no such directory')
        errors += 1
    elif not os.path.isfile(serv_opt['path']+'/server'):
        output('error bad option: server_path: wrong directory')
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
    global output_str
    if usr_func['finalize']:
        usr_func['finalize']()
    file = open(file_name, 'w')
    file.write(output_str)
    file.close()

