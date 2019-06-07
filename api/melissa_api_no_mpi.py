# -*- coding: utf-8 -*-

###################################################################
#                            Melissa                              #
#-----------------------------------------------------------------#
#   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    #
#                                                                 #
# This source is covered by the BSD 3-Clause License.             #
# Refer to the  LICENCE file for further information.             #
#                                                                 #
#-----------------------------------------------------------------#
#  Original Contributors:                                         #
#    Theophile Terraz,                                            #
#    Bruno Raffin,                                                #
#    Alejandro Ribes,                                             #
#    Bertrand Iooss,                                              #
###################################################################

import ctypes
import numpy as np
import os

c_melissa_api_no_mpi = np.ctypeslib.load_library('libmelissa_api',os.path.join(os.path.dirname(__file__),"libmelissa_api.so"))

# C prototypes
c_char_ptr = ctypes.POINTER(ctypes.c_char)
c_double_ptr = ctypes.POINTER(ctypes.c_double)

c_melissa_api_no_mpi.melissa_init_no_mpi.argtypes = (c_char_ptr,   #field_name
                                                     ctypes.c_int, #vect_size
                                                     ctypes.c_int, #simu_id
                                                     ctypes.c_int) #coupling


c_melissa_api_no_mpi.melissa_send_no_mpi.argtypes = (c_char_ptr,   #field_name
                                                     c_double_ptr) #send_vect

c_melissa_api_no_mpi.melissa_finalize.argtypes = ()

def melissa_init(field_name, vect_size, simu_id, coupling):
    buff = ctypes.create_string_buffer(len(field_name)+1)
    buff.value = field_name.encode()
    c_melissa_api_no_mpi.melissa_init_no_mpi(
        buff,
        ctypes.c_int(vect_size),
        ctypes.c_int(simu_id),
        ctypes.c_int(coupling)
    )
    pass

def melissa_send(field_name, send_vect):
    buff = ctypes.create_string_buffer(len(field_name)+1)
    buff.value = field_name.encode()
    array = (ctypes.c_double * len(send_vect))(*send_vect)
    c_melissa_api_no_mpi.melissa_send(buff,array)
    pass

def melissa_finalize():
    c_melissa_api_no_mpi.melissa_finalize()
