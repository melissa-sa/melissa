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
#import scipy as sp
import time
import random
import keras
import MEDLoader as ml
import MEDCoupling as mc
import matplotlib.pyplot as plt
#from keras.models import Sequential
#from keras.layers import Dense, Activation
#from keras import backend as K
#import horovod.tensorflow as hvd
from mpi4py import MPI

def rescale_params_fluid(param_set):
    param_set[0] = param_set[0]/300
    param_set[-1] = param_set[-1]/75
    return param_set

def rescale_params_heat(param_set):
#    param_set[-1] = float(param_set[-1])/100.
    param_set[-1] = 0.
#    print " TIMESTEP " + str(param_set[-1])
    return param_set

def rescale_params(param_set):
#    return rescale_params_heat(param_set)
    return param_set

def rescale_data_fluid(data):
    for i in data:
        i = i/500
    return data

def rescale_data(data):
    return data


def neighbors_MED():
    indices = []
    mesh = ml.ReadUMeshFromFile("/home/tterraz/Documents/deep_learning_subashuny/final_output/von_karman/MESH/COARSE/Cylinder_7361.med","Cylinder_1",0)
    tab1, tab2 = mesh.computeNeighborsOfCells()
    for i in range(len(tab2)-1): # cell
        diag_inserted = False
        for k in tab1[tab2[i]:tab2[i+1]]: # neighbors
            if k[0] > i and not diag_inserted:
                indices.append([i,i])
                diag_inserted = True
            indices.append([i,k[0]])
        if not diag_inserted:
            indices.append([i,i])
    return indices

def neighbors_MED_csr():
    indices = []
    mesh = ml.ReadUMeshFromFile("/home/tterraz/Documents/deep_learning_subashuny/final_output/von_karman/MESH/COARSE/Cylinder_7361.med","Cylinder_1",0)
    tab1, tab2 = mesh.computeNeighborsOfCells()
    for i in range(len(tab2)-1): # cell
        indices.append([])
        diag_inserted = False
        for k in tab1[tab2[i]:tab2[i+1]]: # neighbors
            print "k = "+str(k)
            if k[0] > i and not diag_inserted:
                indices[i].append(i)
                diag_inserted = True
            indices[i].append(k[0])
        if not diag_inserted:
            indices[i].append(i)
    return indices

def neighbors_read():
    indices = [[0, 0]]
    file = open("VKindices", "r")
    for line in file:
        indices.append([int(i) for i in line.split()])
    file.close()
    return indices

def neighbors_write(indices):
    file = open("VKindices", "w")
    for i in indices:
        file.write(str(i[0])+" "+str(i[1])+"\n")
    file.close()

def neighbors_square_sparse_tensor(size):
    x = int(np.sqrt(size))
    y = int(np.sqrt(size))
    indices = []
    for i in range(size):
        k = i%x # col
        l = np.floor(i/x) # row
        if l > 0:
            indices.append([i,i-x])
        if k > 0:
            indices.append([i,i-1])
        indices.append([i,i])
        if k < x-1:
            indices.append([i,i+1])
        if l < y-1:
            indices.append([i,i+x])
    return indices

def neighbors_square_csr(size):
    x = int(np.sqrt(size))
    y = int(np.sqrt(size))
    indices = []
    for i in range(size):
        indices.append([])
        k = i%x # col
        l = np.floor(i/x) # row
        if l > 0:
            indices[i].append(i-x)
        if k > 0:
            indices[i].append(i-1)
        indices[i].append(i)
        if k < x-1:
            indices[i].append(i+1)
        if l < y-1:
            indices[i].append(i+x)
    return indices

class MyFirstLayer(tf.keras.layers.Layer):
    def __init__(self, in_size, out_size, trainable=True):
        super(MyFirstLayer, self).__init__()
        self.in_size = in_size
        self.out_size = out_size
#        self.values = self.add_variable("values",
#                                         shape=[self.in_size, 1, self.out_size],
#                                         dtype=tf.float32,
#                                         initializer=tf.keras.initializers.RandomUniform,
#                                         trainable=True)

        self.biases = self.add_variable("biases",
                                        shape=[self.out_size],
                                        dtype=tf.float32,
                                        initializer=tf.keras.initializers.RandomUniform,
                                        #initializer=tf.keras.initializers.Zeros,
                                        trainable=True)

        self.coefs = self.add_variable("coefs",
                                       shape=[self.in_size, self.out_size],
                                       dtype=tf.float32,
                                       initializer=tf.keras.initializers.RandomUniform,
                                       #initializer=tf.keras.initializers.Zeros,
                                       trainable=True)
        self.act = tf.keras.layers.PReLU()
