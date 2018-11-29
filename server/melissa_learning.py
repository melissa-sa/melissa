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

#from __future__ import print_function
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

class melissa_helper_tf:
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

def model_init_tf(vect_size, nb_parameters):
    # Parameters
    helper = melissa_helper(nb_parameters)
    learning_rate = 0.1

    # Network Parameters
    n_hidden_1 = vect_size*2 # 1st layer number of neurons
    n_hidden_2 = vect_size*3 # 2nd layer number of neurons
    n_hidden_3 = vect_size*4 # 3nd layer number of neurons
    n_hidden_4 = vect_size*3 # 4nd layer number of neurons
    n_hidden_5 = vect_size*2 # 5nd layer number of neurons
    n_hidden_6 = vect_size # 6nd layer number of neurons
    num_input = nb_parameters # data input
    num_output = vect_size # total output

    # tf Graph input
    X = tf.placeholder("float", [None, num_input])
    Y = tf.placeholder("float", [None, num_output])

    # Store layers weight & bias
    weights = {
        'h1': tf.Variable(tf.random_normal([num_input, n_hidden_1])),
        'h2': tf.Variable(tf.random_normal([n_hidden_1, n_hidden_2])),
        'h3': tf.Variable(tf.random_normal([n_hidden_2, n_hidden_3])),
        'h4': tf.Variable(tf.random_normal([n_hidden_3, n_hidden_4])),
        'h5': tf.Variable(tf.random_normal([n_hidden_4, n_hidden_5])),
        'h6': tf.Variable(tf.random_normal([n_hidden_5, n_hidden_6])),
        'out': tf.Variable(tf.random_normal([n_hidden_6, num_output]))
    }
    biases = {
        'b1': tf.Variable(tf.random_normal([n_hidden_1])),
        'b2': tf.Variable(tf.random_normal([n_hidden_2])),
        'b3': tf.Variable(tf.random_normal([n_hidden_3])),
        'b4': tf.Variable(tf.random_normal([n_hidden_4])),
        'b5': tf.Variable(tf.random_normal([n_hidden_5])),
        'b6': tf.Variable(tf.random_normal([n_hidden_6])),
        'out': tf.Variable(tf.random_normal([num_classes]))
    }
    # Hidden fully connected layer
    layer_1 = tf.add(tf.matmul(x, weights['h1']), biases['b1'])
    # Hidden fully connected layer
    layer_2 = tf.add(tf.matmul(layer_1, weights['h2']), biases['b2'])
    # Hidden fully connected layer
    layer_3 = tf.add(tf.matmul(layer_2, weights['h3']), biases['b3'])
    # Hidden fully connected layer
    layer_4 = tf.add(tf.matmul(layer_3, weights['h4']), biases['b4'])
    # Hidden fully connected layer
    layer_5 = tf.add(tf.matmul(layer_4, weights['h5']), biases['b5'])
    # Hidden fully connected layer
    layer_6 = tf.add(tf.matmul(layer_5, weights['h6']), biases['b6'])
    # Output fully connected layer
    out_layer = tf.matmul(layer_6, weights['out']) + biases['out']

    # Define loss and optimizer
    loss_op = tf.reduce_mean(tf.nn.softmax_cross_entropy_with_logits(
        logits=out_layer, labels=Y))
    optimizer = tf.train.AdamOptimizer(learning_rate=learning_rate)
    train_op = optimizer.minimize(loss_op)

    # Evaluate model (with test logits, for dropout to be disabled)
    correct_pred = tf.equal(tf.argmax(logits, 1), tf.argmax(Y, 1))
    accuracy = tf.reduce_mean(tf.cast(correct_pred, tf.float32))

    # Initialize the variables (i.e. assign their default value)
    init = tf.global_variables_initializer()

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

def read_model(dirname, filename):
    model=load_model(dirname+"/"+filename)
    MODEL.append(model)

def read_model_minibatch(dirname, filename):
    if hvd.rank() == 0:
        read_model(dirname, filename)
    # Horovod: adjust learning rate based on number of GPUs.
    opt = keras.optimizers.Adam()
    # Horovod: add Horovod Distributed Optimizer.
    opt = hvd.DistributedOptimizer(opt)

    hvd.broadcast_global_variables(0)

def read_model_parallel(dirname, filename):
    read_model(dirname+"/rank"+str(MPI.COMM_WORLD.Get_rank()),filename)

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



