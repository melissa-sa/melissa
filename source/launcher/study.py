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
#from options import GLOBAL_OPTIONS as glob_opt
#from options import STUDY_OPTIONS as stdy_opt
#from options import MELISSA_STATS as ml_stats
#from options import USER_FUNCTIONS as usr_func
imp.load_source('simulation', '@CMAKE_BINARY_DIR@/launcher/simulation.py')
from simulation import Server
from simulation import SingleSimuGroup
from simulation import Group
from simulation import SobolGroup
from simulation import Job

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
server = Server()

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
                with group.lock:
                    if (group.job_status < FINISHED and
                            group.job_status > NOT_SUBMITTED):
                        s = copy.deepcopy(group.job_status)
                        group.check_job()
                        if s <= PENDING and group.job_status == RUNNING:
                            logging.info('group ' + str(group.rank) + ' started')
                            group.start_time = time.time()
                        elif s <= RUNNING and group.job_status == FINISHED:
                            logging.info('end group ' + str(group.rank))
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
                        groups[group_id].job_status = PENDING
            elif message[0] == 'group_state':
                with groups[int(message[1])].lock:
                    groups[int(message[1])].status = int(message[2])
                logging.info('Group ' + message[1] + ' status: ' + message[2])
            elif message[0] == 'server':
                with server.lock:
                    server.status = RUNNING
                    server.node_name = message[1]
                    logging.info('Melissa Server node name: ' +
                                 str(server.node_name) + '; '+
                                 'Melissa Server job id: ' +
                                 str(server.job_id))

            if last_server > 0:
                if (time.time() - last_server) > 100:
                    logging.info('server timeout\n')
                    with server.lock:
                        server.status = TIMEOUT

        get_message.close_message()
        logging.info('closing messenger thread')


class Study(object):
    """
        Study class, containing instances of the threads
    """
    def __init__(self, glob_opt, stdy_opt, ml_stats, usr_func):
        self.groups = list()
        self.nb_groups = stdy_opt['sampling_size']
        if ml_stats['sobol_indices']:
            self.sobol = True
        else:
            self.sobol = False
        self.messenger = Messenger()
        self.state_checker = StateChecker()
        self.glob_opt = glob_opt
        self.stdy_opt = stdy_opt
        self.ml_stats = ml_stats
        self.usr_func = usr_func
        Job.set_usr_func(usr_func)
        Job.set_stdy_opt(stdy_opt)
        Job.set_ml_stats(ml_stats)

    def run(self):
        """
            Main study method
        """
#        global server
#        global groups
        if not os.path.isdir(self.glob_opt['working_directory']):
            os.mkdir(self.glob_opt['working_directory'])
        os.chdir(self.glob_opt['working_directory'])
        if self.check_options() > 0:
            return -1
        self.create_group_list()
        create_study(self.usr_func)
        logging.info('submit server')
        server.set_path(self.glob_opt['working_directory'])
        server.create_options()
        server.launch()
        self.messenger.start()
        server.wait_start()
        server.write_node_name()
#        time.sleep(2)
        self.state_checker.start()
        for group in groups:
            fault_tolerance(self.stdy_opt['simulation_timeout'])
            while check_scheduler_load(self.usr_func) == False:
                time.sleep(1)
                fault_tolerance(self.stdy_opt['simulation_timeout'])
            logging.info('submit group '+str(group.rank))
            group.launch()
            time.sleep(1)
        while (server.status != FINISHED
               or any([i.status != FINISHED for i in groups])):
            fault_tolerance(self.stdy_opt['simulation_timeout'])
            time.sleep(1)
        time.sleep(1)
        self.messenger.running_study = False
        self.state_checker.running_study = False
        self.messenger.join()
        self.state_checker.join()
        postprocessing(self.usr_func)
        finalize(self.usr_func)

    def check_options(self):
        """
            Validates user provided options
        """
        errors = 0
        nb_parameters = self.stdy_opt['nb_parameters']
        if not self.ml_stats['sobol_indices'] and nb_parameters < 1:
            logging.error('error bad option: nb_parameters too small')
            errors += 1
        if self.ml_stats['sobol_indices'] and nb_parameters < 2:
            logging.error('error bad option: nb_parameters too small')
            errors += 1
        if self.stdy_opt['sampling_size'] < 2:
            logging.error('error bad option: sample_size not big enough')
            errors += 1
        return errors

    def create_group_list(self):
        """
            Job list creation
        """
        global groups
        Group.reset()
        if self.sobol:
            while len(self.groups) < self.stdy_opt['sampling_size']:
                self.groups.append(SobolGroup(
                    draw_parameter_set(self.usr_func, self.stdy_opt),
                    draw_parameter_set(self.usr_func, self.stdy_opt)))
        else:
            while len(self.groups) < self.stdy_opt['sampling_size']:
                self.groups.append(SingleSimuGroup(draw_parameter_set(self.usr_func, self.stdy_opt)))
        for group in self.groups:
            group.create()
        groups = self.groups

def fault_tolerance(simulation_timeout):
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
        time.sleep(3)
        sleep = False

    if ((server.status != RUNNING or server.job_status != RUNNING)
            and not all([i.status == FINISHED for i in groups])):
        for group in groups:
            if group.status < FINISHED and group.status > NOT_SUBMITTED:
                with group.lock:
                    group.cancel()
        logging.info('resubmit server job')
        time.sleep(1)
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
                if group.status <= RUNNING and group.job_status == FINISHED:
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
                if group.job_status == RUNNING:
                    if time.time() - group.start_time > simulation_timeout:
                        logging.info("resubmit group " + str(group.rank)
                                     + " (timeout detected by launcher)")
                        group.restart()
                        break
#    time.sleep(1)


def create_study(usr_func):
    """
        Creates study environment
    """
    if usr_func['create_study']:
        usr_func['create_study']()
    else:
        pass

def draw_parameter_set(usr_func, stdy_opt):
    """
        Draws a set of parameters using user defined function
    """
    if usr_func['draw_parameter_set']:
        param_set = usr_func['draw_parameter_set']()
    else:
        param_set = np.zeros(stdy_opt['nb_parameters'])
        for i in range(stdy_opt['nb_parameters']):
            param_set[i] = np.random.uniform(stdy_opt['range_min_param'][i],
                                             stdy_opt['range_max_param'][i])
    return param_set


def check_scheduler_load(usr_func):
    """
        Return False if the load is full
    """
    if usr_func['check_scheduler_load']:
        return usr_func['check_scheduler_load']()
    else:
        return True

# Study end #

def postprocessing(usr_func):
    """
        User defined postprocessing
    """
    if usr_func['postprocessing']:
        usr_func['postprocessing']()
    else:
        pass

def finalize(usr_func):
    """
        User defined final step
    """
    if usr_func['finalize']:
        usr_func['finalize']()

