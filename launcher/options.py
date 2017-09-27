
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
GLOBAL_OPTIONS['working_directory'] = "/home/user/study"

STUDY_OPTIONS = {}
STUDY_OPTIONS['nb_parameters'] = 5
STUDY_OPTIONS['sampling_size'] = 10
STUDY_OPTIONS['nb_time_steps'] = 100
STUDY_OPTIONS['threshold_value'] = 0.7
STUDY_OPTIONS['field_names'] = ["field1", "field2"]

SERVER_OPTIONS = {}
SERVER_OPTIONS['nb_proc'] = 3
SERVER_OPTIONS['path'] = "/home/user/Melissa/build/server"
SERVER_OPTIONS['mpi_options'] = ""
SERVER_OPTIONS['timeout'] = 600

SIMULATIONS_OPTIONS = {}
SIMULATIONS_OPTIONS['coupling'] = True
SIMULATIONS_OPTIONS['mpi_options'] = ""
SIMULATIONS_OPTIONS['timeout'] = 300

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
USER_FUNCTIONS['create_simulation'] = None
USER_FUNCTIONS['create_group'] = None
USER_FUNCTIONS['launch_server'] = None
USER_FUNCTIONS['launch_simulation'] = None
USER_FUNCTIONS['check_server_job'] = None
USER_FUNCTIONS['check_simulation_job'] = None
USER_FUNCTIONS['cancel_job'] = None
USER_FUNCTIONS['restart_server'] = None
USER_FUNCTIONS['restart_simulation'] = None
USER_FUNCTIONS['check_scheduler_load'] = None
USER_FUNCTIONS['postprocessing'] = None
USER_FUNCTIONS['finalize'] = None
