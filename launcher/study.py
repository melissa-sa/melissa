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
from sys import exit
import os
import time
import copy
import numpy as np
import logging
import traceback
from ctypes import cdll, create_string_buffer, c_char_p, c_wchar_p, c_int, c_double, POINTER
c_int_p = POINTER(c_int)
c_double_p = POINTER(c_double)
from launcher.simulation import Server
#from simulation import SingleSimuGroup
from launcher.simulation import MultiSimuGroup
from launcher.simulation import Group
from launcher.simulation import SobolGroup
from launcher.simulation import Job
from launcher.simulation import melissa_install_prefix

melissa_comm4py = cdll.LoadLibrary(melissa_install_prefix + '/lib/libmelissa_comm4py.so')
melissa_comm4py.bind_message_rcv.argtypes = [c_char_p]
melissa_comm4py.bind_message_resp.argtypes = [c_char_p]
melissa_comm4py.bind_message_snd.argtypes = [c_char_p]
melissa_comm4py.send_resp_message.argtypes = [c_char_p]
#melissa_comm4py.wait_message.argtypes = [c_char_p]
melissa_comm4py.send_job.argtypes = [c_int, c_char_p, c_int, c_double_p]


# Jobs and executions status
from launcher.simulation import NOT_SUBMITTED
from launcher.simulation import WAITING
from launcher.simulation import PENDING
from launcher.simulation import RUNNING
from launcher.simulation import FINISHED
from launcher.simulation import TIMEOUT

logging.basicConfig(format='%(asctime)s %(message)s',
                    datefmt='%m/%d/%Y %I:%M:%S %p',
                    filename='melissa_launcher.log',
                    filemode='w',
                    level=logging.DEBUG)

class StateChecker(Thread):
    """
        Thread in charge of recieving Server messages
    """
    def __init__(self, server = [], groups = []):
        Thread.__init__(self)
        self.running_study = True
        self.server = server
        self.groups = groups

    def run(self):
        try:
            while self.running_study:
                time.sleep(1)
                with self.server[0].lock:
                    self.server[0].check_job()
                for group in self.groups:
                    with group.lock:
                        if (group.job_status < FINISHED and
                                group.job_status > NOT_SUBMITTED):
                            s = copy.deepcopy(group.job_status)
                            group.check_job()
                            if s <= PENDING and group.job_status == RUNNING:
                                logging.info('group ' + str(group.group_id) + ' job started')
                                group.start_time = time.time()
                            elif s <= RUNNING and group.job_status == FINISHED:
                                logging.info('end job group ' + str(group.group_id))
                                group.finalize()
            logging.info('closing state checker process')
        except:
            print('=== State checker thread crashed ===')
            logging.error(traceback.print_exc())
            return

