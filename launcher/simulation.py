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
    Simulations and server jobs module
"""

import numpy
import os
import time
import subprocess
import logging
#import openturns as ot
from threading import RLock
from ctypes import cdll, create_string_buffer, c_char_p, c_wchar_p, c_int, c_double, POINTER

melissa_install_prefix = os.getenv('MELISSA_INSTALL_PREFIX')
assert(melissa_install_prefix)

c_int_p = POINTER(c_int)
c_double_p = POINTER(c_double)
melissa_comm4py = cdll.LoadLibrary(melissa_install_prefix + '/lib/libmelissa_comm4py.so')
melissa_comm4py.send_message.argtypes = [c_char_p]
melissa_comm4py.send_job.argtypes = [c_int, c_char_p, c_int, c_double_p]
melissa_comm4py.send_drop.argtypes = [c_int, c_char_p]
melissa_comm4py.send_options.argtypes = [c_char_p]

# Jobs and executions status

NOT_SUBMITTED = -1
PENDING = 0
WAITING = 0
RUNNING = 1
FINISHED = 2
TIMEOUT = 4
COUPLING_DICT = {"MELISSA_COUPLING_NONE":0,
                 "MELISSA_COUPLING_DEFAULT":0,
                 "MELISSA_COUPLING_ZMQ":0,
                 "MELISSA_COUPLING_MPI":1,
                 "MELISSA_COUPLING_FLOWVR":2}

class Job(object):
    """
        Job class
    """
    usr_func = {}
    stdy_opt = {}
    ml_stats = {}
    nb_param = 0
    def __init__(self):
        """
            Job constructor
        """
        self.job_status = NOT_SUBMITTED
        self.job_id = 0
        self.cmd_opt = ''
        self.start_time = 0.0
        self.job_type = 0
        self.node_name = ['']

    def set_usr_func(usr_func):
        Job.usr_func = usr_func

    def set_stdy_opt(stdy_opt):
        Job.stdy_opt = stdy_opt

    def set_ml_stats(ml_stats):
        Job.ml_stats = ml_stats

    def set_nb_param(nb_param):
        Job.nb_param = nb_param

    set_usr_func = staticmethod(set_usr_func)
    set_stdy_opt = staticmethod(set_stdy_opt)
    set_ml_stats = staticmethod(set_ml_stats)
    set_nb_param = staticmethod(set_nb_param)

    def cancel(self):
        """
            Cancels a job (mandatory)
        """
        if "cancel_job" in Job.usr_func.keys() \
        and Job.usr_func['cancel_job']:
            return Job.usr_func['cancel_job'](self)
        else:
            logging.error('Error: no \'cancel_job\' function provided')
            exit()

    def finalize(self):
        """
            Finalize a job (optional)
        """
        pass

class Group(Job):
    """
        Group class
    """
    nb_groups = 0
    def __init__(self):
        """
            Group constructor
        """
        Job.__init__(self)
        self.nb_param = Job.nb_param
        self.size = 0
        self.server_node_name = ''
        self.nb_restarts = 0
        self.status = NOT_SUBMITTED
        self.lock = RLock()
        self.coupling = 0
        self.group_id = Group.nb_groups
        Group.nb_groups += 1

#    @classmethod
    def reset():
        Group.nb_groups = 0

    reset = staticmethod(reset)

    def create(self):
        """
            Creates a group environment
        """
        if "create_group" in Job.usr_func.keys() \
        and Job.usr_func['create_group']:
            Job.usr_func['create_group'](self)

    def launch(self):
        """
            Launches the group (mandatory)
        """
        if "launch_group" in Job.usr_func.keys() \
        and Job.usr_func['launch_group']:
            Job.usr_func['launch_group'](self)
        else:
            logging.error('Error: no \'launch_group\' function provided')
            exit()
        self.job_status = PENDING
        self.status = WAITING

    def check_job(self):
        """
            Checks the group job status (mandatory)
        """
        if "check_group_job" in Job.usr_func.keys() \
        and Job.usr_func['check_group_job']:
            return Job.usr_func['check_group_job'](self)
        elif "check_job" in Job.usr_func.keys() \
        and Job.usr_func['check_job']:
            return Job.usr_func['check_job'](self)
        else:
            logging.error('Error: no \'check_group_job\''
                          +' function provided')
            exit()

    def cancel(self):
        """
            Cancels a simulation job (mandatory)
        """
        if "cancel_group_job" in Job.usr_func.keys() \
        and Job.usr_func['cancel_group_job']:
            return Job.usr_func['cancel_group_job'](self)
        elif "cancel_job" in Job.usr_func.keys() \
        and Job.usr_func['cancel_job']:
           return Job.usr_func['cancel_job'](self)
        else:
            logging.error('Error: no \'cancel_job\' function provided')
            exit()

    def finalize(self):
        """
            Finalize the group (optional)
        """
        if "finalize_group" in Job.usr_func.keys():
            if Job.usr_func['finalize_group']:
                Job.usr_func['finalize_group'](self)

class MultiSimuGroup(Group):
    """
    Multiple simulation group class (without Sobol' indices)
    """
    nb_simu = 0
    def __init__(self, param_sets):
        """
            MultiSimuGroup constructor
        """
        Group.__init__(self)
        self.param_set = list()
        self.simu_id = list()
        self.size = len(param_sets)
        self.simu_status = list()
        self.coupling = 0
        self.job_type = 3
        for i in range(self.size):
            #if isinstance(param_sets[i], ot.Point):
                #self.param_set.append(numpy.zeros(self.nb_param))
                #for j in range(self.nb_param):
                    #self.param_set[i][j] = float(param_sets[i][j])
            #else:
            self.param_set.append(numpy.copy(param_sets[i]))
            self.simu_id.append(MultiSimuGroup.nb_simu)
            self.simu_status.append(0)
            MultiSimuGroup.nb_simu += 1

    def reset():
        MultiSimuGroup.nb_simu = 0

    reset = staticmethod(reset)

    def launch(self):
        Group.launch(self)
        for i in range(self.size):
            melissa_comm4py.send_job_init(self.simu_id[i],
                                          str(self.job_id).encode(),
                                          len(self.param_set[i]),
                                          self.param_set[i].ctypes.data_as(POINTER(c_double)))

    def restart(self):
        """
            Ends and restarts the simulations (mandatory)
        """
        crashs_before_redraw = 3
        self.cancel()  ## TODO: async this one and alike
        self.nb_restarts += 1
        if self.nb_restarts > crashs_before_redraw:
            #logging.warning('Simulation group ' + str(self.group_id) +
                            #'crashed 5 times, drawing new parameter sets')
            #for i in range(self.size):
                #logging.info('old parameter set: ' + str(self.param_set[i]))
                #self.param_set[i] = Job.usr_func['draw_parameter_set']()
                #logging.info('new parameter set: ' + str(self.param_set[i]))
            #self.nb_restarts = 0
            logging.warning('Simulation group ' + str(self.group_id) +
                            'crashed '+str(crashs_before_redraw)+' times, remove simulation')
            for i in range(self.size):
                logging.warning('Bad parameter set '+str(i)+': ' + str(self.param_set[i]))
                melissa_comm4py.send_drop(self.simu_id[i], str(self.job_id).encode())
            self.job_status = FINISHED
            self.status = FINISHED
            return

        if "restart_group" in Job.usr_func.keys() \
        and Job.usr_func['restart_group']:
            Job.usr_func['restart_group'](self)
        else:
            logging.warning('warning: no \'restart_group\''
                            +' function provided,'
                            +' using \'launch_group\' instead')
            self.launch()
        self.job_status = PENDING
        self.status = WAITING
        ParamArray = c_double * len(self.param_set[0])
        for i in range(self.size):
            melissa_comm4py.send_job_init(self.simu_id[i],
                                          str(self.job_id).encode(),
                                          len(self.param_set[i]),
                                          self.param_set[i].ctypes.data_as(POINTER(c_double)))


class SobolGroup(Group):
    """
        Sobol coupled group class
    """
    def __init__(self, param_set_a, param_set_b):
        """
            SobolGroup constructor
        """
        Group.__init__(self)
        self.param_set = list()
        self.simu_id = list()
        self.job_type = 4
        self.coupling = COUPLING_DICT.get(Job.stdy_opt['coupling'].upper(), "MELISSA_COUPLING_DEFAULT")
        #if isinstance(param_set_a, ot.Point):
            #self.param_set.append(numpy.zeros(self.nb_param))
            #for j in range(self.nb_param):
                #self.param_set[0][j] = float(param_set_a[j])
        #else:
        self.param_set.append(numpy.copy(param_set_a))
        self.simu_id.append(self.group_id*(len(param_set_a)+2))
        #if isinstance(param_set_b, ot.Point):
            #self.param_set.append(numpy.zeros(self.nb_param))
            #for j in range(self.nb_param):
                #self.param_set[1][j] = float(param_set_b[j])
        #else:
        self.param_set.append(numpy.copy(param_set_b))
        self.simu_id.append(self.group_id*(len(param_set_a)+2)+1)
        for i in range(len(param_set_a)):
            self.param_set.append(numpy.copy(self.param_set[0]))
            self.param_set[i+2][i] = numpy.copy(self.param_set[1][i])
            self.simu_id.append(self.group_id*(len(param_set_a)+2)+i+2)
        self.size = len(self.param_set)

    def launch(self):
        Group.launch(self)
        melissa_comm4py.send_job_init(self.group_id,
                                      str(self.job_id).encode(),
                                      len(self.param_set[0]),
                                      self.param_set[0].ctypes.data_as(POINTER(c_double)))

    def restart(self):
        """
            Ends and restarts the Sobol group (mandatory)
        """
        crashs_before_redraw = 3
        self.cancel()
        self.nb_restarts += 1
        if self.nb_restarts > crashs_before_redraw:
            #logging.warning('Group ' +
                            #str(self.group_id) +
                            #'crashed 5 times, drawing new parameter sets')
            #logging.debug('old parameter set A: ' + str(self.param_set[0]))
            #logging.debug('old parameter set B: ' + str(self.param_set[1]))
            #self.param_set[0] = Job.usr_func['draw_parameter_set']()
            #self.param_set[1] = Job.usr_func['draw_parameter_set']()
            #logging.info('new parameter set A: ' + str(self.param_set[0]))
            #logging.info('new parameter set B: ' + str(self.param_set[1]))
            #for i in range(len(self.param_set[0])):
                #self.param_set[i+2] = numpy.copy(self.param_set[0])
                #self.param_set[i+2][i] = numpy.copy(self.param_set[1][i])
            #self.nb_restarts = 0
            logging.warning('Simulation group ' + str(self.group_id) +
                            'crashed '+str(crashs_before_redraw)+' times, remove simulation')
            logging.warning('Bad parameter set A: ' + str(self.param_set[0]))
            logging.warning('Bad parameter set B: ' + str(self.param_set[1]))
            melissa_comm4py.send_drop(self.group_id, str(self.job_id).encode())
            self.job_status = FINISHED
            self.status = FINISHED
            return

        param_str = ""

        if "restart_group" in Job.usr_func.keys() \
        and Job.usr_func['restart_group']:
            Job.usr_func['restart_group'](self)
            param_str = " ".join(str(j) for j in self.param_set[0])
        else:
            logging.warning('warning: no \'restart_group\''
                            +' function provided,'
                            +' using \'launch_group\' instead')
            self.launch()
        self.job_status = PENDING
        self.status = WAITING
        melissa_comm4py.send_job_init(self.group_id,
                                      str(self.job_id).encode(),
                                      len(self.param_set[0]),
                                      self.param_set[0].ctypes.data_as(POINTER(c_double)))

class Server(Job):
    """
        Server class
    """
    def __init__(self):
        """
            Server constructor
        """
        Job.__init__(self)
        self.nb_param = 0
        self.status = WAITING
        self.first_job_id = ''
        self.directory = "./"
#        self.create_options()
        self.lock = RLock()
        self.path = melissa_install_prefix+'/bin'
        self.job_type = 1
        self.options = ''

    def set_path(self, work_dir="./"):
        self.directory = work_dir

    def set_nb_param(self, nb_param):
        self.nb_param = nb_param

    def write_node_name(self):
        os.chdir(self.directory)
        fichier=open("server_name.txt", "w")
        fichier.write(self.node_name[0])
        fichier.close()
        os.system("chmod 744 server_name.txt")

    def create_options(self):
        """
            Melissa Server command line options
        """
        op_str = ':'.join([x for x in Job.ml_stats if Job.ml_stats[x]])
        self.options = ':'.join([x for x in Job.ml_stats if Job.ml_stats[x]])
        field_str = ':'.join([x for x in Job.stdy_opt['field_names']])
        self.options += ' '
        self.options += ':'.join([x for x in Job.stdy_opt['field_names']])
        if field_str == '':
            logging.error('error bad option: no field name given')
            return
        quantile_str = '0'
        if Job.ml_stats['quantiles']:
            quantile_str = ':'.join([str(x) for x in Job.stdy_opt['quantile_values']])
            if quantile_str == '':
                logging.error('error bad option: no quantile value given')
                return
        self.options += ' '
        self.options += quantile_str
        threshold_str = '0'
        if Job.ml_stats['threshold_exceedances']:
            threshold_str = ':'.join([str(x) for x in Job.stdy_opt['threshold_values']])
            if threshold_str == '':
                logging.error('error bad option: no threshold value given')
                return
        self.options += ' '
        self.options += threshold_str
        buff = create_string_buffer(256)
        melissa_comm4py.get_node_name(buff)
        self.cmd_opt = ' '.join(('-o', op_str,
                                 '-p', str(self.nb_param),
                                 '-s', str(Job.stdy_opt['sampling_size']),
                                 '-t', str(Job.stdy_opt['nb_timesteps']),
                                 '-q', quantile_str,
                                 '-e', threshold_str,
                                 '-c', str(Job.stdy_opt['checkpoint_interval']),
                                 '-w', str(Job.stdy_opt['simulation_timeout']),
                                 '-f', field_str,
                                 '-v', str(Job.stdy_opt['verbosity']),
                                 '--txt_push_port', str(Job.stdy_opt['recv_port']),
                                 '--txt_pull_port', str(Job.stdy_opt['send_port']),
                                 '--txt_req_port', str(Job.stdy_opt['resp_port']),
                                 '--data_port', str(Job.stdy_opt['data_port']),
                                 '-n', buff.value.decode()))
        if Job.stdy_opt['learning']:
            self.cmd_opt += " -l "+str(Job.stdy_opt['nn_path'])
        else:
            if op_str == '':
                logging.error('error bad option: no operation given')
                return
        if Job.stdy_opt['disable_fault_tolerance'] == True:
            self.cmd_opt += " --disable_fault_tolerancel"


    def launch(self):
        """
            Launches server job
        """
        os.chdir(self.directory)
        logging.info('launch server')
        logging.info('server options: '+self.cmd_opt)
        if "launch_server" in Job.usr_func.keys() \
        and Job.usr_func['launch_server']:
            Job.usr_func['launch_server'](self)
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
        if not "-r" in self.cmd_opt:
            self.cmd_opt += ' -r . '
        os.chdir(self.directory)
        if "restart_server" in Job.usr_func.keys() \
        and Job.usr_func['restart_server']:
            Job.usr_func['restart_server'](self)
        else:
            logging.warning('Warning: no \'restart_server\' function provided'
                            +' using launch_server instead')
            self.launch()
        with self.lock:
            self.status = WAITING
            self.job_status = PENDING
        self.start_time = 0.0

    def check_job(self):
        """
            Checks server job status
        """
        if "check_server_job" in Job.usr_func.keys() \
         and Job.usr_func['check_server_job']:
            Job.usr_func['check_server_job'](self)
        elif "check_job" in Job.usr_func.keys() \
        and Job.usr_func['check_job']:
            Job.usr_func['check_job'](self)
        else:
            logging.error('Error: no \'check_server_job\' function provided')
            exit()

    def cancel(self):
        """
            Cancels a simulation job (mandatory)
        """
        if "cancel_server_job" in Job.usr_func.keys() \
        and Job.usr_func['cancel_server_job']:
            return Job.usr_func['cancel_server_job'](self)
        elif "cancel_job" in Job.usr_func.keys() \
        and Job.usr_func['cancel_job']:
           return Job.usr_func['cancel_job'](self)
        else:
            logging.error('Error: no \'cancel_job\' function provided')
            exit()

    def finalize(self):
        """
            Finalize the group (optional)
        """
        if "finalize_server" in Job.usr_func.keys():
            if Job.usr_func['finalize_server']:
                Job.usr_func['finalize_server'](self)
