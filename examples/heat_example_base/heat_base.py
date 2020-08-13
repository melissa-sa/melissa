#!@PYTHON_EXECUTABLE@

###################################################################
#                            Melissa                              #
#-----------------------------------------------------------------#
#   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    #
#                                                                 #
# This source is covered by the BSD 3-Clause License.             #
# Refer to the  LICENSE file for further information.             #
#                                                                 #
#-----------------------------------------------------------------#
#  Original Contributors:                                         #
#    Theophile Terraz,                                            #
#    Bruno Raffin,                                                #
#    Alejandro Ribes,                                             #
#    Bertrand Iooss,                                              #
###################################################################


"""
    heat example in Python
"""

import sys
import time
import numpy as np
from ctypes import c_double, c_int, c_char, byref
from mpi4py import MPI

heat_utils = np.ctypeslib.load_library('libheat_utils','@CMAKE_INSTALL_PREFIX@/share/melissa/examples/heat_example_base/lib/libheat_utils.so')
A_array = c_double * 3
A = A_array()

# The program takes at least one parameter: the initial temperature
narg = len(sys.argv)
if (narg < 2):
    print("Missing parameter")
    exit()
# The initial temperature is stored in param[0]
# The four next optional parameters are the boundary temperatures
param_array = c_double * 5
pyparam = []
for i in range(5):
    if (narg > i+1):
        pyparam.append(float(sys.argv[i+1]))
    else:
        pyparam.append(0)
param = param_array(pyparam[0],pyparam[1],pyparam[2],pyparam[3],pyparam[4])
temp = c_double(pyparam[0])

# The MPI communicator, process rank and communicator size
comm = MPI.COMM_WORLD.Dup()
me = c_int(comm.Get_rank())
np = c_int(comm.Get_size())
i1 = c_int(0)
iN = c_int(0)

# Init timer
t1 = time.time()

# Neighbour ranks
next = c_int(me.value+1)
previous = c_int(me.value-1)

if (next.value == np.value):
    next = c_int(MPI.PROC_NULL)
if (previous.value == -1):
    previous = c_int(MPI.PROC_NULL)

nx        = c_int(100) # x axis grid subdivisions
ny        = c_int(100) # y axis grid subdivisions
lx        = c_double(10.0) # x length
ly        = c_double(10.0) # y length
d         = c_double(1.0) # diffusion coefficient
t         = c_double(0.0)
dt        = c_double(0.01) # timestep value
nmax      = c_int(100) # number of timesteps
dx        = c_double(lx.value/(nx.value+1)) # x axis step
dy        = c_double(ly.value/(ny.value+1)) # y axis step
epsilon   = c_double(0.0001) # conjugated gradient precision
n         = c_int(nx.value*ny.value) # number of cells in the drid

# work repartition over the MPI processes
# i1 and in: first and last global cell indices atributed to this process
heat_utils.load(byref(me), byref(n), byref(np), byref(i1), byref(iN))

# local number of cells
vect_size = iN.value-i1.value+1

# initialization
heat_array = c_double * vect_size
U = heat_array()
F = heat_array()
# we will solve Au=F
heat_utils.init(U,
                byref(i1),
                byref(iN),
                byref(dx),
                byref(dy),
                byref(nx),
                byref(lx),
                byref(ly),
                byref(temp))
# init A (tridiagonal matrix):
heat_utils.filling_A(byref(d),
                     byref(dx),
                     byref(dy),
                     byref(dt),
                     byref(nx),
                     byref(ny),
                     A) # fill A

# main loop:
for i in range(nmax.value):
    t = c_double(t.value + dt.value)
    # filling F (RHS) before each iteration:
    heat_utils.filling_F(byref(nx),
                         byref(ny),
                         U,
                         byref(d),
                         byref(dx),
                         byref(dy),
                         byref(dt),
                         byref(t),
                         F,
                         byref(i1),
                         byref(iN),
                         byref(lx),
                         byref(ly),
                         param)
    # conjugated gradient to solve Au = F.
    heat_utils.conjgrad(A,
                        F,
                        U,
                        byref(nx),
                        byref(ny),
                        byref(epsilon),
                        byref(i1),
                        byref(iN),
                        byref(np),
                        byref(me),
                        byref(next),
                        byref(previous),
                        byref(c_int(comm.py2f())))
    # The result is U
    print('Time step: ', t.value)


# write results on disk
heat_utils.finalize(byref(dx),
                    byref(dy),
                    byref(nx),
                    byref(ny),
                    byref(i1),
                    byref(iN),
                    U,
                    F,
                    byref(me),
                    byref(c_int(0)))
# end timer
t2 = time.time()
print("Calcul time: "+str(t2-t1)+" sec\n");