#        self.conv = tf.keras.layers.Conv1D(out_size, 1, input_shape=[None, in_size, 1])

    def compute_output_shape(self, input_shape):
        return (input_shape[0], self.out_size)

    def call(self, input):
#        res = tf.reshape(input, [self.in_size,1,1])
#        print res.shape
#        out = tf.matmul(res, self.values)
#        print out.shape
#        out = tf.reshape(out, [self.in_size, self.out_size])
#        print out.shape
        input = tf.expand_dims(input, -1)
        print "input_shape "+str(input.shape)
#        out = self.conv (input)
        multiples = [1, 1, self.out_size]
        out = tf.multiply(tf.tile (input, multiples), self.coefs)
        print out.shape
        out = tf.reduce_sum(out, axis = [1])
        print "output_shape "+str(out.shape)
        return self.act(tf.add(out, self.biases))

#class MyMidLayer(tf.keras.layers.Layer):
#    def __init__(self, size, trainable=True):
#        super(MyMidLayer, self).__init__()
#        self.size = size
##        self.values = self.add_variable("values",
##                                         shape=[self.in_size, 1, self.out_size],
##                                         dtype=tf.float32,
##                                         initializer=tf.keras.initializers.RandomUniform,
##                                         trainable=True)

#        self.biases = self.add_variable("biases",
#                                        shape=[self.out_size],
#                                        dtype=tf.float32,
#                                        initializer=tf.keras.initializers.RandomUniform,
#                                        #initializer=tf.keras.initializers.Zeros,
#                                        trainable=True)

#        self.coefs = self.add_variable("coefs",
#                                       shape=[self.in_size, self.out_size],
#                                       dtype=tf.float32,
#                                       initializer=tf.keras.initializers.RandomUniform,
#                                       #initializer=tf.keras.initializers.Zeros,
#                                       trainable=True)
#        self.act = tf.keras.layers.ReLU()
##        self.conv = tf.keras.layers.Conv1D(out_size, 1, input_shape=[None, in_size, 1])

#    def compute_output_shape(self, input_shape):
#        return (input_shape[0], self.size)

#    def call(self, input):
##        res = tf.reshape(input, [self.in_size,1,1])
##        print res.shape
##        out = tf.matmul(res, self.values)
##        print out.shape
##        out = tf.reshape(out, [self.in_size, self.out_size])
##        print out.shape
#        input = tf.expand_dims(input, -1)
#        print "input_shape "+str(input.shape)
##        out = self.conv (input)
#        multiples = [1, 1, 10]
#        out = tf.multiply(tf.tile (input, multiples), self.coefs)
#        print out.shape
#        out = tf.reduce_sum(out, axis = [1])
#        print "output_shape "+str(out.shape)
#        return self.act(tf.add(out, self.biases))



class MySparseLayerSparseTensor(tf.keras.layers.Layer):
    def __init__(self, size, trainable=True):
        super(MySparseLayerSparseTensor, self).__init__()
        self.size = size
        self.indices = neighbors_square_sparse_tensor(size)
#        self.indices = neighbors_MED()
        #print "value shape: " + str(self.sparse_tensor.values.shape)
        #print "indices: " + str(self.sparse_tensor.indices)
#        indice_size = self.values_ptr[-1]
        self.values = self.add_variable("values",
                                         shape=[len(self.indices)],
                                         dtype=tf.float32,
                                         initializer=tf.keras.initializers.RandomUniform,
                                         trainable=True)
        self.sparse_tensor = tf.SparseTensor(self.indices, self.values, [self.size, self.size])
        self.biases = self.add_variable("biases",
                                        shape=[self.size],
                                        dtype=tf.float32,
                                        initializer=tf.keras.initializers.RandomUniform,
#                                        initializer=tf.keras.initializers.Zeros,
                                        trainable=True)
        self.act = tf.keras.layers.PReLU(alpha_initializer=tf.keras.initializers.RandomUniform)
