
import os
import time
import sys
import signal
import numpy as np
import numpy.random as rd
import socket
import imp
from threading import Thread, RLock
from ctypes import cdll, create_string_buffer
imp.load_source("options", "./options.py")
from options import *
get_message = cdll.LoadLibrary(server_path+"/../master/libget_message.so")
signal.signal(signal.SIGINT, signal.SIG_DFL)

# ------------- thread ------------- #

class message_reciever(Thread):
    def __init__(self):
        Thread.__init__(self)
    def run(self):
        while True:
            message = create_string_buffer('\000' * 256)
            get_message.wait_message(message)
            print "message: "+message.value
            if message.value == "stop":
                return 0

# ------------- functions ------------- #

def create_matrix(nb_parameters, nb_groups, range_min, range_max):
  A = np.zeros((nb_groups, nb_parameters))
  for i in range(nb_parameters):
      for j in range(nb_groups):
          A[j,i] = rd.uniform(range_min[i], range_max[i])
  return A

def create_matrix_k(A, B, k):
  Ck = np.array(A)
  Ck[:,k] = np.array(B[:,k])
  return Ck

def launch_simu (Ai, sobol_rank, sobol_group, nb_proc_simu, nb_parameters):
#  os.system("cd /home/tterraz/avido/source/Melissa/build/examples/heat_example")
  os.system("export OMP_NUM_THREADS=2")
  parameters = ""
  for i in Ai:
      parameters += str(i) + " "
  parameters += str(sobol_rank) + " "
  parameters += str(sobol_group)
  print "mpirun -n "+str(nb_proc_simu)+" ./heatc "+parameters+" &"
  return os.system("mpirun -n "+str(nb_proc_simu)+" ./heatc "+parameters+" &")

def launch_coupled_simu (Ai, Bi, C, sobol_group, nb_proc_simu, nb_parameters):
#  os.system("cd /home/tterraz/avido/source/Melissa/build/examples/heat_example")
  os.system("export OMP_NUM_THREADS=2")
  command = "mpirun"
  parameters = ""
  for j in Ai:
      parameters += str(j) + " "
  parameters += str(0) + " "
  parameters += str(sobol_group)
  command += " -n "+str(nb_proc_simu)+" ./heatc "+parameters
  parameters = ""
  for j in Bi:
      parameters += str(j) + " "
  parameters += str(1) + " "
  parameters += str(sobol_group)
  command += " :"
  command += " -n "+str(nb_proc_simu)+" ./heatc "+parameters
  for j in range(nb_parameters):
      parameters = ""
      for i in C[j][sobol_group,:]:
          parameters += str(i) + " "
      parameters += str(j+2) + " "
      parameters += str(sobol_group)
      command += " :"
      command += " -n "+str(nb_proc_simu)+" ./heatc "+parameters
  print command
  return os.system(command+" &")

def launch_melissa (command_line):
#  os.system("cd /home/tterraz/avido/source/Melissa/build/examples/heat_example")
  os.system("export OMP_NUM_THREADS=2")
  if (not os.path.isdir("resu")):
      os.system("mkdir resu")
  os.system("cd resu")
  return os.system(command_line)

