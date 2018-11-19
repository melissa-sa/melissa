#!@PYTHON_EXECUTABLE@

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

# -*- coding: utf-8 -*-

"""
    Python sequential version of melissa_server.

    usage:
    python melissa_server.py <options>
"""

import shutil
import numpy as np
import tensorflow as tf
import timeit
import sys
import os
import os.path
import scipy as sp
import time
import random
import keras
from keras.models import Sequential
from keras.layers import Dense, Activation
from keras import backend as K
import horovod.keras as hvd
from mpi4py import MPI

MODEL = Sequential()
class melissa_helper:
    def __init__(self, nb_parameters):
        self.train_x = []
        self.train_y = []
        self.test_x = []
        self.test_y = []
        self.nb_parameters = nb_parameters
        for i in range(nb_parameters):
            self.train_x.append([])
            self.test_x.append([])


def model_init_minibatch(vect_size, nb_parameters):
    hvd.init()
    # Horovod: pin GPU to be used to process local rank (one GPU per process)
    config = tf.ConfigProto()
#    config.gpu_options.allow_growth = True
#    config.gpu_options.visible_device_list = str(hvd.local_rank())
    K.set_session(tf.Session(config=config))
    # creating the model
    print "vect_size = "+str(vect_size)
    print "nb_parameters = "+str(nb_parameters)
    MODEL.add(Dense(vect_size*2, input_dim=nb_parameters, activation='relu'))
    MODEL.add(Dense(vect_size*3, kernel_initializer='normal', activation='relu'))
    MODEL.add(Dense(vect_size*4, kernel_initializer='normal', activation='relu'))
    MODEL.add(Dense(vect_size*3, kernel_initializer='normal', activation='relu'))
    MODEL.add(Dense(vect_size*2, kernel_initializer='normal', activation='relu'))
    MODEL.add(Dense(vect_size, kernel_initializer='normal'))

    # Horovod: adjust learning rate based on number of GPUs.
    opt = keras.optimizers.Adam()
    # Horovod: add Horovod Distributed Optimizer.
    opt = hvd.DistributedOptimizer(opt)

    MODEL.compile(optimizer=opt, loss='mse', metrics=['mse'])

    hvd.broadcast_global_variables(0)

    return melissa_helper(nb_parameters)

def model_init_parallel(vect_size, nb_parameters):
    hvd.init()
    # creating the model
    print "vect_size = "+str(vect_size)
    print "nb_parameters = "+str(nb_parameters)
    MODEL.add(Dense(vect_size*2, input_dim=nb_parameters, activation='relu'))
    MODEL.add(Dense(vect_size*3, kernel_initializer='normal', activation='relu'))
    MODEL.add(Dense(vect_size*4, kernel_initializer='normal', activation='relu'))
    MODEL.add(Dense(vect_size*3, kernel_initializer='normal', activation='relu'))
    MODEL.add(Dense(vect_size*2, kernel_initializer='normal', activation='relu'))
    MODEL.add(Dense(vect_size, kernel_initializer='normal'))

    MODEL.compile(optimizer='adam', loss='mse', metrics=['mse'])

    return melissa_helper(nb_parameters)

def add_to_training_set(x, y, handle):
    handle.train_y.append(y)
    for i in range(handle.nb_parameters):
        handle.train_x[i].append(x[i])

def add_to_testing_set(x, y, handle):
    handle.test_y.append(np.array(y))
    for i in range(handle.nb_parameters):
        handle.test_x[i].append(x[i])

def train_batch(handle):
    for i in range (handle.nb_parameters):
        X_temp=np.array(handle.train_x[i])
        X_temp=np.expand_dims(X_temp, 1)
        if i>0 :
            X_train =np.concatenate((X_train,X_temp), axis=1)
        else:
            X_train = X_temp
    Y_train=np.array(handle.train_y)
    res = MODEL.train_on_batch(X_train,Y_train)
    handle.train_x = []
    for i in range(handle.nb_parameters):
        handle.train_x.append([])
    handle.train_y = []
    return res

def test_batch(handle):
    for i in range (handle.nb_parameters):
        X_temp=np.array(handle.test_x[i])
        X_temp=np.expand_dims(X_temp, 1)
        if i>0 :
            X_test =np.concatenate((X_test,X_temp), axis=1)
        else:
            X_test = X_temp
    Y_test=np.array(handle.test_y)
    res = MODEL.test_on_batch(X_test,Y_test)
    handle.test_x = []
    for i in range(handle.nb_parameters):
        handle.test_x.append([])
    handle.test_y = []
    return res

def save_model(dirname, filename):
    MODEL.save(dirname+"/"+filename)

def save_model_minibatch(dirname, filename):
    if hvd.rank() == 0:
        save_model(dirname, filename)
    print "Horovod size: "+str(hvd.size())
    print "Horovod local rank: "+str(hvd.local_rank())
    print "Horovod rank: "+str(hvd.rank())

def save_model_parallel(dirname, filename):
    if (not os.path.isdir(dirname+"/rank"+str(MPI.COMM_WORLD.Get_rank()))):
        os.mkdir(dirname+"/rank"+str(MPI.COMM_WORLD.Get_rank()))
    save_model(dirname+"/rank"+str(MPI.COMM_WORLD.Get_rank()),filename)



