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
import math
import tensorflow as tf
import timeit
import sys
import os
import os.path
#import scipy as sp
import time
import random
import tensorflow.keras.backend as K
from tensorflow.compat.v1.keras.backend import set_session
#from keras.backend.tensorflow_backend import set_session
from tensorflow.keras.layers import BatchNormalization
import horovod.tensorflow as hvd
# from mpi4py import MPI
from datetime import datetime
import logging

from config import Config, Specials, Params
config = Config()
logging.basicConfig( filename ="melissa_learning.log", level=logging.INFO)
log = logging.getLogger("melissa_learning")


# TODO
# Include Config
# Setup Handles
# Integrate with Talos
# Test with Melissa
# Incorporate Logging
# Callbacks
# Continous Logging (Tensorboard maybe, but also a custom log)


# TODO
# set seed for reproducibility

def rescale_params(param_set):
    return param_set


def rescale_data(data):
    for i in range(len(data)):
        if(data[i] <= 0):
            data[i] = 0
        else:
            data[i] = math.floor(data[i]*10000)
    return data

def neighbors_read():
    indices = [[0, 0]]
    file = config.get(Specials.VONKARMAN_NEIGHBOURHOOD, open, 'r')
    for line in file:
        indices.append([int(i) for i in line.split()])
    file.close()
    return indices
# extend an existing edge_list to incorporate parameters/features for each cell
def extend_neighbourhood(edge_list, number_of_parameters):
    extended_t = []
    for i, j in edge_list:
        for k1 in range(number_of_parameters):
            for k2 in range(number_of_parameters):
                extended_t.append([i*number_of_parameters+k1, j*number_of_parameters+k2])
    return extended_t

# nbs = extend_neighbourhood(neighbors_read(), 4)

def get_uniform_map(dims, index=None, cell=None):
    #cell is (x,y,z .. ) matching the len of dims
    # given an index 
    
#     first = index%dims[0]
#     index = int(index/dims[0])
#     second = index%dims[1]

    # and so on
    index_copy = index
    if index is not None:
        cell = []
        for dim in dims:
            cell.append(index%dim)
            index = int(index/dim)
        if get_uniform_map(dims, cell=cell) == index_copy:
            return cell
        else:
            return None
    elif cell is not None:
        #t needs to be multiplied with dimsX dimsY and dimsZ
        #whereas z only needs to be multiplied with dimsX dimsY 
        
        # but what if the cell is beyond the dimensions?
        # so we return none
        # check condition
        
        multiplier = 1
        index = 0
        for i,c in enumerate(cell):
            d = dims[i]
            if c >= d or c<0:
                return None
            index += c*multiplier
            multiplier *= d
        return index
    else:
        return None
            
from functools import reduce

def extract_neighbours(dims, diagonals=True, order=1):
    # dims is an array
    # dims[0] is the size of the first dimension and so on
    
    # so the maximum value of order is equal to the number of dimensions
    # assert 0 <= order <= len(dims)
    
    
    # assuming that the cell contains some additional information
    # need to generate an edge list
    
    # in case the cell info is not provided in the cell value
    # we can generate it from the helper function
    
    
    size = reduce(lambda x,y: x*y, dims, 1)
    print(size)
    edge_list = []
    for index in range(size):
    
        # assuming no useful information in cell
        # i is the index
        # so we can get the cell in the grid
        c = get_uniform_map(dims, index=index)
        
        
#         ncs = []
#         # start of logic
#         for i, dim in enumerate(dims):
#             for diff in [-1,0,1]:
#                 # permute
#                 c[i] += diff
#                 nc = get_uniform_map(dims, cell=c)
#                 if nc is not None:
#                     ncs.append(nc)
#                 c[i] -= diff
        
        # end of logic
        
        # now we just need to get the neighbours
        ncs = [index]
        
        
        for i,dim in enumerate(dims):
            c[i]+=1
            nc = get_uniform_map(dims, cell=c)
            if nc is not None:
                ncs.append(nc)
            c[i]-=2
            nc = get_uniform_map(dims, cell=c)
            if nc is not None:
                ncs.append(nc)
            c[i]+=1
        for nc in ncs:
            edge_list.append((index, nc))
    return edge_list

