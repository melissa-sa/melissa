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
    user defined options
"""

import numpy as np

def draw_param_set():
    param_set = np.zeros(STUDY_OPTIONS['nb_parameters'])
    for i in range(STUDY_OPTIONS['nb_parameters']):
        param_set[i] = np.random.uniform(0, 1)
    return param_set

GLOBAL_OPTIONS = {}
GLOBAL_OPTIONS['user_name'] = "user"
GLOBAL_OPTIONS['working_directory'] = "/home/user/melissa_study"

STUDY_OPTIONS = {}
STUDY_OPTIONS['nb_parameters'] = 5                  # number of varying parameters of the study
STUDY_OPTIONS['sampling_size'] = 10                 # initial number of parameter sets
STUDY_OPTIONS['nb_time_steps'] = 100                # number of timesteps, from Melissa point of view
STUDY_OPTIONS['threshold_value'] = 0.7
STUDY_OPTIONS['field_names'] = ["field1", "field2"] # list of field names
STUDY_OPTIONS['simulation_timeout'] = 400           # simulations are restarted if no life sign for 400 seconds (int)
STUDY_OPTIONS['checkpoint_interval'] = 300          # server checkpoints every 300 seconds (double)

MELISSA_STATS = {}
MELISSA_STATS['mean'] = True
MELISSA_STATS['variance'] = True
MELISSA_STATS['min'] = True
MELISSA_STATS['max'] = True
MELISSA_STATS['threshold_exceedance'] = True
MELISSA_STATS['quantile'] = True
MELISSA_STATS['sobol_indices'] = True

USER_FUNCTIONS = {}
USER_FUNCTIONS['create_study'] = None
USER_FUNCTIONS['draw_parameter_set'] = draw_param_set
USER_FUNCTIONS['create_group'] = None
USER_FUNCTIONS['launch_server'] = None
USER_FUNCTIONS['launch_group'] = None
USER_FUNCTIONS['check_server_job'] = None
USER_FUNCTIONS['check_group_job'] = None
USER_FUNCTIONS['cancel_job'] = None
USER_FUNCTIONS['restart_server'] = None
USER_FUNCTIONS['restart_group'] = None
USER_FUNCTIONS['check_scheduler_load'] = None
USER_FUNCTIONS['postprocessing'] = None
USER_FUNCTIONS['finalize'] = None
