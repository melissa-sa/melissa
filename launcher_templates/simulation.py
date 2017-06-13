
import os
from options import *

class Simulation:
    nb_simulations = 0
    def __init__(self, parameter_set, options, sobol_id):
        self.options = options
        self.status = 0
        self.id = Simulation.nb_simulations
        self.sobol_id = sobol_id
        Simulation.nb_simulations += 1
        self.parameter_set = np.copy(parameter_set)
        self.nb_restarts = 0
        self.job_id = 0

class Sobol_group:
    nb_groups = 0
    def __init__(self, parameter_set_A, parameter_set_B, options):
        self.options = options
        self.status = 0
        self.id = Sobol_group.nb_groups
        self.simulations = list()
        Sobol_group.nb_groups += 1
        self.parameter_set_A = np.copy(parameter_set_A)
        self.simulations.append(Simulation(parameter_set=parameter_set_A, options=self.options, sobol_id=0))
        self.parameter_set_B = np.copy(parameter_set_B)
        self.simulations.append(Simulation(parameter_set=parameter_set_A, options=self.options, sobol_id=1))
        for i in range(options.nb_parameters):
            temp_parameter_set = np.copy(parameter_set_A)
            temp_parameter_set[i] = np.copy(parameter_set_B[i])
            self.simulations.append(Simulation(parameter_set=temp_parameter_set, options=self.options, sobol_id=i+2))
        self.nb_restarts = 0
        self.job_id = 0