def extract_neighbours(dims, order=1):
    # dims is an array
    # dims[0] is the size of the first dimension and so on
    
    # so the maximum value of order is equal to the number of dimensions
    # assert 0 <= order <= len(dims)
    
    
    # assuming that the cell contains some additional information
    # need to generate an edge list
    
    # in case the cell info is not provided in the cell value
    # we can generate it from the helper function
    
    # the order here refers to the connectivity, for reference, check out Pixel Connectivity on Wikipedia
    
    size = reduce(lambda x,y: x*y, dims, 1)
    print(size)
    edge_list = []
    for index in range(size):
    
        # assuming no useful information in cell
        # i is the index
        # so we can get the cell in the grid
        c = get_uniform_map(dims, index=index)
        
        # now we have the main logic, the c is connected to its neighbours depending on its order
        # order is less than or equal to the dimensions
        
#         for idx_dim, dim in enumerate(dims):
#             for order in range(order):

        # when the order is 0, you can only change 1 of the dims
        # so allowed changes is order + 1
        # how do I pick the dimensions?
        # do a nested for loop for the number of times the order can change
        # use binary number concept
        # 100 means that only the first dim has been changed for a 3D cell
        # but now that the first dim has to be changed, we can do it in two ways, they are + and -1
        # so what happens when we have multiple dimensions to change? we need to worry about 2 to the 
        # power of number of changing dims
        # how do we do this?
        # we need to nest loops here
        # so can we remove the binary number logic and incorporate the same nested loops to generate
        # our picked dimensions?

        # oui je pense oui!
        # need recursion here ;

        # first parameter is the number of changes that are required (maximum)
        # second is the cell for which we compute the neighbours
        # and lastly we have the dims that is required to call get_uniform_map inside the recursion
        # or maybe we can shift that to in the end, yes that makes more sense
        # SO NO DIMS IN RECURSER
        # recurser(order+1, c, dims)

        # ncs = recurser(order, c)
        ncs = recursor(order, c, 0, [])
        # this function ideally reeturns all the locality aware neighbours
        # unaffected by the existence

        # so now we filter

        filtered_ncs = filter(lambda x: x is not None, map(lambda x: get_uniform_map(dims, cell=x), ncs))
        
        for nc in filtered_ncs:
            edge_list.append((index, nc))
#             edge_list.append((index,nc))
        
    return edge_list
#         ncs = []
#         # start of logic
#         for i, dim in enumerate(dims):
#             for diff in [-1,0,1]:
#                 # permute
#                 c[i] += diff
#                 nc = get_uniform_map(dims, cell=c)
#                 if nc is not None:
#                     ncs.append(nc)
#                 c[i] -= diff
        
        # end of logic
        
        # now we just need to get the neighbours
#         ncs = [index]
        
        
#         for i,dim in enumerate(dims):
#             c[i]+=1
#             nc = get_uniform_map(dims, cell=c)
#             if nc is not None:
#                 ncs.append(nc)
#             c[i]-=2
#             nc = get_uniform_map(dims, cell=c)
#             if nc is not None:
#                 ncs.append(nc)
#             c[i]+=1
#         for nc in ncs:
#             edge_list.append((indimsdex, nc))
#     return edge_list

def recursor(changes, cell, current_dim, acc_list, offset_list = []):
    if current_dim >= len(cell):
        new_cell = [cell[i] + offset_list[i] for i in range(len(cell))]
        # print("cell: ",cell)
        # print("offset list: ", offset_list)
        offset_list = []
        # print(new_cell)
        acc_list.append(tuple(new_cell))
        return acc_list
    
