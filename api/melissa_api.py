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

# -*- coding: utf-8 -*-

"""
    Python wraper for Melissa API.
"""

import numpy as np
import ctypes
from mpi4py import MPI

melissa_c_api = np.ctypeslib.load_library('libmelissa_api',os.path.join(os.path.dirname(__file__),"libmelissa_api.so"))

# C prototypes
c_char_ptr = ctypes.POINTER(ctypes.c_char)
c_void_ptr = ctypes.c_void_p
c_double_ptr = ctypes.POINTER(ctypes.c_double)
c_int_ptr = ctypes.POINTER(ctypes.c_int)

melissa_c_api.melissa_init_f.argtypes = (c_char_ptr,
                                         c_int_ptr,
                                         c_int_ptr)

melissa_c_api.melissa_send.argtypes = (c_char_ptr,
                                       c_double_ptr)

melissa_c_api.melissa_finalize.argtypes = ()

def melissa_init(field_name,
                 local_vect_size,
                 comm,
                 coupling):
    comm_f = comm.py2f()
    buff = ctypes.create_string_buffer(len(field_name)+1)
    buff.value = field_name.encode()
    melissa_c_api.melissa_init_f(buff,
                                 ctypes.byref(ctypes.c_int(local_vect_size)),
                                 ctypes.byref(ctypes.c_int(comm_f)))

def melissa_send(field_name,
                 send_vect):
    buff = ctypes.create_string_buffer(len(field_name)+1)
    buff.value = field_name.encode()
    array = (ctypes.c_double * len(send_vect))(*send_vect)
    melissa_c_api.melissa_send(buff,
                               array)

def melissa_finalize():
    melissa_c_api.melissa_finalize()