class Messenger(Thread):
    """
        Thread in charge of checking job status
    """
    def __init__(self, batch_size = 1, server = [], groups = []):
        Thread.__init__(self)
        self.running_study = True
        self.batch_size = batch_size
        self.server = server  # refactor:  sometimes is a server, sometimes a server object! thats confusing!
        self.groups = groups
        self.confidence_interval = dict()

    def run(self):
        try:
            last_server = 0
            last_msg_to_server = 0
            buff = create_string_buffer(melissa_comm4py.melissa_get_message_len())
            while self.running_study:
                melissa_comm4py.wait_message(buff)
                last_server = time.time()
                message = buff.value.decode().split()
                if message[0] != 'nothing':
                    logging.debug('message: '+buff.value.decode())
                if message[0] == 'stop':
                    with self.server[0].lock:
                        self.server[0].status = FINISHED  # finished
                        self.server[0].want_stop = True
                        logging.info('Received stop message from server')
                    for group in self.groups:
                        with group.lock:
                            if group.job_status < FINISHED and group.job_status > NOT_SUBMITTED:
                                group.cancel()
                            group.status = FINISHED  # do not restart groups!
                        logging.info('end study')
                elif message[0] == 'timeout':
                    if message[1] != '-1':
                        group_id = int(message[1])//self.batch_size
                        with self.groups[group_id].lock:
                            self.groups[group_id].status = TIMEOUT
                elif message[0] == 'group_state':
                    group_id = int(message[1])//self.batch_size
                    simu_id = int(message[1])%self.batch_size
                    with self.groups[group_id].lock:
                        if self.groups[group_id].job_type == 3:
                            if self.groups[group_id].status < 2:
                                self.groups[group_id].status = int(message[2])
                            #print("group " +str(group_id)+ " simu "+str(simu_id))
                            self.groups[group_id].simu_status[simu_id] = int(message[2])
                            if all(simu_status == 2 for simu_status in self.groups[group_id].simu_status):
                                self.groups[group_id].status = 2
                                #print('Group ' + str(group_id) + ' status: ' + str(groups[group_id].status))
                        else:
                            self.groups[group_id].status = int(message[2])
                        if self.groups[group_id].status == 2:
                            self.groups[group_id].finalize()
                    if message[2] == 2:
                        logging.info('Group ' + message[1] + ' finished')
                    if message[2] == 1:
                        logging.info('Group ' + message[1] + ' connected to server')
                elif message[0] == 'server':
                    with self.server[0].lock:
                        rank = int(message[1])
                        while (len(self.server[0].node_name) <= rank):
                            self.server[0].node_name.append([''])
                        self.server[0].node_name[rank] = message[2]
                        logging.info('Melissa Server node '+str(rank)+' name: ' +
                                     str(self.server[0].node_name[rank]) + '; '+
                                     'Melissa Server job id: ' +
                                     str(self.server[0].job_id))
                        self.server[0].status = RUNNING
                        self.server[0].job_status = RUNNING
                elif message[0] == 'interval':
                    self.confidence_interval[message[1]] = float(message[3])
                    logging.info(message[1] + ' confidence interval: ' + message[3])

                if last_server > 0:
                    if self.server[0].status != RUNNING:
                        last_server = 0
                    else:
                        if (time.time() - last_server) > 100:
                            logging.info('server timeout\n')
                            with self.server[0].lock:
                                self.server[0].status = TIMEOUT
                if (time.time() - last_msg_to_server) > 10 and self.server[0].status == RUNNING:
                    melissa_comm4py.send_hello()
                    last_msg_to_server = time.time()
            logging.info('closing messenger thread')
        except:
            print('=== Messenger thread crashed ===')
            logging.error(traceback.print_exc())
            return

class Responder(Thread):
    """
        Thread in charge of responding to server requests
    """
    def __init__(self, batch_size = 1, server = [], groups = []):
        Thread.__init__(self)
        self.running_study = True
        self.batch_size = batch_size
        self.server = server
        self.groups = groups

    def run(self):
        try:
            while self.running_study:
                buff = create_string_buffer(melissa_comm4py.melissa_get_message_len())
                melissa_comm4py.get_resp_message(buff)
                    #with self.server[0].lock:
                        #self.server[0].status = RUNNING
                    #print('message: '+buff.value.decode())
                message = buff.value.decode().split()
                if message[0] != 'nothing':
                    logging.debug('message: '+buff.value.decode())
                if message[0] == 'simu_info':
                    group = int(message[1])//self.batch_size
                    simu = int(message[1])%self.batch_size
                    with self.groups[group].lock:
                        if self.groups[group].job_type == 4:
                            params = self.groups[group].param_set[0]
                            melissa_comm4py.send_job(self.groups[group].group_id,
                                                     str(self.groups[group].job_id).encode(),
                                                     len(self.groups[group].param_set[0]),
                                                     (c_double * len(params))(*params))
                        elif self.groups[group].job_type == 3:
                            params = self.groups[group].param_set[simu]
                            melissa_comm4py.send_job(self.groups[group].simu_id[simu],
                                                     str(self.groups[group].job_id).encode(),
                                                     len(self.groups[group].param_set[0]),
                                                     (c_double * len(params))(*params))
                        elif self.groups[group].job_type == 2:
                            params = self.groups[group].param_set
                            melissa_comm4py.send_job(self.groups[group].group_id,
                                                     str(self.groups[group].job_id).encode(),
                                                     len(self.groups[group].param_set[0]),
                                                     (c_double * len(params))(*params))
                if message[0] == 'test_timeout':
                    melissa_comm4py.send_alive()
                if message[0] == 'options':
                    melissa_comm4py.send_options(self.server[0].options.encode())


            logging.info('closing responder thread')
        except:
            print('=== Responder thread crashed ===')
            logging.error(traceback.print_exc())
            return

