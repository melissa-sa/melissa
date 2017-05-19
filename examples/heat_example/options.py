
import numpy as np

# ------------- options ------------- #

nb_parameters = 5
sampling_size = 100
nb_time_steps = 1
#operations = ["mean","variance","min","max","threshold","sobol"]
operations = ["mean","variance","min","max","threshold", "quantile"]
threshold = 0.7
mpi_options = ""
nb_proc_simu = 2
nb_proc_server = 2
server_path = "../../server"
range_min = np.zeros(nb_parameters)
range_max = np.zeros(nb_parameters)
range_min[0] = 0
range_max[0] = 1
range_min[1] = 0
range_max[1] = 1
range_min[2] = 0
range_max[2] = 1
range_min[3] = 0
range_max[3] = 1
range_min[4] = 0
range_max[4] = 1
coupling = 1
