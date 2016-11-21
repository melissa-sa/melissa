import os
import time
import sys
import numpy as np
import numpy.random as rd
import zmq

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

def launch_simu (Ai, sobol_rank, sobol_group, nb_proc_server, nb_parameters):
  os.system("cd /home/tterraz/avido/source/Melissa/build/examples/heat_example")
  parameters = ""
  for i in Ai:
      parameters += str(i) + " "
  parameters += str(sobol_rank) + " "
  parameters += str(sobol_group)
#  print "mpirun -n "+str(nb_proc_server)+" ./heatc "+parameters+" &"
  return os.system("mpirun -n "+str(nb_proc_server)+" ./heatc "+parameters+" &")

def launch_melissa (command_line):
  os.system("cd /home/tterraz/avido/source/Melissa/build/examples/heat_example")
  if (not os.path.isdir("resu")):
      os.system("mkdir resu")
  os.system("cd resu")
  return os.system(command_line)

# ------------- options ------------- #

nb_parameters = 2
nb_simu = 8
nb_groups = 2
nb_time_steps = 100
operations = ["mean","variance","min","max","threshold","sobol"]
#operations_list = operations.split()
threshold = 0.7
op_str=""
mpi_options = ""
nb_proc_simu = 3
nb_proc_server = 2
server_path = "/home/tterraz/avido/source/Melissa/build/src"
range_min = np.zeros(nb_groups,float)
range_max = np.ones(nb_groups,float)

# ------------- main ------------- #

context = zmq.Context()
socket = context.socket(zmq.PULL)
socket.bind("tcp://*:5555")

if (not (("sobol" in operations) or ("sobol_indices" in operations))):
    nb_groups = nb_simu
A = create_matrix(nb_parameters, nb_groups, range_min, range_max)
if ("sobol" in operations) or ("sobol_indices" in operations):
  B = create_matrix(nb_parameters, nb_groups, range_min, range_max)
  C = [create_matrix_k(A, B, i) for i in range(nb_parameters)]

ret = np.zeros(nb_parameters + 2)
for i in range(nb_groups):
  ret[0] = launch_simu(A[i,:], 0, i, nb_proc_server, nb_parameters)
  if ("sobol" in operations) or ("sobol_indices" in operations):
    ret[1] = launch_simu(B[i,:], 1, i, nb_proc_server, nb_parameters)
    for j in range(nb_parameters):
      ret[j+2] = launch_simu(C[j][i,:], j+2, i, nb_proc_server, nb_parameters)
    for k in range(len(ret)):
      if (ret[k] != 0):
        print "error launching simulation "+str(i)+" of group "+str(k)
  else:
    if (ret[0] != 0):
      print "error launching simulation "+str(i)


for i in range(len(operations)):
  if i < len(operations) - 1:
    op_str += operations[i] + ":"
  else:
    op_str += operations[i]

options = " -p " + str(nb_parameters)\
        + " -s " + str(nb_simu)\
        + " -g " + str(nb_groups)\
        + " -t " + str(nb_time_steps)\
        + " -o " + op_str\
        + " -e " + str(threshold)

#print "mpirun "+mpi_options+" -n "+str(nb_proc_server)+" "+server_path+"/server"+options
if (launch_melissa("mpirun "+mpi_options+" -n "+str(nb_proc_server)+" "+server_path+"/server"+options+"&") != 0):
    print "error launching Melissa"

if (("sobol" in operations) or ("sobol_indices" in operations)):
    converged_sobol = np.zeros(nb_proc_server,int)
    while 0 in converged_sobol:
        message = socket.recv_string()
        converged_sobol[int(message)] = 1
#       kill all simulations here