class Server_user_functions(object):
    """
        Options for the simulations
    """
    def __init__(self, parent):
        self.parent = parent

    def create(self, func_ptr):
        self.parent.usr_func['create_server'] = func_ptr

    def launch(self, func_ptr):
        self.parent.usr_func['launch_server'] = func_ptr

    def check_job(self, func_ptr):
        self.parent.usr_func['check_server_job'] = func_ptr

    def restart(self, func_ptr):
        self.parent.usr_func['restart_server'] = func_ptr

    def cancel_job(self, func_ptr):
        self.parent.usr_func['cancel_server_job'] = func_ptr

    def finalize(self, func_ptr):
        self.parent.usr_func['finalize_server'] = func_ptr

class Simulation_user_functions(object):
    """
        Options for the simulations
    """
    def __init__(self, parent):
        self.parent = parent

    def create(self, func_ptr):
        self.parent.usr_func['create_group'] = func_ptr

    def launch(self, func_ptr):
        self.parent.usr_func['launch_group'] = func_ptr

    def check_job(self, func_ptr):
        self.parent.usr_func['check_group_job'] = func_ptr

    def restart(self, func_ptr):
        self.parent.usr_func['restart_group'] = func_ptr

    def cancel_job(self, func_ptr):
        self.parent.usr_func['cancel_group_job'] = func_ptr

    def finalize(self, func_ptr):
        self.parent.usr_func['finalize_group'] = func_ptr

