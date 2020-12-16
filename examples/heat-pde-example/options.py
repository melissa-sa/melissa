#!/usr/bin/python3

import numpy as np


def draw_param_set():
    n = STUDY_OPTIONS['nb_parameters']
    return np.random.uniform(0, 1, size=n)


STUDY_OPTIONS = {}
# number of varying parameters of the study
STUDY_OPTIONS['nb_parameters'] = 5
# initial number of parameter sets
STUDY_OPTIONS['sampling_size'] = 6
# number of timesteps from Melissa's point of view
STUDY_OPTIONS['nb_timesteps'] = 100
STUDY_OPTIONS['threshold_values'] = [0.7, 0.8]
STUDY_OPTIONS['quantile_values'] = [0.05,0.25,0.5,0.75,0.95]
# list of field names
STUDY_OPTIONS['field_names'] = ["heat1"]
# simulations are restarted after this amount of seconds
STUDY_OPTIONS['simulation_timeout'] = 400
# server checkpoint interval in seconds
STUDY_OPTIONS['checkpoint_interval'] = 300
# option for Sobol' simulation groups coupling
STUDY_OPTIONS['coupling'] = "MELISSA_COUPLING_MPI"
# verbosity (the default level is 2):
# * 0: show only errors
# * 1: show errors and warnings
# * 2: show errors, warnings, and useful information
# * 3: show errors, warnings, useful information, and debugging data
STUDY_OPTIONS['verbosity'] = 2

STUDY_OPTIONS['batch_size'] = 2
STUDY_OPTIONS['resp_port'] = 5546
STUDY_OPTIONS['recv_port'] = 5547
STUDY_OPTIONS['send_port'] = 5548

MELISSA_STATS = {}
MELISSA_STATS['mean'] = True
MELISSA_STATS['variance'] = False
MELISSA_STATS['skewness'] = False
MELISSA_STATS['kurtosis'] = False
MELISSA_STATS['min'] = True
MELISSA_STATS['max'] = True
MELISSA_STATS['threshold_exceedance'] = True
MELISSA_STATS['quantiles'] = True
MELISSA_STATS['sobol_indices'] = True

USER_FUNCTIONS = { 'draw_parameter_set': draw_param_set }
