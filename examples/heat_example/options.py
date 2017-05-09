
import numpy as np

# ------------- options ------------- #

nb_parameters = 2
sampling_size = 4
nb_time_steps = 100
operations = ["mean","variance","min","max","threshold","sobol"]
threshold = 0.7
mpi_options = ""
nb_proc_simu = 2
nb_proc_server = 3
server_path = "../../server"
range_min = np.zeros(nb_parameters)
range_max = np.zeros(nb_parameters)
range_min[0] = 0
range_max[0] = 1
range_min[1] = 2
range_max[1] = 3
coupling = 1