#     offset_list.append(0)
    
    if changes > 0:
        for offset in [-1,1]:
            temp_offset_list = list(offset_list)
            temp_offset_list.append(offset)
            acc_list = recursor(changes-1, cell, current_dim+1, acc_list, temp_offset_list)
    offset = 0
    temp_offset_list = list(offset_list)
    temp_offset_list.append(offset)
    return recursor(changes, cell, current_dim+1, acc_list, temp_offset_list)

        

nbs = extract_neighbours([48, 72, 48], 1)
nbs = extend_neighbourhood(nbs, 4)

def neighbors_write(indices, filename):
    file = config.get("extras", lambda prefix: open(prefix+'/'+filename, "w"))
    for i in indices:
        file.write(str(i[0])+" "+str(i[1])+"\n")
    file.close()

class MySparseLayerSparseTensor(tf.keras.layers.Layer):
    def __init__(self, size, trainable=True, indices=None):
        super(MySparseLayerSparseTensor, self).__init__()
        self.size = size
        self.indices = nbs
        # TODO
        # Find a good initialiser for Sparse Layer
        self.values = self.add_variable("values",
                                         shape=[len(self.indices)],
                                         dtype=tf.float32,
                                         initializer=tf.keras.initializers.glorot_normal(),
                                         trainable=True)
        self.sparse_tensor = tf.SparseTensor(self.indices, self.values, [self.size, self.size])
        self.biases = self.add_variable("biases",
                                        shape=[self.size],
                                        dtype=tf.float32,
                                        initializer=tf.keras.initializers.Zeros(),
                                        trainable=True)

    def compute_output_shape(self, input_shape):
        return (input_shape[0], self.size)

    def call(self, input):
        return (tf.nn.bias_add(tf.linalg.transpose(tf.sparse_tensor_dense_matmul(self.sparse_tensor, (input), adjoint_b = True)), self.biases))

class MyModel(tf.keras.Model):
    def __init__(self, nb_parameters, vect_size, number_of_sparse_layers=30, skip_connection=False, extended=True, batch_normalisation=True, repeat_input_frequency=5, initialiser_for_repeat=tf.initializers.he_normal()):
        super(MyModel, self).__init__()

        self.dense1 = tf.keras.layers.Dense(vect_size,
                                            kernel_initializer=tf.keras.initializers.glorot_normal(),
                                            bias_initializer=tf.keras.initializers.Zeros(),
                                            activation=tf.keras.activations.linear,
                                            trainable = True,
                                            name="InitialDense")
        self.sparse = []
        self.act = []
        self.batch_normalisation = []
        self.skip_connection = skip_connection
        self.indices = None
        self.dense = []
        self.bn = batch_normalisation
        self.repeat= repeat_input_frequency
        self.initialiser = initialiser_for_repeat
        # Some sample initialisers to choose from
        # with prefix: tf.keras.initializers.
        #   he_normal, glorot_normal, Constant(k)
        # tf.ones_initializer 
      
        # Force an even number of Sparse Layers 
        # so that we can have the exact number of BN Layers
        # and ensure skip connections till the very last layer
        
        # increment to the nearest even number 
        # without using an if condition

        number_of_sparse_layers = int((number_of_sparse_layers+1)/2)*2

        for i in range(number_of_sparse_layers):
            if self.bn:
                self.batch_normalisation.append(BatchNormalization(
                    axis=-1,
    #                 momentum=0.99,
    #                 epsilon=0.001,
                    center=True,
                    scale=True,
    #                 beta_initializer='zeros',
    #                 gamma_initializer='ones',
    #                 moving_mean_initializer='zeros',
    #                 moving_variance_initializer='ones',
    #                 beta_regularizer=None,
    #                 gamma_regularizer=None,
    #                 beta_constraint=None,
    #                 gamma_constraint=None,
    #                 renorm=False,
    #                 renorm_clipping=None,
    #                 renorm_momentum=0.99,
    #                 fused=None,
                    trainable=True,
    #                 virtual_batch_size=None,
    #                 adjustment=None,
                    name="BatchLayer_"+str(i)))
            self.act.append(tf.keras.layers.PReLU(alpha_initializer=tf.keras.initializers.Zeros()))
            if i%self.repeat == self.repeat-1:
                self.dense.append(tf.keras.layers.Dense(vect_size, kernel_initializer=tf.ones_initializer(), activation=tf.keras.activations.linear, trainable=True, use_bias=False))
            self.sparse.append(MySparseLayerSparseTensor(vect_size, trainable = True,indices=self.indices))
        # end of for loop
        self.out = MySparseLayerSparseTensor(vect_size, trainable = True, indices=self.indices)

    def call(self, x):
        # skip connection
