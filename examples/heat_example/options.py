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
    User defined options module
"""
import os
import time
import numpy as np
import subprocess
import getpass
import imp
from matplotlib import pyplot as plt
from matplotlib import cm
from string import Template
from shutil import copyfile

imp.load_source("options", "./scripts/options_local.py")
from options import *

USERNAME = getpass.getuser()
BUILD_WITH_MPI = '@BUILD_WITH_MPI@'.upper()
BUILD_WITH_FLOWVR = '@BUILD_WITH_FLOWVR@'.upper()
BUILD_EXAMPLES_WITH_MPI = '@BUILD_EXAMPLES_WITH_MPI@'.upper()
EXECUTABLE='heatc'
BATCH_SCHEDULER = "local"
WALLTIME_SERVER = 600
NODES_SERVER = 3
WALLTIME_SIMU = 300
NODES_GROUP = 2


def create_flowvr_group(executable, args, group_id, nb_proc_simu, nb_parameters):
    content = ""
    file=open("@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/scripts/flowvr_group.py", "r")
    content = Template(file.read()).substitute(args=str(args),
                                     group_id=str(group_id),
                                     np_simu=str(nb_proc_simu),
                                     nb_param=str(nb_parameters),
                                     executable=str(executable))
    file.close()
    file=open("create_group"+str(group_id)+".py", "w")
    file.write(content)
    file.close()

def draw_param_set():
    param_set = np.zeros(STUDY_OPTIONS['nb_parameters'])
    for i in range(STUDY_OPTIONS['nb_parameters']):
        param_set[i] = np.random.uniform(0, 1)
    return param_set

STUDY_OPTIONS = {}
STUDY_OPTIONS['user_name'] = USERNAME
STUDY_OPTIONS['working_directory'] = '@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/STATS'
STUDY_OPTIONS['nb_parameters'] = 5                 # number of varying parameters of the study
STUDY_OPTIONS['sampling_size'] = 10                 # initial number of parameter sets
STUDY_OPTIONS['nb_time_steps'] = 100               # number of timesteps, from Melissa point of view
STUDY_OPTIONS['threshold_value'] = 0.7
STUDY_OPTIONS['quantile_values'] = [0.05,0.25,0.5,0.75,0.95]
STUDY_OPTIONS['field_names'] = ["heat1"]           # list of field names
STUDY_OPTIONS['simulation_timeout'] = 40           # simulations are restarted if no life sign for 40 seconds
STUDY_OPTIONS['checkpoint_interval'] = 30          # server checkpoints every 30 seconds
STUDY_OPTIONS['coupling'] = "MELISSA_COUPLING_MPI" # option for Sobol' simulation groups coupling

MELISSA_STATS = {}
MELISSA_STATS['mean'] = True
MELISSA_STATS['variance'] = True
MELISSA_STATS['min'] = True
MELISSA_STATS['max'] = True
MELISSA_STATS['threshold_exceedance'] = True
MELISSA_STATS['quantiles'] = True
MELISSA_STATS['sobol_indices'] = True

USER_FUNCTIONS = {}
USER_FUNCTIONS['create_study'] = None
USER_FUNCTIONS['draw_parameter_set'] = draw_param_set
USER_FUNCTIONS['create_group'] = None
#if MELISSA_STATS['sobol_indices']:
#    USER_FUNCTIONS['launch_group'] = launch_group
#else:
USER_FUNCTIONS['launch_group'] = launch_simu
USER_FUNCTIONS['launch_server'] = launch_server
USER_FUNCTIONS['check_server_job'] = check_job
USER_FUNCTIONS['check_group_job'] = check_job
USER_FUNCTIONS['restart_server'] = launch_server
USER_FUNCTIONS['restart_group'] = None
USER_FUNCTIONS['check_scheduler_load'] = check_load
USER_FUNCTIONS['cancel_job'] = kill_job
USER_FUNCTIONS['postprocessing'] = @HEAT_VISU@
USER_FUNCTIONS['finalize'] = None