class Study(object):
    """
        Study class, containing instances of the threads
    """
    def __init__(self, stdy_opt = None, ml_stats = None, usr_func = None):
        self.nb_param = 0
        self.groups = list()
        self.server_obj = [Server()]
        self.sobol = False
        if stdy_opt is None:
            self.stdy_opt = dict()
        else:
            try:
                self.stdy_opt = dict(stdy_opt)
            except:
                logging.error(traceback.print_exc())
                return 1

        if ml_stats is None:
            self.ml_stats = dict()
        else:
            try:
                self.ml_stats = dict(ml_stats)
            except:
                logging.error(traceback.print_exc())
                return 1

        if usr_func is None:
            self.usr_func = dict()
        else:
            try:
                self.usr_func = dict(usr_func)
            except:
                logging.error(traceback.print_exc())
                return 1

        self.threads = dict()
        self.server = Server_user_functions(self)
        self.simulation = Simulation_user_functions(self)

    # options
    def set_option(self, option_key, option_value):
        self.stdy_opt[option_key] = option_value

    def get_option(self, option_key):
        return self.stdy_opt[option_key]

    def set_threshold_values(self, threshold_values):
        self.stdy_opt['threshold_values'] = threshold_values

    def get_threshold_values(self):
        return self.stdy_opt['threshold_values']

    def set_quantile_values(self, quantile_values):
        self.stdy_opt['quantile_values'] = quantile_values

    def get_quantile_values(self):
        return self.stdy_opt['quantile_values']

    def set_working_directory(self, working_directory):
        self.stdy_opt['working_directory'] = working_directory

    def get_working_directory(self):
        return self.stdy_opt['working_directory']

    def set_sampling_size(self, sampling_size):
        self.stdy_opt['sampling_size'] = sampling_size

    def get_sampling_size(self):
        return self.stdy_opt['sampling_size']

    def set_nb_timesteps(self, nb_timesteps):
        self.stdy_opt['nb_timesteps'] = nb_timesteps

    def get_nb_timesteps(self):
        return self.stdy_opt['nb_timesteps']

    def set_field_names(self, field_names):
        self.stdy_opt['field_names'] = field_names

    def get_field_names(self):
        return self.stdy_opt['field_names']

    def set_simulation_timeout(self, simulation_timeout):
        self.stdy_opt['simulation_timeout'] = simulation_timeout

    def get_simulation_timeout(self):
        return self.stdy_opt['simulation_timeout']

    def set_checkpoint_interval(self, checkpoint_interval):
        self.stdy_opt['checkpoint_interval'] = checkpoint_interval

    def get_checkpoint_interval(self):
        return self.stdy_opt['checkpoint_interval']

    def set_coupling(self, coupling):
        self.stdy_opt['coupling'] = coupling

    def get_coupling(self):
        return self.stdy_opt['coupling']

    def set_verbosity(self, verbosity):
        self.stdy_opt['verbosity'] = verbosity

    def get_verbosity(self):
        return self.stdy_opt['verbosity']

    def set_batch_size(self, batch_size):
        self.stdy_opt['batch_size'] = batch_size

    def set_assimilation(self, assimilation):
        self.stdy_opt['assimilation'] = assimilation

    def get_batch_size(self):
        return self.stdy_opt['batch_size']

    def set_param_distribution(self, param_distribution):
        self.stdy_opt['param_distribution'] = param_distribution

    def get_param_distribution(self):
        return self.stdy_opt['param_distribution']

    def set_disable_fault_tolerance(self, disable_fault_tolerance):
        self.stdy_opt['disable_fault_tolerance'] = disable_fault_tolerance

    def get_disable_fault_tolerance(self):
        return self.stdy_opt['disable_fault_tolerance']

    # stats
    def compute_stat(self, stat_key):
        self.ml_stats[stat_key] = True

    def compute_mean(self):
        self.ml_stats['mean'] = True

    def compute_variance(self):
        self.ml_stats['variance'] = True

    def compute_skewness(self):
        self.ml_stats['skewness'] = True

    def compute_kurtosis(self):
        self.ml_stats['kurtosis'] = True

    def compute_min(self):
        self.ml_stats['min'] = True

    def compute_max(self):
        self.ml_stats['max'] = True

    def compute_threshold_exceedances(self, threshold_values):
        self.stdy_opt['threshold_values'] = threshold_values
        self.ml_stats['threshold_exceedance'] = True

    def compute_quantiles(self, quantile_values):
        self.stdy_opt['quantile_values'] = quantile_values
        self.ml_stats['quantiles'] = True

    def compute_sobol_indices(self, coupling = "MELISSA_COUPLING_DEFAULT"):
        self.stdy_opt['coupling'] = coupling
        self.ml_stats['sobol_indices'] = True

    # functions
    def set_function(self, func_key, func_ptr):
        self.usr_func[func_key] = func_ptr

    def check_job(self, func_ptr):
        self.usr_func['check_job'] = func_ptr

    def check_scheduler_load(self, func_ptr):
        self.usr_func['check_scheduler_load'] = func_ptr

    def cancel_job(self, func_ptr):
        self.usr_func['cancel_job'] = func_ptr

    def postprocessing(self, func_ptr):
        self.usr_func['postprocessing'] = func_ptr

    def finalize(self, func_ptr):
        self.usr_func['finalize'] = func_ptr

    def run(self):
        """
            Main study method
        """
        if not os.path.isdir(self.stdy_opt['working_directory']):
            os.mkdir(self.stdy_opt['working_directory'])
        os.chdir(self.stdy_opt['working_directory'])

        nb_errors = self.check_options()
        if nb_errors > 0:
            logging.error(str(nb_errors) + ' errors in options')
            exit()

        Job.set_usr_func(self.usr_func)
        Job.set_stdy_opt(self.stdy_opt)
        Job.set_ml_stats(self.ml_stats)
        Job.set_nb_param(self.nb_param)

        logging.debug('create threads dict')
        self.threads['messenger'] = Messenger(self.stdy_opt['batch_size'], self.server_obj, self.groups)
        self.threads['state_checker'] = StateChecker(self.server_obj, self.groups)
        self.threads['responder'] = Responder(self.stdy_opt['batch_size'], self.server_obj, self.groups)

        create_study(self.usr_func)
        self.create_group_list()
        # init zmq context
        melissa_comm4py.init_context()
        logging.debug('bind to recv port '+str(self.stdy_opt['recv_port']))
        print('bind to recv port '+str(self.stdy_opt['recv_port']))
        melissa_comm4py.bind_message_rcv(str(self.stdy_opt['recv_port']).encode())
        melissa_comm4py.bind_message_resp(str(self.stdy_opt['resp_port']).encode())
        logging.info('submit server')

        self.server_obj[0].set_nb_param(self.nb_param)
        self.server_obj[0].set_path(self.stdy_opt['working_directory'])

        self.server_obj[0].create_options()

        try:
            self.server_obj[0].launch()
        except:
            logging.error('Error while launching server')
            print('=== Error while launching server ===')
            logging.error(traceback.print_exc())
            self.stop()
            return
        logging.debug('start messenger thread')
        self.threads['messenger'].start()
        logging.debug('wait server start')
        try:
            self.server_obj[0].wait_start()
        except:
            logging.error('Error while waiting server')
            print('=== Error while waiting server ===')
            logging.error(traceback.print_exc())
            self.stop()
            return
        self.server_obj[0].write_node_name()
        # connect to server
        logging.debug('connect to server port '+str(self.stdy_opt['send_port']))
        #melissa_comm4py.connect_message_snd(str(server.node_name), str(self.stdy_opt['send_port']))
        melissa_comm4py.bind_message_snd(str(self.stdy_opt['send_port']).encode())
        logging.debug('start status checker thread')
        self.threads['state_checker'].start()
        self.threads['responder'].start()
        for group in self.groups:
            if self.fault_tolerance() != 0: return
            while check_scheduler_load(self.usr_func) == False:  #TODO: need to check this als during the simulation, not only on the beginnint!
                if self.stdy_opt['assimilation']:
                    if self.server_obj[0].want_stop:
                        break  # Refactor all those breaks!
                else:
                    # don't start to quick after each other... achtually that should not be a problem!
                    time.sleep(1)
                if self.fault_tolerance() != 0: return

            if self.server_obj[0].want_stop:  # nice! already finished, break out!
                break

            logging.info('submit group '+str(group.group_id))
            try:
                group.server_node_name = str(self.server_obj[0].node_name[0])
                group.launch()
            except:
                logging.error('Error while launching group')
                print('=== Error while launching group ===')
                logging.error(traceback.print_exc())
                self.stop()
                return
        # time.sleep(1)
        while (not self.server_obj[0].want_stop) and (self.server_obj[0].status != FINISHED
               or any([i.status != FINISHED for i in self.groups])):
            if self.fault_tolerance() != 0: return
            time.sleep(0.05)
        # time.sleep(1)
        self.server_obj[0].finalize()
        self.stop()
        return

    def stop(self):

        logging.info('Stopping study')
        for group in self.groups:
            if group.status < FINISHED and group.status > NOT_SUBMITTED:
                with group.lock:
                    group.cancel()
        if self.server_obj[0].job_status < FINISHED:
            self.server_obj[0].cancel()
        self.threads['messenger'].running_study = False
        self.threads['responder'].running_study = False
        self.threads['state_checker'].running_study = False
        self.threads['messenger'].join()
        self.threads['responder'].join()
        self.threads['state_checker'].join()
        postprocessing(self.usr_func)
        finalize(self.usr_func)
        # finalize zmq context
        melissa_comm4py.close_message()

        logging.info('Study stopped')

    def check_options(self):
        """
            Validates user provided options
        """
        errors = 0
        if (not 'sobol_indices' in self.ml_stats.keys()):
            self.ml_stats['sobol_indices'] = False

        if (not 'mean' in self.ml_stats.keys()):
            self.ml_stats['mean'] = False

        if (not 'variance' in self.ml_stats.keys()):
            self.ml_stats['variance'] = False

        if (not 'skewness' in self.ml_stats.keys()):
            self.ml_stats['skewness'] = False

        if (not 'kurtosis' in self.ml_stats.keys()):
            self.ml_stats['kurtosis'] = False

        if (not 'min' in self.ml_stats.keys()):
            self.ml_stats['min'] = False

        if (not 'max' in self.ml_stats.keys()):
            self.ml_stats['max'] = False

        if (not 'threshold_exceedance' in self.ml_stats.keys()):
            self.ml_stats['threshold_exceedance'] = False

        if (not 'quantiles' in self.ml_stats.keys()):
            self.ml_stats['quantiles'] = False

        if (not 'assimilation' in self.stdy_opt):
            self.stdy_opt['assimilation'] = False

        if (not 'server_cores' in self.stdy_opt):
            self.stdy_opt['server_cores'] = -1

        if (not 'server_nodes' in self.stdy_opt):
            self.stdy_opt['server_nodes'] = -1

        if (not 'simulation_cores' in self.stdy_opt):
            self.stdy_opt['simulation_cores'] = -1

        if (not 'simulation_nodes' in self.stdy_opt):
            self.stdy_opt['simulation_nodes'] = -1

        if self.stdy_opt['assimilation']:
            self.usr_func['draw_parameter_set'] = lambda : []

        test_parameters = draw_parameter_set(self.usr_func, self.stdy_opt)
        self.nb_param = len(test_parameters)

        # refactor option error checking?
        if not self.stdy_opt['assimilation']:
            if not self.ml_stats['sobol_indices'] and self.nb_param < 1:
                logging.error('Error bad option: not enough parameters')
                errors += 1
            if self.ml_stats['sobol_indices'] and self.nb_param < 2:
                logging.error('Error bad option: not enough parameters')
                errors += 1
            if self.stdy_opt['sampling_size'] < 1:
                logging.error('Error bad option: sample_size not big enough')
                errors += 1

        if (not 'verbosity' in self.stdy_opt):
            self.stdy_opt['verbosity'] = 2

        if (not 'disable_fault_tolerance' in self.stdy_opt):
            self.stdy_opt['disable_fault_tolerance'] = False

        if (not 'batch_size' in self.stdy_opt):
            self.stdy_opt['batch_size'] = 1

        # Refactor: omg these port names are bullshit!
        if (not 'send_port' in self.stdy_opt):
            self.stdy_opt['send_port'] = 5556

        if (not 'recv_port' in self.stdy_opt):
            self.stdy_opt['recv_port'] = 5555

        if (not 'resp_port' in self.stdy_opt):
            self.stdy_opt['resp_port'] = 5554

        if (not 'data_port' in self.stdy_opt):
            self.stdy_opt['data_port'] = 2006


        if (not 'learning' in self.stdy_opt):
            self.stdy_opt['learning'] = False
        else:
            if (not 'nn_path' in self.stdy_opt):
                self.stdy_opt['nn_path'] = melissa_install_prefix + "/lib"


        if (self.ml_stats['sobol_indices']):
            self.stdy_opt['batch_size'] = 1
            self.sobol = True

        if self.stdy_opt['send_port'] == self.stdy_opt['recv_port']:
            logging.error('Error: send_port and recv_port can not have the same value')
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

        if 'threshold_values' in self.stdy_opt:
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

        if 'quantile_values' in self.stdy_opt:
            if type(self.stdy_opt['quantile_values']) not in (list, tuple, set):
                self.stdy_opt['quantile_values'] = [self.stdy_opt['quantile_values']]

        if not 'check_server_job' in self.usr_func.keys():
            if 'check_job' in self.usr_func.keys():
                self.usr_func['check_server_job'] = self.usr_func['check_job']

        if not 'check_group_job' in self.usr_func.keys():
            if 'check_job' in self.usr_func.keys():
                self.usr_func['check_group_job'] = self.usr_func['check_job']  # TODO: refactor how these functions work! it's dirty that they change their parameter!

        if not 'cancel_server_job' in self.usr_func.keys():
            if 'cancel_job' in self.usr_func.keys():
                self.usr_func['cancel_server_job'] = self.usr_func['cancel_job']

        if not 'cancel_group_job' in self.usr_func.keys():
            if 'cancel_job' in self.usr_func.keys():
                self.usr_func['cancel_group_job'] = self.usr_func['cancel_job']


        optional_func = ['create_study',
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
                          'cancel_server_job',
                          'cancel_group_job']

        for func_name in optional_func:
            if not func_name in self.usr_func.keys():
                self.usr_func[func_name] = None
                logging.warning('Warning: no \''+func_name+'\' key in USER_FUNCTIONS')
            elif self.usr_func[func_name] is None:
                logging.warning('Warning: no \''+func_name+'\' provided')

        for func_name in mandatory_func:
            if not func_name in self.usr_func.keys():
                logging.error('Error: no \''+func_name+'\' key in USER_FUNCTIONS')
                errors += 1

        return errors

    def create_group_list(self):
        """
            Job list creation
        """
        Group.reset()
        MultiSimuGroup.reset()

        if self.sobol:
            for i in range(self.stdy_opt['sampling_size']):
                input_parameters = draw_parameter_set(self.usr_func, self.stdy_opt)
                input_parameters2 = draw_parameter_set(self.usr_func, self.stdy_opt)
                self.groups.append(SobolGroup(
                    input_parameters,
                    input_parameters2))
        else:
            for i in range(self.stdy_opt['sampling_size']//self.stdy_opt['batch_size']):
                param_sets = list()
                for j in range(self.stdy_opt['batch_size']):
                    param_sets.append(draw_parameter_set(self.usr_func, self.stdy_opt))
                self.groups.append(MultiSimuGroup(param_sets))

        logging.debug('created %d groups' % len(self.groups))

        for group in self.groups:
            group.create()
        # groups = self.groups

    def fault_tolerance(self):
        """
            Compares job status and study status, restart crashed groups
        """

        if self.stdy_opt['disable_fault_tolerance']:
            return 0

        # with self.server_obj[0].lock:
            # if self.server_obj[0].status == FINISHED:
                # return 0

        # check if threads are still alive
        if not self.threads['messenger'].isAlive():
            logging.error('Messenger thread crashed')
            self.threads['messenger'].join()
            if self.server_obj[0].status != FINISHED:
                self.threads['messenger'] = Messenger(self.stdy_opt['batch_size'])
                self.threads['messenger'].start()
        if not self.threads['state_checker'].isAlive():
            logging.error('State checker thread crashed')
            self.threads['state_checker'].join()
            if self.server_obj[0].status != FINISHED:
                self.threads['state_checker'] = StateChecker()
                self.threads['state_checker'].start()
        if not self.threads['responder'].isAlive():
            logging.error('Responder thread crashed')
            self.threads['responder'].join()
            if self.server_obj[0].status != FINISHED:
                self.threads['responder'] = Responder()
                self.threads['responder'].start()

        sleep = False
        with self.server_obj[0].lock:
            if self.server_obj[0].status != RUNNING or self.server_obj[0].job_status != RUNNING:
                sleep = True
        if sleep == True:
            time.sleep(3)
            sleep = False

        want_stop = False
        with self.server_obj[0].lock:  # did we get a want stop in the meantime? ... Refactor race conditions!
            want_stop = self.server_obj[0].want_stop

        if (not want_stop and ((self.server_obj[0].status != RUNNING or self.server_obj[0].job_status > RUNNING)
                and self.server_obj[0].status != FINISHED)
                and not all([i.status == FINISHED for i in self.groups])):
            print('server status: '+str(self.server_obj[0].status)+' job_status: '+str(self.server_obj[0].job_status))
            for group in self.groups:
                if group.status < FINISHED and group.status > NOT_SUBMITTED:
                    with group.lock:
                        group.cancel()
            logging.info('resubmit server job')
            time.sleep(1)
            try:
                self.server_obj[0].cancel()
                self.server_obj[0].restart()
            except:
                logging.error('Error while restarting server')
                print('=== Error while restarting server ===')
                logging.error(traceback.print_exc())
                self.stop()
                return 1
            logging.debug('wait server restart')
            try:
                self.server_obj[0].wait_start()
            except:
                logging.error('Error while waiting server')
                print('=== Error while waiting server ===')
                logging.error(traceback.print_exc())
                self.stop()
                return 1

            logging.info('server start')
            self.server_obj[0].write_node_name()
            # connect to server
            #logging.debug('connect to server port '+str(self.stdy_opt['send_port']))
            #melissa_comm4py.connect_message_snd(str(server.node_name[0]), str(self.stdy_opt['send_port']))

            time.sleep(1)
            for group in self.groups:
                if group.status < FINISHED and group.status > NOT_SUBMITTED:
                    with group.lock:
                        logging.info('resubmit group ' + str(group.group_id)
                                     + ' (server crash)')
                        try:
                            group.server_node_name = str(self.server_obj[0].node_name[0])
                            group.restart()
                        except:
                            logging.error('Error while restarting group '+group.group_id)
                            print('=== Error while restarting group '+group.group_id+' ===')
                            logging.error(traceback.print_exc())
                            self.stop()
                            return 1
            time.sleep(2)

        for group in self.groups:
            if group.status > NOT_SUBMITTED and group.status < FINISHED:
                with group.lock:
                    if group.status <= RUNNING and group.job_status == FINISHED:
                        sleep = True
            if sleep == True:
                time.sleep(10)
                sleep = False
                with group.lock:
                    if group.status <= RUNNING:
                        logging.info("resubmit group " + str(group.group_id)
                                     + " (simulation crashed)")
                        try:
                            group.restart()
                        except:
                            logging.error('Error while restarting group '+str(group.group_id))
                            print('=== Error while restarting group '+str(group.group_id)+' ===')
                            logging.error(traceback.print_exc())
                            self.stop()
                            return 1
            with group.lock:
                if group.status == WAITING:
                    if group.job_status == RUNNING:
                        if time.time() - group.start_time > self.stdy_opt['simulation_timeout']:
                            logging.info("resubmit group " + str(group.group_id)
                                         + " (timeout detected by launcher)")
                            try:
                                group.restart()
                            except:
                                logging.error('Error while restarting group '+group.group_id)
                                print('=== Error while restarting group '+group.group_id+' ===')
                                logging.error(traceback.print_exc())
                                self.stop()
                                return 1
                if group.status == TIMEOUT:
                    logging.info("resubmit group " + str(group.group_id)
                                 + " (timeout detected by server)")
                    try:
                        group.restart()
                    except:
                        logging.error('Error while restarting group '+group.group_id)
                        print('=== Error while restarting group '+group.group_id+' ===')
                        logging.error(traceback.print_exc())
                        self.stop()
                        return 1
#    time.sleep(1)
        return 0


def create_study(usr_func):
    """
        Creates study environment
    """
    if "create_study" in usr_func.keys() \
    and usr_func['create_study']:
        try:
            usr_func['create_study']()
        except:
            logging.warning("Create study failed")
            traceback.print_exc()
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
            logging.error(traceback.print_exc())
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
    if "check_scheduler_load" in usr_func.keys() \
    and usr_func['check_scheduler_load']:
        try:
            return usr_func['check_scheduler_load']()
        except:
            logging.warning("Check scheduler load failed")
            logging.error(traceback.print_exc())
            return True
    else:
        return True

# Study end #

def postprocessing(usr_func):
    """
        User defined postprocessing
    """
    if "postprocessing" in usr_func.keys() \
    and usr_func['postprocessing']:
        try:
            usr_func['postprocessing']()
        except:
            logging.warning("Postprocessing failed")
            traceback.print_exc()
    else:
        pass

def finalize(usr_func):
    """
        User defined final step
    """
    if "finalize" in usr_func.keys() \
    and usr_func['finalize']:
        try:
            usr_func['finalize']()
        except:
            logging.warning("Finalize failed")
            logging.error(traceback.print_exc())
    else:
        pass