def launch_heatc(nb_parameters,
                 nb_groups,
                 nb_time_steps,
                 operations,
                 threshold,
                 mpi_options,
                 nb_proc_simu,
                 nb_proc_server,
                 server_path,
                 range_min,
                 range_max,
                 coupling):

    if (("sobol" in operations) or ("sobol_indices" in operations)):
        nb_simu = nb_groups * (nb_parameters + 2)
    A = create_matrix(nb_parameters, nb_groups, range_min, range_max)
    np.save("Amatrix", A)
    if ("sobol" in operations) or ("sobol_indices" in operations):
      B = create_matrix(nb_parameters, nb_groups, range_min, range_max)
      C = [create_matrix_k(A, B, i) for i in range(nb_parameters)]

      np.save("Bmatrix", B)
      for i in range(nb_parameters):
        np.save("C"+str(i)+"matrix", C[i])


    op_str=""
    for i in range(len(operations)):
      if (i < len(operations) - 1 and 1 < len(operations[i])):
        op_str += operations[i] + ":"
      else:
        op_str += operations[i]

    options = " -p " + str(nb_parameters)\
            + " -s " + str(nb_groups)\
            + " -t " + str(nb_time_steps)\
            + " -o " + op_str\
            + " -e " + str(threshold)\
            + " -n " + str(socket.gethostname())
    print "mpirun "+mpi_options+" -n "+str(nb_proc_server)+" "+server_path+"/server"+options
    if (launch_melissa("mpirun "+mpi_options+" -n "+str(nb_proc_server)+" "+server_path+"/server"+options+"&") != 0):
        print "error launching Melissa"
    #print "mpirun "+mpi_options+" -n "+str(nb_proc_server)+" "+server_path+"/server"+options+"&"
    #launch_melissa("valgrind --leak-check=full mpirun -n 1 ./server -p 2 -s 8 -g 5 -t 100 -o mean:variance:min:max:threshold:sobol -e 0.7")

    get_message.init_message()
    thread = message_reciever()
    thread.start()

    ret = np.zeros(nb_parameters + 2)
    for i in range(nb_groups/2):
      if ("sobol" in operations) or ("sobol_indices" in operations):
        if (coupling == 0):
          ret[0] = launch_simu(A[i,:], 0, i, nb_proc_simu, nb_parameters)
          ret[1] = launch_simu(B[i,:], 1, i, nb_proc_simu, nb_parameters)
          for j in range(nb_parameters):
            ret[j+2] = launch_simu(C[j][i,:], j+2, i, nb_proc_simu, nb_parameters)
          for k in range(len(ret)):
            if (ret[k] != 0):
              print "error launching simulation "+str(i)+" of group "+str(k)
        else:
          launch_coupled_simu(A[i,:], B[i,:], C, i, nb_proc_simu, nb_parameters)
      else:
        ret[0] = launch_simu(A[i,:], 0, i, nb_proc_simu, nb_parameters)
        if (ret[0] != 0):
          print "error launching simulation "+str(i)
      time.sleep(4)
    time.sleep(3)
    os.system("killall -s USR1 mpirun")
    time.sleep(5)
    print "server killed"
    print "restarting:"
    print "mpirun "+mpi_options+" -n "+str(nb_proc_server)+" "+server_path+"/server"+options+" -r ."
    if (launch_melissa("mpirun "+mpi_options+" -n "+str(nb_proc_server)+" "+server_path+"/server"+options+" -r . &") != 0):
        print "error launching Melissa"

    for i in range(nb_groups/2):
      if ("sobol" in operations) or ("sobol_indices" in operations):
        if (coupling == 0):
          ret[0] = launch_simu(A[i,:], 0, i+nb_groups/2, nb_proc_simu, nb_parameters)
          ret[1] = launch_simu(B[i,:], 1, i+nb_groups/2, nb_proc_simu, nb_parameters)
          for j in range(nb_parameters):
            ret[j+2] = launch_simu(C[j][i+nb_groups/2,:], j+2, i+nb_groups/2, nb_proc_simu, nb_parameters)
          for k in range(len(ret)):
            if (ret[k] != 0):
              print "error launching simulation "+str(i+nb_groups/2)+" of group "+str(k)
        else:
          launch_coupled_simu(A[i+nb_groups/2,:], B[i+nb_groups/2,:], C, i+nb_groups/2, nb_proc_simu, nb_parameters)
      else:
        ret[0] = launch_simu(A[i+nb_groups/2,:], 0, i+nb_groups/2, nb_proc_simu, nb_parameters)
        if (ret[0] != 0):
          print "error launching simulation "+str(i+nb_groups/2)
      time.sleep(4)


    if (("sobol" in operations) or ("sobol_indices" in operations)) and (pyzmq == 1):
        converged_sobol = np.zeros(nb_proc_server,int)
        finished_server = np.zeros(nb_proc_server,int)
        snd_message = "continue"
        while True:
            rcv_message = rep_socket.recv_string()
            message = int(rcv_message)
            print "rcv_message "+rcv_message+", message "+str(message)
            if (message >= 0 and message < nb_proc_server):
                rep_socket.send_string("ok")
                converged_sobol[message] = 1
                if (not 0 in converged_sobol):
                    snd_message = "stop"
            else:
                finished_server[message - nb_proc_server] = 1
                rep_socket.send_string(snd_message)
                if (not 0 in finished_server):
                    break

#       kill all simulations here
    thread.join()
    get_message.close_message()
    os.system("killall heatc")
    return 0

# ------------- main ------------- #

if __name__ == '__main__':
    launch_heatc(nb_parameters,
    sampling_size,
    nb_time_steps,
    operations,
    threshold,
    mpi_options,
    nb_proc_simu,
    nb_proc_server,
    server_path,
    range_min,
    range_max,
    coupling)