#        self.act = tf.keras.activations.linear

    def compute_output_shape(self, input_shape):
        return (input_shape[0], self.size)

    def call(self, input):
        return self.act(tf.nn.bias_add(tf.linalg.transpose(tf.sparse_tensor_dense_matmul(self.sparse_tensor, input, adjoint_b = True)), self.biases))
#        return tf.linalg.transpose(tf.sparse_tensor_dense_matmul(self.sparse_tensor, input, adjoint_b = True))

class MySparseLayerCSR(tf.keras.layers.Layer):
    def __init__(self, size, trainable=True):
        super(MySparseLayerCSR, self).__init__()
        self.size = size
        self.indices = neighbors_square_csr(size)
#        self.indices = neighbors_MED_csr()
        self.values_ptr = [0]
        for temp in self.indices:
            self.values_ptr.append(len(temp)+self.values_ptr[-1])
        self.partitions = np.zeros(self.values_ptr[-1])
        for i in range(self.size):
            self.partitions[self.values_ptr[i]:self.values_ptr[i+1]]=i
        #print "value shape: " + str(self.sparse_tensor.values.shape)
        #print "indices: " + str(self.sparse_tensor.indices)
#        indice_size = self.values_ptr[-1]
        self.values = self.add_variable("values",
                                         shape=[self.values_ptr[-1]],
                                         dtype=tf.float32,
                                         initializer=tf.keras.initializers.RandomUniform,
                                         trainable=True)
        self.biases = self.add_variable("biases",
                                        shape=[self.size],
                                        dtype=tf.float32,
#                                        initializer=tf.keras.initializers.RandomUniform,
                                        initializer=tf.keras.initializers.Zeros,
                                        trainable=True)
#        self.output_var = self.add_variable("output_var",
#                                             shape=[self.size],
#                                             dtype=tf.float32,
#                                             initializer=tf.keras.initializers.Zeros,
#                                             trainable=False)
        self.act = tf.keras.layers.PReLU()

    def compute_output_shape(self, input_shape):
        return (input_shape[0], self.size)

    def cond1(self, i, j, input, output):
        return i < self.size

    def body1(self, i, j, input, output):
        print "i = " + str(i)
        print "j = " + str(j)
#        tensorflow sparse tensor
#        output[self.indices[i][0]] = tf.math.add (output[self.indices[i][0]], tf.math.multiply(input[self.indices[i][1]], self.values[i]))
#        csr
        x = tf.gather(self.values, np.arange(self.values_ptr[j], self.values_ptr[j+1]))
        y = tf.gather(input, self.indices[j][:])
        output[i] = tf.tensordot(x, y)
        print "input[i] = " + str(input[i])
        return i + 1, j+1,  input, output

    def sparse_vectmat(self, vect):
        i = tf.constant(0)
        j = 0
        output_var = tf.fill([vect.shape[-1]], 0)
        i, j, vect, output_var = tf.while_loop(self.cond1, self.body1, loop_vars=[i, j, vect, output_var])
        return output_var

    def sparse_vectmat_old(self, vect):
        output_var = tf.fill([vect.shape[-1]], 0)
        for i in range(len(self.indices)):
            output_var[self.indices[i][0]] = tf.add (output_var[self.indices[i][0]], tf.multiply(vect[self.indices[i][1]], self.values[i]))
        return output_var

    def sparse_vectmat_test(self, vect):
#        output_var = np.zeros(vect.shape[-1])
#        output_var = tf.TensorArray(dtype=tf.float32, size=self.size)
        output_list = []
        print "vect = " + str(vect)
        y_list = tf.dynamic_partition(self.values, self.partitions, self.size)
        for i in range(self.size):
            x = tf.gather(vect, self.indices[i], axis = 1)
#            y = tf.gather(self.values, np.arange(self.values_ptr[i], self.values_ptr[i+1]))

#            y = y_list[i]
#            print "y shape : "+str(y.shape)
#            print "y= "+str(y)
            y = tf.reshape(y_list[i], [len(self.indices[i]), 1])
#            print "x = " + str(x)
#            print "y = " + str(y)
#            print "tensordot = " + str(tf.matmul(x, y))
#            output_var = output_var.write(i, tf.matmul(x, y))
            output_list.append(tf.tensordot(x, y, 1))
#        print "output var = " + str(output_var)
#        out = output_var.stack()
        out = tf.concat(output_list, 1)
#        out = tf.reshape(out, [-1, 100])
        print "out stack = " + str(out)
