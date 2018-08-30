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
import traceback
from ctypes import cdll, create_string_buffer
#from options import GLOBAL_OPTIONS as stdy_opt
#from options import STUDY_OPTIONS as stdy_opt
#from options import MELISSA_STATS as ml_stats
#from options import USER_FUNCTIONS as usr_func
imp.load_source('simulation', '@CMAKE_INSTALL_PREFIX@/share/launcher/simulation.py')
from simulation import Server
from simulation import SingleSimuGroup
from simulation import Group
from simulation import SobolGroup
from simulation import Job

get_message = cdll.LoadLibrary('@CMAKE_INSTALL_PREFIX@/share/launcher/libget_message.so')

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
                    level=logging.DEBUG)
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
        try:
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
                                group.finalize()
            logging.info('closing state checker process')
        except:
            print '=== State checker thread crashed ==='
            traceback.print_exc()
            return

class Messenger(Thread):
    """
        Thread in charge of checking job status
    """
    def __init__(self):
        Thread.__init__(self)
        self.running_study = True

    def run(self):
        try:
            last_server = 0
            last_msg_to_server = 0
            while self.running_study:
#                if np.random.rand() < 0.2: print 'r' + 1
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
                        server.job_status = RUNNING
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
                if (time.time() - last_msg_to_server) > 10:
                    buff.value = 'hello server '+str(time.time())+'\000'
                    get_message.send_message(buff)
                    last_msg_to_server = time.time()
            logging.info('closing messenger thread')
        except:
            print '=== Messenger thread crashed ==='
            traceback.print_exc()
            return


class Study(object):
    """
        Study class, containing instances of the threads
    """
    def __init__(self, stdy_opt, ml_stats, usr_func):
        self.groups = list()
        self.nb_groups = stdy_opt['sampling_size']
        if ml_stats['sobol_indices']:
            self.sobol = True
        else:
            self.sobol = False
        self.messenger = Messenger()
        self.state_checker = StateChecker()
        self.stdy_opt = stdy_opt
        self.stdy_opt = stdy_opt
        self.ml_stats = ml_stats
        self.usr_func = usr_func
        nb_errors = self.check_options()
        if nb_errors > 0:
            logging.error(str(nb_errors) + ' errors in options.py.')
            exit()
        Job.set_usr_func(self.usr_func)
        Job.set_stdy_opt(self.stdy_opt)
        Job.set_ml_stats(self.ml_stats)

    def run(self):
        """
            Main study method
        """
        if not os.path.isdir(self.stdy_opt['working_directory']):
            os.mkdir(self.stdy_opt['working_directory'])
        os.chdir(self.stdy_opt['working_directory'])
        create_study(self.usr_func)
        self.create_group_list()
        # init zmq context
        get_message.init_context()
        get_message.bind_message_rcv("5555")
        logging.info('submit server')
        server.set_path(self.stdy_opt['working_directory'])
        server.create_options()
        server.launch()
        logging.debug('start messenger thread')
        self.messenger.start()
        logging.debug('wait server start')
        server.wait_start()
        server.write_node_name()
        # connect to server
        get_message.bind_message_snd("5556")
        logging.debug('start status checker thread')
        self.state_checker.start()
        for group in groups:
            self.fault_tolerance()
            while check_scheduler_load(self.usr_func) == False:
                time.sleep(1)
                self.fault_tolerance()
            logging.info('submit group '+str(group.rank))
            try:
                group.launch()
            except:
                print '=== Error while launching group ==='
                traceback.print_exc()
                self.stop()
        time.sleep(1)
        while (server.status != FINISHED
               or any([i.status != FINISHED for i in groups])):
            self.fault_tolerance()
            time.sleep(1)
        time.sleep(1)
        self.stop()

    def stop(self):
        for group in groups:
            if group.status < FINISHED and group.status > NOT_SUBMITTED:
                with group.lock:
                    group.cancel()
        if server.job_status < FINISHED:
            server.cancel()
        self.messenger.running_study = False
        self.state_checker.running_study = False
        self.messenger.join()
        self.state_checker.join()
        postprocessing(self.usr_func)
        finalize(self.usr_func)
        # finalize zmq context
        get_message.close_message()

    def check_options(self):
        """
            Validates user provided options
        """
        errors = 0
        nb_parameters = self.stdy_opt['nb_parameters']
        if not self.ml_stats['sobol_indices'] and nb_parameters < 1:
            logging.error('Error bad option: nb_parameters too small')
            errors += 1
        if self.ml_stats['sobol_indices'] and nb_parameters < 2:
            logging.error('Error bad option: nb_parameters too small')
            errors += 1
        if self.stdy_opt['sampling_size'] < 2:
            logging.error('Error bad option: sample_size not big enough')
            errors += 1

        if 'threshold_exceedance' in self.ml_stats:
            self.ml_stats['threshold_exceedances'] = self.ml_stats['threshold_exceedance']
            self.ml_stats['threshold_exceedance'] = False

        if 'threshold_value' in self.stdy_opt:
            self.stdy_opt['threshold_values'] = self.stdy_opt['threshold_value']
        elif 'threshold_value' in self.ml_stats:
            self.stdy_opt['threshold_values'] = self.ml_stats['threshold_value']
            self.ml_stats['threshold_value'] = False
        elif 'threshold_values' in self.ml_stats:
            self.stdy_opt['threshold_values'] = self.ml_stats['threshold_values']
            self.ml_stats['threshold_values'] = False

        if type(self.stdy_opt['threshold_values']) not in (list, tuple, set):
            self.stdy_opt['threshold_values'] = [self.stdy_opt['threshold_values']]

        if 'quantile' in self.ml_stats:
            self.ml_stats['quantiles'] = self.ml_stats['quantile']
            self.ml_stats['quantile'] = False

        if 'quantile_value' in self.stdy_opt:
            self.stdy_opt['quantile_values'] = self.stdy_opt['quantile_value']
        elif 'quantile_value' in self.ml_stats:
            self.stdy_opt['quantile_values'] = self.ml_stats['quantile_value']
            self.ml_stats['quantile_value'] = False
        elif 'quantile_values' in self.ml_stats:
            self.stdy_opt['quantile_values'] = self.ml_stats['quantile_values']
            self.ml_stats['quantile_values'] = False

        if type(self.stdy_opt['threshold_values']) not in (list, tuple, set):
            self.stdy_opt['threshold_values'] = [self.stdy_opt['threshold_values']]

        optional_func = ['create_study',
                         'draw_parameter_set',
                         'create_group',
                         'restart_server',
                         'restart_group',
                         'check_scheduler_load',
                         'postprocessing',
                         'finalize']
        mandatory_func = ['launch_server',
                          'launch_group',
                          'check_server_job',
                          'check_group_job',
                          'cancel_job']

        for func_name in optional_func:
            if not func_name in self.usr_func.keys():
                self.usr_func[func_name] = None
                logging.debug('Warning: no \''+func_name+'\' key in USER_FUNCTIONS')

        for func_name in mandatory_func:
            if not func_name in self.usr_func.keys():
                logging.error('Error: no \''+func_name+'\' key in USER_FUNCTIONS')
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

    def fault_tolerance(self):
        """
            Compares job status and study status, restart crashed groups
        """

        # check if threads are still alive
        if not self.messenger.isAlive():
            logging.error('Messenger thread crashed')
            self.messenger.join()
            self.messenger = Messenger()
            self.messenger.start()
        if not self.state_checker.isAlive():
            logging.error('State checker thread crashed')
            self.state_checker.join()
            self.state_checker = StateChecker()
            self.state_checker.start()

        sleep = False
        with server.lock:
            if server.status != RUNNING or server.job_status != RUNNING:
                sleep = True
        if sleep == True:
            time.sleep(3)
            sleep = False

        if ((server.status != RUNNING or server.job_status != RUNNING)
                and not all([i.status == FINISHED for i in groups])):
            print 'status: '+str(server.status)+'job_status: '+str(server.job_status)
            for group in groups:
                if group.status < FINISHED and group.status > NOT_SUBMITTED:
                    with group.lock:
                        group.cancel()
            logging.info('resubmit server job')
            time.sleep(1)
            server.restart()
            get_message.connect_message_snd(server.node_name+'\000')
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
                        if time.time() - group.start_time > self.stdy_opt['simulation_timeout']:
                            logging.info("resubmit group " + str(group.rank)
                                         + " (timeout detected by launcher)")
                            group.restart()
