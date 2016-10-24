import os
import time
import sys
import numpy as np
import numpy.random as rd

# ------------- options ------------- #

nb_parameters = 3
nb_simu = 1000
nb_groups = 1000
nb_time_steps = 30
operations = ["mean","variance","min","max","threshold","sobol"]
operations_list = operations.split()
threshold = 1000
op_str=""
mpi_options = ""
nb_procs = 2
server_path = "/home/tterraz/avido/source/Melissa/build/src"
range_min = np.zeros(nb_groups)
range_max = np.ones(nb_groups)

# ------------- functions ------------- #

def create_matrix(nb_parameters, nb_groups, range_min, range_max):
  A = np.zeros((nb_groups, nb_parameters))
  for i in range(nb_parameters):
    A[:,i] = rd.uniform(range_min[i], range_max[i], nb_groups)
  return A

def create_matrix_k(A, B, k):
  Ck = A
  Ck[:,k] = B[:,k]
  return Ck

def launch_simu (Ai, param)
  os.system("cd /home/tterraz/avido/source/Melissa/build/examples/heat_example")
  os.system("mpirun -n 3 ./heatc "+str(Ai[0])+" "+param)

def launch_melissa (command_line)
  os.system("cd /home/tterraz/avido/source/Melissa/build/examples/heat_example")
  os.system("mkdir resu")
  os.system("cd resu")
  os.system(command_line)

# ------------- main ------------- #

A = create_matrix(nb_parameters, nb_groups, range_min, range_max)
B = create_matrix(nb_parameters, nb_groups, range_min, range_max)
C = [create_matrix_k(A, B, i) for i in range(nb_parameters)]

ret = np.zeros(nb_parameters + 2)
for i in range(nb_groups):
  ret[0] = launch_simu(A[i,:], "0:"+str(i))
  if ("sobol" in operations) or ("sobol_indices" in operations):
    ret[1] = launch_simu(B[i,:], "1:"+str(i))
    for j in range(nb_parameters):
      ret[j+2] = launch_simu(C[j+2][i,:], str(j+2)":"+str(i))
  for k in range(len(ret)):
    if (ret[k] != 0):
      print "error launching simulation "+str(i)+" of group "+str(k)
  
for i in range(len(operations)):
  if i < len(operations) - 1:
    op_str += operations[i] + ":"
  else:
    op_str += operations[i]

if ("sobol" in operations) or ("sobol_indices" in operations):
  parameters = str(nb_parameters) + ":" + str(nb_groups)
else:
  parameters = str(nb_parameters) + ":" + str(nb_simu)

options = " -p " + parameters\
        + " -t " + str(nb_time_steps)\
        + " -o " + op_str\
        + " -e " + str(threshold)

launch_melissa("mpirun "+mpi_options+" -n "+nb_procs+" "+server_path+"/server"+options+" &")