#        out = output_var.concat()
#        print "out concat = " + str(out)
#        out = tf.concat(output_var, 0)
#        print "out = " + str(out)
        return out


    def call(self, input):
        print "input shape ===== :" + str(input.shape)
#        output_var = tf.fill([-1, self.size], 0)
#        return self.act(tf.nn.bias_add(tf.linalg.transpose(tf.sparse_tensor_dense_matmul(self.sparse_tensor, input, adjoint_b = True)), self.biases))
        return self.act(tf.nn.bias_add(self.sparse_vectmat_test(input), self.biases))
#        return self.sparse_vectmat_test(input)


class MyModel(tf.keras.Model):
    def __init__(self, nb_parameters, vect_size):
        super(MyModel, self).__init__()
#        self.first_layer = MyFirstLayer(nb_parameters, vect_size, trainable = True)
        self.dense1 = tf.keras.layers.Dense(vect_size,
                                            kernel_initializer=tf.keras.initializers.Ones,
                                            bias_initializer=tf.keras.initializers.Zeros,
                                            activation=tf.keras.activations.linear,
                                            trainable = True)
        self.sparse = []
        for i in range(6):
            self.sparse.append(MySparseLayerSparseTensor(vect_size, trainable = True))
        self.out = MySparseLayerSparseTensor(vect_size, trainable = True)

    def call(self, x):
#        x = self.first_layer(x)
        x = self.dense1(x)
#        x_saved = tf.identity(x)
        for i in range(len(self.sparse)):
            x = self.sparse[i](x)
#            if (i%2 == 0 and i > 0):
#                x = tf.keras.layers.add([x, x_saved])
#                x_saved = tf.identity(x)
        return self.out(x)
#        return x

class DenseModel(tf.keras.Model):
    def __init__(self, nb_parameters, vect_size, nb_layers = 10):
        super(DenseModel, self).__init__()
        self.first_layer = tf.keras.layers.Dense(vect_size, input_dim=nb_parameters, kernel_initializer='normal', activation='relu')
        self.dense_layers = []
        for i in range(nb_layers):
            self.dense_layers.append(tf.keras.layers.Dense(vect_size, input_dim=vect_size, kernel_initializer='normal', activation='relu'))

    def call(self, inputs):
        x = self.first_layer(inputs)
        x_saved = tf.identity(x)
        for i in range(len(self.dense_layers)):
            x = self.dense_layers[i](x)
            if (i%2 == 0 and i > 0):
                x = tf.keras.layers.add([x, x_saved])
                x = tf.keras.activations.relu(x)
                x_saved = tf.identity(x)
        return x


def InitModelSubashiny(nb_parameters, vect_size):
    model = tf.keras.models.Sequential()
#    model.add(tf.keras.layers.BatchNormalization())
    model.add(tf.keras.layers.Dense(vect_size*2, input_dim=nb_parameters))
#    model.add(tf.keras.layers.BatchNormalization())
    model.add(tf.keras.layers.Activation('relu'))
    model.add(tf.keras.layers.Dense(vect_size*3, kernel_initializer='normal'))
#    model.add(tf.keras.layers.BatchNormalization())
    model.add(tf.keras.layers.Activation('relu'))
    model.add(tf.keras.layers.Dense(vect_size*4, kernel_initializer='normal'))
#    model.add(tf.keras.layers.BatchNormalization())
    model.add(tf.keras.layers.Activation('relu'))
    model.add(tf.keras.layers.Dense(vect_size*3, kernel_initializer='normal'))
#    model.add(tf.keras.layers.BatchNormalization())
    model.add(tf.keras.layers.Activation('relu'))
    model.add(tf.keras.layers.Dense(vect_size*2, kernel_initializer='normal'))
#    model.add(tf.keras.layers.BatchNormalization())
    model.add(tf.keras.layers.Activation('relu'))
    model.add(tf.keras.layers.Dense(vect_size, kernel_initializer='normal'))
    return model


class melissa_helper:
    def __init__(self, nb_parameters, vect_size):
        self.train_x = []
        self.train_y = []
        self.test_x = []
        self.test_y = []
        self.nb_parameters = nb_parameters
        self.model = MyModel(nb_parameters, vect_size)
#        self.model = DenseModel(nb_parameters, vect_size, nb_layers = 5)
#        self.model = InitModelSubashiny(nb_parameters, vect_size)

