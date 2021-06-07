#!/usr/bin/python3

# Copyright (c) 2017, Institut National de Recherche en Informatique et en Automatique (https://www.inria.fr/)
#               2017, EDF (https://www.edf.fr/)
#               2020, 2021 Institut National de Recherche en Informatique et en Automatique (Inria)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import numpy as np

STUDY_OPTIONS = {}
MELISSA_STATS = {}

#  STATISTICS 

# Fields: name of the ouputs fields sent to Melissa server (on set of statitics are computed per field)
STUDY_OPTIONS['field_names'] = ["heat1"]
MELISSA_STATS['mean'] = True
MELISSA_STATS['variance'] = False
MELISSA_STATS['skewness'] = False
MELISSA_STATS['kurtosis'] = False
MELISSA_STATS['min'] = False
MELISSA_STATS['max'] = False
MELISSA_STATS['threshold_exceedance'] = False
STUDY_OPTIONS['threshold_values'] = [0.7, 0.8]
MELISSA_STATS['quantiles'] = False
STUDY_OPTIONS['quantile_values'] = [0.05, 0.25, 0.5, 0.75, 0.95]
MELISSA_STATS['sobol_indices'] = True

#  STUDY / PARAMETER SWEEP / SIMULATIONS

# Sampling function: called to set the parameter value for each simulation 
# For the heat example we have at least two and up to five parameters (initial temperatures for the 4 borders  + grid cells)
def draw_param_set():
    return np.random.uniform(0, 1, size=2)
USER_FUNCTIONS = {'draw_parameter_set': draw_param_set}

# Size of the parameter sweep  (= number of simulations to execute)
STUDY_OPTIONS['sampling_size'] = 6

# Number of timesteps Melissa is expected to receive (the simulation do not have to send all the computed timesteps)
STUDY_OPTIONS['nb_timesteps'] = 100


# SYSTEM

# verbosity (the default level is 2):
# * 0: show only errors
# * 1: show errors and warnings
# * 2: show errors, warnings, and useful information
# * 3: show errors, warnings, useful information, and debugging data
STUDY_OPTIONS['verbosity'] = 2

# Number of simulations started per client / scheduler batch. Option ignored when Sobol' indices are computed.
# For Sobol' indices the batch size is always  P+2 where P is the number of parameters 
# (constraint related to the pick-freeze method)
STUDY_OPTIONS['batch_size'] = 2

# Fault tolerance protocol control
# Timeouts (trigger the fault tolerance protocol when reached)
# A simulation/client is restarted after being silent for (seconds):
STUDY_OPTIONS['simulation_timeout'] = 400

# Server checkpoint interval (seconds)
STUDY_OPTIONS['checkpoint_interval'] = 300

# Method used to couple the simulation in a pick-freeze batch (specific to Sobol' Indices)
STUDY_OPTIONS['coupling'] = "MELISSA_COUPLING_MPI"

# Port numbers used  to connect launcher, clients and server through ZMQ. 
STUDY_OPTIONS['resp_port'] = 5546
STUDY_OPTIONS['recv_port'] = 5547
STUDY_OPTIONS['send_port'] = 5548


