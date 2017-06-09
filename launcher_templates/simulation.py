
from options import *

class Simulation:

    nb_simulations = 0
    def __init__(self, parameter_set, options):
        self.options = options
        self.status = 0
        self.id = Simulation.nb_simulations
        Simulation.nb_simulations += 1
        self.sobol_id = sobol_id
        self.parameter_set = parameter_set
        self.nb_restarts = 0
        self.job_id = 0

    def create_simulation(self):
        if self.options.create_simulation != None:
            return self.options.create_simulation()
        else:
            pass

    def launch_simulation(self):
        if self.options.launch_simulation != None:
            return self.options.launch_simulation()
        else:
            pass

    def check_simulation_job(self):
        if self.options.check_simulation_job != None:
            return self.options.check_simulation_job()
        else:
            pass

    def check_simulation_timeout(self):
        if self.options.check_simulation_timeout != None:
            return self.options.check_simulation_timeout()
        else:
            pass
