
import numpy as np

# ------------- options ------------- #

nb_parameters = 5
sampling_size = 1000
nb_time_steps = 1
#operations = ["mean","variance","min","max","threshold"]
operations = ["mean","variance","sobol"]
threshold = 0.7
mpi_options = ""
nb_proc_simu = 1
nb_proc_server = 1
server_path = "../../server"
range_min = np.zeros(nb_parameters)
range_max = np.zeros(nb_parameters)
for i in range(nb_parameters):
  range_min[i] =  0
  range_max[i] =  1
coupling = 1