#        x = self.first_layer(x)
        x_init_params = tf.identity(x)
        x = self.dense1(x)
        x_saved = tf.identity(x)
        for i in range(len(self.sparse)):
#             x = tf.keras.layers.BatchNormalization()(x)
            
            x = self.act[i](x)
            if self.bn:
                x = self.batch_normalisation[i](x)
            if i%self.repeat == self.repeat-1:
                x = tf.keras.layers.add([x, self.dense[int(i/self.repeat)](x_init_params)])
            x = self.sparse[i](x)
            if self.skip_connection:
                if i%2 == 1:
                    x = tf.keras.layers.add([x, x_saved])
                    x_saved = tf.identity(x)
        return self.out(x)


class melissa_helper:
    def __init__(self, nb_parameters, vect_size):
        self.train_x = []
        self.train_y = []
        self.test_x = []
        self.test_y = []

        # parse the hyperparameters and network mods here
        hps = config[Params]
        self.nb_parameters = nb_parameters

        log.info("Model Created")
        log.info("Hyperparams: ", hps)
        
        self.model = MyModel(nb_parameters,
                             vect_size,
                             hps[Params.HIDDENLAYERS],
                             hps[Params.SKIPCONNECTION],
                             hps[Params.BATCHNORMALIZATION],
                             hps[Params.REPEATINPUTFREQ])
        
        self.config = config
        
def model_init_minibatch(vect_size, nb_parameters):
    hvd.init()
    # Horovod: pin GPU to be used to process local rank (one GPU per process)
    tf_config = tf.ConfigProto()
    tf_config.gpu_options.allow_growth = True
    tf_config.gpu_options.visible_device_list = str(hvd.local_rank())
    tf_config.log_device_placement = True  # to log device placement (on which device the operation ran)
    sess = tf.Session(config=tf_config)
    set_session(sess)
    print("vect_size = "+str(vect_size))
    print("nb_parameters = "+str(nb_parameters))
    helper = melissa_helper(nb_parameters, vect_size)
    # Horovod: adjust learning rate based on number of GPUs.
    # opt = tf.keras.optimizers.Adam(lr=0.001)
    opt = tf.train.AdamOptimizer()
    # Horovod: add Horovod Distributed Optimizer.
    optimizer = hvd.DistributedOptimizer(opt)
    log.info("Model will start compiling")
    helper.model.compile(optimizer=optimizer,
                         loss=helper.config[Params.LOSSFUNCTION],
                         metrics=helper.config[Params.METRICS])
    hvd.broadcast_global_variables(0) 
    log.info("Model will start compiling")
    return helper

def model_init_parallel(vect_size, nb_parameters):
    log.info("vect_size = " + str(vect_size))
    log.info("nb_parameters = " + str(nb_parameters))
    helper = melissa_helper(nb_parameters, vect_size)
    # optimizer = tf.keras.optimizers.Adam(lr=0.001)
    optimizer = tf.train.AdamOptimizer()
    helper.model.compile(optimizer=optimizer, loss=config[Params.LOSSFUNCTION], metrics=config[Params.METRICS])
    # helper.model.summary()
    return helper

