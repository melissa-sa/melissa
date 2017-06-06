
import numpy as np

# ------------- options ------------- #

nb_parameters = 5
sampling_size = 20
nb_time_steps = 1
#operations = ["mean","variance","min","max","threshold"]
#operations = ["mean","variance","min","max","threshold", "quantile","sobol"]
operations = ["mean","sobol"]
threshold = 0.7
mpi_options = ""
nb_proc_simu = 1
nb_proc_server = 1
server_path = "../../server"
range_min = np.zeros(nb_parameters)
range_max = np.zeros(nb_parameters)
for i in range(nb_parameters):
  range_min[i] =  0 + i
  range_max[i] =  1 + i
coupling = 1