#    time.sleep(1)


def create_study(usr_func):
    """
        Creates study environment
    """
    if "create_study" in Job.usr_func.keys() \
    and usr_func['create_study']:
        try:
            usr_func['create_study']()
        except:
            logging.warning("Create study failed")
    else:
        pass

def draw_parameter_set(usr_func, stdy_opt):
    """
        Draws a set of parameters using user defined function
    """
    if usr_func['draw_parameter_set']:
        try:
            param_set = usr_func['draw_parameter_set']()
        except:
            logging.error("Draw parameter set failed")
            exit()
    else:
        param_set = np.zeros(stdy_opt['nb_parameters'])
        for i in range(stdy_opt['nb_parameters']):
            param_set[i] = np.random.uniform(0, 1)
    return param_set

def check_scheduler_load(usr_func):
    """
        Return False if the load is full
    """
    if "check_scheduler_load" in Job.usr_func.keys() \
    and usr_func['check_scheduler_load']:
        try:
            return usr_func['check_scheduler_load']()
        except:
            logging.warning("Check scheduler load failed")
            return True
    else:
        return True

# Study end #

def postprocessing(usr_func):
    """
        User defined postprocessing
    """
    if "postprocessing" in Job.usr_func.keys() \
    and usr_func['postprocessing']:
        try:
            usr_func['postprocessing']()
        except:
            logging.warning("Postprocessing failed")
    else:
        pass

def finalize(usr_func):
    """
        User defined final step
    """
    if "finalize" in Job.usr_func.keys() \
    and usr_func['finalize']:
        try:
            usr_func['finalize']()
        except:
            logging.warning("Finalize failed")
    else:
        pass