def model_init(vect_size, nb_parameters):
    return model_init_minibatch(vect_size, nb_parameters)
    return model_init_parallel(vect_size, nb_parameters)

delete_this_var = 0
def add_to_training_set(x, y, handle):

    global counter
    counter += 1
    global delete_this_var
    delete_this_var += 1


    log.info("Adding to Training Set")

    # print(x[1], y[1])

    # assuming that the data cleaning and preprocessing has taken place 
    # as its more efficient to do that at the source of data generation
    
    # np.savetxt( "addTrSetX_"+str(delete_this_var), x)
    # np.savetxt( "addTrSetY_"+str(delete_this_var), y)

    handle.train_y.append(y)
    handle.train_x.append(x)
    
    # handle.train_y.append(rescale_data(y))
    # handle.train_x.append(rescale_params(x))

def add_to_testing_set(x, y, handle):

    log.info("Adding to Testing Set")

    handle.test_y.append(rescale_data(y))
    handle.test_x.append(rescale_params(x))

counter = 0

def train_batch(handle):
    
    global counter
    log.info("Counter is"+str(counter))
    counter = 0    
    log.info("Training Batch")

    X_train=np.array(handle.train_x)
    Y_train=np.array(handle.train_y)
    log.info("X_Train length is " + str(len(X_train)) + " Y_Train length is " + str(len(Y_train)))

    res = handle.model.train_on_batch(X_train, Y_train)
    
    # here we need to extract the results from the train_on_batch
    # it could be a list of losses and metrics 
    # thus we need to segregate them in to a named entity for the logging

    handle.train_x = []
    handle.train_y = []

    # the loss function can be an array
    losses = handle.config[Params.LOSSFUNCTION]
    metrics = handle.config[Params.METRICS]

    if metrics is None:
        metrics = []

    if not isinstance(losses, list):
        losses = [losses]
    if not isinstance(metrics, list):
        metrics = [metrics]
    if not isinstance(res, list):
        res = [res]

    res_keys = losses + metrics

    # we want to ensure that our model is correctly returning the list
    # of expected outputs
    print(res_keys)
    print(handle.model.metrics_names)
    print(len(res))
    assert len(res_keys) == len(res)

    res_map = dict(zip(res_keys, res))
    print(res_map)

    return res_map

def test_batch(handle):

    log.info("Testing Batch")

    X_test=np.array(handle.test_x)
    Y_test=np.array(handle.test_y)

    res = handle.model.test_on_batch(X_test,Y_test)

    handle.test_x = []
    handle.test_y = []

# the loss function can be an array
    losses = handle.config[Params.LOSSFUNCTION]
    metrics = handle.config[Params.METRICS]

    if metrics is None:
        metrics = []

    if not isinstance(losses, list):
        losses = [losses]
    if not isinstance(metrics, list):
        metrics = [metrics]
    if not isinstance(res, list):
        res = [res]

    res_keys = losses + metrics

    # we want to ensure that our model is correctly returning the list
    # of expected outputs
    assert len(res_keys) == len(res)

    res_map = dict(zip(res_keys, res))
    return res_map

def save_output(handle, input, dirname):

    # folder name is the unique identifier
    # so we ask the config for the folder 
    # then we can save the model outputs and charts and
    # other meta data like epochs etc 
    # into the same folder
    # just dumping bulk data fow now

    output = handle.model.predict(input)

    # to maintain the reusability of this function
    # we need to keep it agnostic of size
    # so even though for Von Karman, we would do this:
    # output = np.reshape(output, [500, 7361, 4])
    # we will instead directly save the outputs using np.savetxt

    # and later the entity using this information could extract it
    # as per their requirements

    output_name = dirname +'/outputs.txt'
    # output_folder = config["results"] + '/' + datetime.today().strftime('%Y-%m-%d-%H:%M:%S')

    np.savetxt(output_name, output)

    # TODO
    # Need to save multiple other things like 

