import numpy as np

class Options:
    def __init__(self):
        # user #
        self.home_path                = "/scratch/G95757"
        self.user_name                = "toto"
        # parameter sets #
        self.nb_parameters            = 6
        self.range_min_param          = np.zeros(self.nb_parameters,float)
        self.range_max_param          = np.ones(self.nb_parameters,float)
        self.sampling_size            = 1000
        # operations #
        self.mean                     = True
        self.variance                 = True
        self.min                      = True
        self.max                      = True
        self.threshold_exceedance     = True
        self.threshold                = 0.7
        self.quantile                 = True
        self.sobol_indices            = True
        # Melissa Server #
        self.nb_time_steps            = 100
        self.server_path              = self.home_path+"/Melissa/build/server"
        self.mpi_options              = ""
        # functions #
        # Functions must only depend on self.
        self.create_study             = None
        self.draw_parameter           = np.random.uniform
        self.create_simulation        = None
        self.launch_simulation        = None
        self.check_simulation_job     = None
        self.check_simulation_timeout = None
        self.check_server_job         = None
        self.check_server_timeout     = None

        # add all the options you need.
