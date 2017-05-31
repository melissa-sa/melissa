
import numpy as np

# ------------- options ------------- #

nb_parameters = 3
sampling_size = 6
nb_time_steps = 1
#operations = ["mean","variance","min","max","threshold","sobol"]
operations = ["mean","variance","min","max","threshold", "quantile","sobol"]
threshold = 0.7
mpi_options = ""
nb_proc_simu = 2
nb_proc_server = 3
server_path = "../../server"
range_min = np.zeros(nb_parameters)
range_max = np.zeros(nb_parameters)
for i in range(nb_parameters):
  range_min[i] = 0
  range_max[i] = 1
coupling = 1