def model_init_minibatch(vect_size, nb_parameters):
    hvd.init()
    # Horovod: pin GPU to be used to process local rank (one GPU per process)
    config = tf.ConfigProto()
#    config.gpu_options.allow_growth = True
#    config.gpu_options.visible_device_list = str(hvd.local_rank())
    tf.set_session(tf.Session(config=config))
    print "vect_size = "+str(vect_size)
    print "nb_parameters = "+str(nb_parameters)
    helper = melissa_helper(nb_parameters, vect_size)
    # Horovod: adjust learning rate based on number of GPUs.
    opt = tf.keras.optimizers.Adam()
    # Horovod: add Horovod Distributed Optimizer.
    optimizer = hvd.DistributedOptimizer(opt)
    helper.model.compile(optimizer=optimizer, loss='mse', metrics=['mse'])
    helper.model.summary()
    hvd.broadcast_global_variables(0)
    return helper

def model_init_parallel(vect_size, nb_parameters):
    print "vect_size = "+str(vect_size)
    print "nb_parameters = "+str(nb_parameters)
    helper = melissa_helper(nb_parameters, vect_size)
    optimizer = tf.keras.optimizers.Adam(lr=0.001)
    helper.model.compile(optimizer=optimizer, loss='mse', metrics=['mse'])
#    helper.model.summary()
    return helper

def model_init(vect_size, nb_parameters):
    return model_init_parallel(vect_size, nb_parameters)

def add_to_training_set(x, y, handle):
    handle.train_y.append(rescale_data(y))
    handle.train_x.append(rescale_params(x))

def add_to_testing_set(x, y, handle):
    handle.test_y.append(rescale_data(y))
    handle.test_x.append(rescale_params(x))

def train_batch(handle):
    X_train=np.array(handle.train_x)
    Y_train=np.array(handle.train_y)
#    print "train" + str(X_train)
    res = handle.model.train_on_batch(X_train, Y_train)
    handle.train_x = []
    handle.train_y = []
    return res

def test_batch(handle):
    X_test=np.array(handle.test_x)
    Y_test=np.array(handle.test_y)
#    print "X_test = "+str(X_test)
    res = handle.model.test_on_batch(X_test,Y_test)
    handle.test_x = []
    handle.test_y = []
    return res

def save_model(handle, dirname, filename):
    save_model_parallel(handle, dirname, filename)

def read_model(handle, dirname, filename):
    read_model_parallel(handle, dirname, filename)

def read_model_minibatch(handle, dirname, filename):
    if hvd.rank() == 0:
        handle.model.build((1,handle.nb_parameters))
        handle.model.load_weights(dirname+"/"+filename)
    # Horovod: adjust learning rate based on number of GPUs.
    opt = keras.optimizers.Adam()
    # Horovod: add Horovod Distributed Optimizer.
    opt = hvd.DistributedOptimizer(opt)

    hvd.broadcast_global_variables(0)

def read_model_parallel(handle, dirname, filename):
    handle.model.build((1,handle.nb_parameters))
    handle.model.load_weights(dirname+"/rank"+str(MPI.COMM_WORLD.Get_rank())+"/"+filename)

def save_model_minibatch(handle, dirname, filename):
    if hvd.rank() == 0:
        handle.model.save_weights(dirname+"/"+filename)
    print "Horovod size: "+str(hvd.size())
    print "Horovod local rank: "+str(hvd.local_rank())
    print "Horovod rank: "+str(hvd.rank())

def save_model_parallel(handle, dirname, filename):
    handle.model.summary()
    keras.utils.plot_model(handle.model, to_file='model.png')
    if (not os.path.isdir(dirname+"/rank"+str(MPI.COMM_WORLD.Get_rank()))):
        os.mkdir(dirname+"/rank"+str(MPI.COMM_WORLD.Get_rank()))
    handle.model.save_weights(dirname+"/rank"+str(MPI.COMM_WORLD.Get_rank())+"/"+filename)


def test_model(handle):
    param_set = np.zeros(2)
    param_set[0] = 0.99
    param_set[-1] = 0.9
    param_set=np.expand_dims(param_set, 0)
#    res = handle.model.predict(param_set, batch_size=1)
#    c = res.reshape(50, 50)
#    plt.matshow(c)
#    plt.show()

