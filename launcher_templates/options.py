
import os
import numpy as np
from matplotlib import pyplot as plt
from matplotlib import cm

class Options:
    def __init__(self):
        # user #
        self.home_path                = "/home/tterraz"
        self.user_name                = "tterarz"
        # parameter sets #
        self.nb_parameters            = 5
        self.range_min_param          = np.zeros(self.nb_parameters,float)
        self.range_max_param          = np.ones(self.nb_parameters,float)
        self.sampling_size            = 10
        self.nb_time_steps            = 1
        # operations #
        self.mean                     = True
        self.variance                 = True
        self.min                      = True
        self.max                      = True
        self.threshold_exceedance     = True
        self.threshold                = 0.7
        self.quantile                 = True
        self.sobol_indices            = False
        # Melissa Server #
        self.nb_proc_server           = 3
        self.server_path              = self.home_path+"/avido/source/Melissa/build/server"
        self.mpi_options              = ""
        # simulations #
        self.simulation_path          = self.home_path+"/avido/source/Melissa/build/example/heat_example"
        self.simulation_executable    = "heatc"
        self.nb_proc_simulation       = 2
        self.coupling                 = 1
        # functions #
        self.create_study             = None
        self.draw_parameter           = np.random.uniform
        self.create_simulation        = None
        self.launch_simulation        = None
        self.reboot_simulation        = None
        self.check_simulation_job     = None
        self.check_simulation_timeout = None
        self.launch_server            = None
        self.reboot_server            = None
        self.wait_server_start        = None
        self.check_server_job         = None
        self.check_server_timeout     = None
        self.stats_visu               = None
        self.finalize                 = None

        # add all the options you need.
