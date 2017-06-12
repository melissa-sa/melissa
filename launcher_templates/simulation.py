
import os
from options import *

class Simulation:

    nb_simulations = 0
    def __init__(self, parameter_set, options, sobol_id):
        self.options = options
        self.status = 0
        self.id = Simulation.nb_simulations
        self.sobol_id = sobol_id
        if sobol_id < 1:
            Simulation.nb_simulations += 1
        self.parameter_set = parameter_set
        self.nb_restarts = 0
        self.job_id = 0
