
"""
    Study main module
"""

from threading import Thread
import os
import time
import copy
import imp
import numpy as np
import logging
from ctypes import cdll, create_string_buffer
from options import GLOBAL_OPTIONS as glob_opt
from options import SERVER_OPTIONS as serv_opt
from options import STUDY_OPTIONS as stdy_opt
from options import SIMULATIONS_OPTIONS as simu_opt
from options import MELISSA_STATS as ml_stats
from options import USER_FUNCTIONS as usr_func
imp.load_source('simulation', '@CMAKE_BINARY_DIR@/launcher_templates/simulation.py')
from simulation import Server
from simulation import SingleSimuGroup
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

logging.basicConfig(format='%(asctime)s %(message)s',
                    datefmt='%m/%d/%Y %I:%M:%S %p',
                    filename='melissa_launcher.log',
                    filemode='w',
                    level=logging.INFO)
groups = list()
server = Server(glob_opt['working_directory'],
                serv_opt['mpi_options'],
                serv_opt['nb_proc'])

class StateChecker(Thread):
    """
        Thread in charge of recieving Server messages
    """
    def __init__(self):
        Thread.__init__(self)
        self.running_study = True

    def run(self):
#        global groups
#        global server
        while self.running_study:
            time.sleep(1)
            with server.lock:
                server.check_job()
            for group in groups:
                for simu in group.simulations:
                    if (simu.job_status < FINISHED and
                            simu.job_status > NOT_SUBMITTED):
                        stat = copy.deepcopy(simu.job_status)
                        with simu.lock:
                            simu.check_job()
                            if stat <= PENDING and simu.job_status == RUNNING:
                                logging.info('start simulation ' +
                                             str(simu.rank))
                                simu.start_time = time.time()
                            elif stat <= RUNNING and simu.job_status == FINISHED:
                                logging.info('end simulation ' + str(simu.rank)
                                             + ' group ' + str(simu.group.rank))
        logging.info('closing state checker process')

class Messenger(Thread):
    """
        Thread in charge of checking job status
    """
    def __init__(self):
        Thread.__init__(self)
        self.running_study = True

    def run(self):
#        global groups
#        global server
        last_server = 0
        get_message.init_message()
        while self.running_study:
            buff = create_string_buffer('\000' * 256)
            get_message.wait_message(buff)
            if buff.value != 'nothing':
                logging.debug('message: '+buff.value)
            last_server = time.time()
            message = buff.value.split()
            if message[0] == 'stop':
                with server.lock:
                    server.status = FINISHED # finished
                    logging.info('end study')
            elif message[0] == 'timeout':
                if message[1] != '-1':
                    group_id = int(message[1])
                    logging.info('restarting group ' + str(group_id)
                                 + ' (timeout detected by server)')
                    with groups[group_id].lock:
                        groups[group_id].restart()
                        for simu in groups[group_id].simulations:
                            simu.job_status = PENDING
            elif message[0] == 'group_state':
                with groups[int(message[1])].lock:
                    groups[int(message[1])].status = int(message[2])
            elif message[0] == 'server':
                with server.lock:
                    server.status = RUNNING
                    server.node_name = message[1]
                    logging.info('Melissa Server node name: ' +
                                 str(server.node_name) + '; '+
                                 'Melissa Server job id: ' +
                                 str(server.job_id))

            if last_server > 0:
                if (time.time() - last_server) > serv_opt['timeout']:
                    logging.info('server timeout\n')
                    with server.lock:
                        server.status = TIMEOUT

        get_message.close_message()
        logging.info('closing messenger thread')


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
#        global server
#        global groups
        os.chdir(glob_opt['working_directory'])
        if check_options() > 0:
            return -1
        print "plop"
        self.create_group_list()
        print "plop"
        create_study()
        logging.info('submit server')
        server.launch()
        self.messenger.start()
        server.wait_start()
        logging.info('start server')
        time.sleep(2)
        self.state_checker.start()
        for group in groups:
            fault_tolerance()
            check_scheduler_load()
            logging.info('submit group '+str(group.rank))
            group.launch()
        while (server.status != FINISHED
               or any([i.status != FINISHED for i in groups])):
            fault_tolerance()
            time.sleep(1)
        time.sleep(2)
        self.messenger.running_study = False
        self.state_checker.running_study = False
        self.messenger.join()
        self.state_checker.join()
        postprocessing()
        finalize()

    def create_group_list(self):
        """
            Job list creation
        """
        global groups
        if self.sobol and simu_opt['coupling']:
            while len(self.groups) < stdy_opt['sampling_size']:
                self.groups.append(SobolCoupledGroup(
                    draw_parameter_set(),
                    draw_parameter_set(),
                    simu_opt['executable']))
        elif self.sobol:
            while len(self.groups) < stdy_opt['sampling_size']:
                self.groups.append(SobolMultiJobsGroup(
                    draw_parameter_set(),
                    draw_parameter_set(),
                    simu_opt['executable']))
        else:
            while len(self.groups) < stdy_opt['sampling_size']:
                self.groups.append(SingleSimuGroup(draw_parameter_set(),
                                                   simu_opt['executable']))
        self.groups[-1].create()
        groups = self.groups