def save_model(handle):
   
    output_folder = config.dirname()

    handle.model.save_weights(output_folder + "/" + "model_weights")
    return

def save(handle, input):
    
    log.info("Saving Everything")



    output_folder = config.dirname()
    

    save_model(handle, output_folder)
    save_output(handle, input, output_folder)

def read_model(handle, dirname, filename):
    read_model_parallel(handle, dirname, filename)

def read_model_minibatch(handle, dirname, filename):
    if hvd.rank() == 0:
        handle.model.build((1,handle.nb_parameters))
        handle.model.load_weights(dirname+"/"+filename)
    # Horovod: adjust learning rate based on number of GPUs.
    opt = tf.keras.optimizers.Adam(lr=0.001)
    # Horovod: add Horovod Distributed Optimizer.
    opt = hvd.DistributedOptimizer(opt)

    hvd.broadcast_global_variables(0)

def read_model_parallel(handle, dirname, filename):
    handle.model.build((1,handle.nb_parameters))
    handle.model.load_weights(dirname+"/rank"+str(MPI.COMM_WORLD.Get_rank())+"/"+filename)

def save_model_minibatch(handle, dirname, filename):
    print("Saving Weights ;)")
    return
    if hvd.rank() == 0:
        save_model(handle, dirname, filename)
        handle.model.save_weights(dirname+"/"+filename)
    print("Horovod size: "+str(hvd.size()))
    print("Horovod local rank: "+str(hvd.local_rank()))
    print("Horovod rank: "+str(hvd.rank()))

def save_model_parallel(handle, dirname, filename):
    #handle.model.summary()
    print("Saving Weights ;)")
    return
    if (not os.path.isdir(dirname+"/rank"+str(MPI.COMM_WORLD.Get_rank()))):
        os.mkdir(dirname+"/rank"+str(MPI.COMM_WORLD.Get_rank()))
    handle.model.save_weights(dirname+"/rank"+str(MPI.COMM_WORLD.Get_rank())+"/"+filename)


#def test_model(handle):
#    param_set = np.zeros(2)
#    param_set[0] = 0.99
#    param_set[-1] = 0.9
#    param_set=np.expand_dims(param_set, 0)
#    res = handle.model.predict(param_set, batch_size=1)
#    c = res.reshape(50, 50)
#    plt.matshow(c)
#    plt.show()
def test_model(handle):
    print("Testing Model")
    return
    X_test=np.array(handle.test_x)
    Y_test=np.array(handle.test_y)
#    print "X_test = "+str(X_test)
    Y_predicted = handle.model.predict(X_test)
    diff_obs_simu = 0
    diff_obs_mean = 0
    y_mean = 0
    print("--------------------------")
    print("learning_rate = "+str(K.eval(handle.model.optimizer.lr)))
    # print(Y_test)
    for i in range (Y_test.shape[0]):
        for j in range (Y_test.shape[1]):
            y_mean += Y_test[i][j]
    y_mean = y_mean/((Y_test.shape[0]*Y_test.shape[1]))
    for i in range (Y_predicted.shape[0]):
        for j in range (Y_predicted.shape[1]):
            temp1 = (Y_predicted[i][j]- Y_test[i][j])**2
            temp2 = (Y_test[i][j]- y_mean)**2
            diff_obs_simu += temp1
            diff_obs_mean += temp2            
    
    print("NSE = " + str(1 - diff_obs_simu/diff_obs_mean))
    print("--------------------------")
# This function is for changing the learning rate interactively with number of simulations
def set_learning_rate(handle, cnt, threshold1, lr1, threshold2, lr2, threshold3, lr3):
    print("set learning rate")
    return
    if(cnt >= threshold1):
        K.set_value(handle.model.optimizer.lr, lr1)
    if(cnt >= threshold2):
        K.set_value(handle.model.optimizer.lr, lr2)
    if(cnt >= threshold3):
        K.set_value(handle.model.optimizer.lr, lr3)