def fault_tolerance():
    """
        Compares job status and study status, restart crashed groups
    """
#    global groups
#    global server
    sleep = False
    with server.lock:
        if server.status != RUNNING or server.job_status != RUNNING:
            sleep = True
    if sleep == True:
        time.sleep(5)
        sleep = False

    if ((server.status != RUNNING or server.job_status != RUNNING)
            and not all([i.status == FINISHED for i in groups])):
        for group in groups:
            if group.status < FINISHED and group.status > NOT_SUBMITTED:
                with group.lock:
                    group.cancel()
        with server.lock:
            logging.info('resubmit server job')
            server.restart()
            server.wait_start()
            logging.info('server start')
            time.sleep(1)
        for group in groups:
            if group.status < FINISHED and group.status > NOT_SUBMITTED:
                with group.lock:
                    logging.info('resubmit group ' + str(group.rank)
                                 + ' (server crash)')
                    group.restart()
        time.sleep(2)

    for group in groups:
        if group.status > NOT_SUBMITTED and group.status < FINISHED:
            with group.lock:
                for simu in group.simulations:
                    if group.status <= RUNNING and simu.job_status == FINISHED:
                        sleep = True
                        break
        if sleep == True:
            time.sleep(10)
            sleep = False
            with group.lock:
                if group.status <= RUNNING:
                    logging.info("resubmit group " + str(group.rank)
                                 + " (simulation crashed)")
                    group.restart()
        with group.lock:
            if group.status == WAITING:
                for simu in group.simulations:
                    if simu.job_status == RUNNING:
                        if time.time() - simu.start_time > simu_opt['timeout']:
                            logging.info("resubmit group " + str(simu.rank)
                                         + " (timeout detected by launcher)")
                            group.restart()
    time.sleep(1)

def check_options():
    """
        Validates user provided options
    """
    errors = 0
    nb_parameters = stdy_opt['nb_parameters']
    if not os.path.isdir(glob_opt['home_path']):
        logging.error('error bad option: home_path: no such directory')
        errors += 1
    if not ml_stats['sobol_indices'] and nb_parameters < 1:
        logging.error('error bad option: nb_parameters too small')
        errors += 1
    if ml_stats['sobol_indices'] and nb_parameters < 2:
        logging.error('error bad option: nb_parameters too small')
        errors += 1
    if len(stdy_opt['range_min_param']) != nb_parameters:
        logging.error('error bad option: wrong dimension for range_min_param')
        errors += 1
    if len(stdy_opt['range_max_param']) != nb_parameters:
        logging.error('error bad option: wrong dimension for range_max_param')
        errors += 1
    if stdy_opt['sampling_size'] < 2:
        logging.error('error bad option: sample_size not big enough')
        errors += 1
    if not os.path.isdir(serv_opt['path']):
        logging.error('error bad option: server_path: no such directory')
        errors += 1
    elif not os.path.isfile(serv_opt['path']+'/server'):
        logging.error('error bad option: server_path: wrong directory')
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

def finalize():
    """
        User defined final step
    """
    if usr_func['finalize']:
        usr_func['finalize']()

