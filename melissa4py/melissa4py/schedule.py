import os
import time
import numpy as np
import tensorflow as tf
import horovod.keras as hvd

from collections import defaultdict
from random import choice, sample

from melissa4py.buffer import ReplayBuffer, BucketizedReplayBuffer
from melissa4py.stats import Statistic
from mpi4py import MPI

comm = MPI.COMM_WORLD

def print_once(*args):
    if hvd.rank() == 0:
        print(args)

def build_lr_shedule(decrease_every=200, factor=0.5, min_lr=1e-6):
    def lr_schedule(epoch, lr):
        if epoch % decrease_every == 0:
            lr =  max(lr * factor, min_lr)
        return lr
    return lr_schedule


# as divergence can be a problem, one solution is to use patience but the problem with patience is that
# because we have just one epoch, initially we could reach a very low loss and that is a problem that even running average loss
# will have
# so we need better ways of detecting divergence
# and that would be to use the running average and see if it 
class Bucket_LR_Scheduler(tf.keras.callbacks.Callback):

    def __init__(self, maxlr, minlr, buckets, epochs_per_bucket=100,
                 patience=100, monitor='mae', restore_best_weights=False,
                 init_bucket=0):
        
        super(Bucket_LR_Scheduler, self).__init__()
        self.init_bucket = init_bucket
        self.lrs = np.linspace(minlr, maxlr, buckets)
        self.lr = self.lrs[init_bucket]
        self.buckets = buckets
        self.reverse_lrs = {self.lrs[i]: i for i in range(buckets)}
        print(f'Learning rates: {self.lrs} | reverse: {self.reverse_lrs}')
        # TODO use this
        self.epochs_per_bucket = 10
        self.monitor = monitor
        self.baseline = None 
        self.best = np.Inf
        self.patience = patience
        # we are not computing self.cycles but if we need to
        # increment this variable in on_cycle_end method
        self.cycles = 0
        # automatic early stopping with global patience
        # number of epochs should enough for one complete cycle 
        self.global_patience = 10
        self.cycles_waited = 0
        # if we see no decrease in learning rate for 10 cycles
        # we start searching for the local minima
        # this is not a hyper parameter so that the user is not confused about the
        # two patience variables
        self.is_final_descent = False
        self.final_descend = tf.keras.callbacks.ReduceLROnPlateau(
            monitor='mae', 
            factor=0.67, 
            patience=1000, 
            verbose=0, 
            mode='auto',
            min_delta=0.0001, 
            cooldown=0, 
            min_lr=10**-7,
        )
        # what should global patience help us with?
        # when to start search for the local optima
        self.min_delta = 0
        self.wait = 0
        self.stopped_epoch = 0
        self.restore_best_weights = restore_best_weights
        self.best_weights = None
        self.monitor_op = np.less
        # counts how many buckets have been iterated till now
        self.buckets_travelled = 1
        self.cycles_waited = -1
        self.current_bucket = self.init_bucket
        self.epochs_in_bucket = 0
        self.going = 1
        self.current_score = Statistic(memory=0.9)
        self.final_descent_epoch =0 
    def get_bucket(self, lr):
        return np.argmin(np.abs(self.lrs - lr))
    
    def detect_divergence(self, loss):
        # best loss could go around 0.002?
        # divergence could be considered when loss is above 2?

        # TODO Achal
        # discuss with Bruno and Juan
        if loss > 500*self.best:
            return True

    def final_descent_init():
        tf.keras.backend.set_value(self.model.optimizer.lr, self.lrs[0])
        # should I restore best weights here before starting final descent?
        # TODO Discuss
    def final_descent_epoch_end(score):
        
        self.final_descend.set_model(self.model)
        self.final_descent_epoch += 1
        # current mae loss
        
        np_loss = np.array(score, dtype=np.float32)
        np_sum_loss = np.zeros(1, dtype=np.float32)
        # I guess we could get the number of MPI processes and divide the score by that to get a single gpu representation
        # but as that won't really change anything, except for precise loss values in the log, we can just go ahead with the sum 
        comm.Allreduce([np_loss, MPI.FLOAT],[np_sum_loss, MPI.FLOAT],op=MPI.SUM)
        # all references to score (current) is to the summed score from here on
        current = np_sum_loss[0]

        self.current_score.add(current)        
        return self.final_descend.on_epoch_end(self.final_descent_epoch, {self.monitor:current})

    def reduce_bucket():
        # finally, we won't be using this function due to the induced bias
        # but I am keeping the function call for future use cases with multiple epochs
        # as it is definitely something that should be experimented with when using cyclic learning rates
            
        pass
    
    # more like a update and get than just a getter,
    # the getter would be to use the variable directly: self.lrs[self.current_bucket]
    def get_lr(self, lr):
        self.buckets_travelled += 1
        self.epochs_in_bucket = 0
        current_bucket = self.get_bucket(lr)
        if current_bucket == 0:
            self.going = 1
        if current_bucket == (self.buckets - 1):
            self.going = -1
        new_bucket = current_bucket + self.going
        print_once('Bucket -> current_bucket: {current_bucket} | going {self.going} | new: {new_bucket}')
        self.current_bucket = new_bucket
        return self.lrs[new_bucket]

    def schedule(self, epoch):
        bucket = self.init_bucket + self.buckets_travelled
        return self.lrs[bucket]
    
    def on_train_begin(self, logs=None):
        return None

    def on_cycle_end(self, epoch, logs):
        self.cycles_waited += 1
        if self.cycles_waited > self.global_patience:
            return None
            # set learning rate to max minimum as ReduceLROnPla.. doesnt have bestweights concept
            # maybe it is a good idea to implement RLRoP here separately
            tf.keras.backend.set_value(self.model.optimizer.lr, self.lrs[0])
            # hvd.K.set_value(self.model.optimizer.lr, self.lrs[0])
            self.is_final_descent = True
            if self.restore_best_weights:
                print_once('Restoring model weights from ' + str(self.wait) + ' epochs before')
                self.model.set_weights(self.best_weights)
        return None

    def on_epoch_end(self, epoch, logs=None):
        
        # current mae loss
        current = logs.get(self.monitor)
        if current is None:
            raise Exception('logs does not contain the value for ' + self.monitor)
        np_loss = np.array(current, dtype=np.float32)
        np_sum_loss = np.zeros(1, dtype=np.float32)
        # I guess we could get the number of MPI processes and divide the score by that to get a single gpu representation
        # but as that won't really change anything, except for precise loss values in the log, we can just go ahead with the sum 
        comm.Allreduce([np_loss, MPI.FLOAT],[np_sum_loss, MPI.FLOAT],op=MPI.SUM)
        # all references to score (current) is to the summed score from here on
        current = np_sum_loss[0]

        self.current_score.add(current)



        if self.buckets_travelled % (2*self.buckets -1) == 0:
            # if buckets travelled is not incremented, this branch is always taken
            self.buckets_travelled += 1
            self.on_cycle_end(epoch, {self.monitor:current})
        # this condition will only be true if we forget to measure mae or changed our loss function
        
        if self.epochs_in_bucket == self.epochs_per_bucket:
            lr = self.get_lr(tf.keras.backend.get_value(self.model.optimizer.lr))
            tf.keras.backend.set_value(self.model.optimizer.lr, lr)
        else:
            self.epochs_in_bucket += 1
        
        
        # if current mae is better than the previous best
        # if self.current_score.n < 10:
        #     return

        print_once(f'Bucket -> current score: {current} | best: {self.best}')
        if self.monitor_op(current - self.min_delta, self.best):
            print_once(f'Bucket -> improvement {self.best} -> {current}')
            # update previous best
            self.best = current
            # reset wait counters
            self.cycles_waited = 0
            self.wait = 0
            # update best weights
            if self.restore_best_weights:
                self.best_weights = self.model.get_weights()
        # disabled patience
        # turning on divergence control
        if self.detect_divergence(current):
             print_once('Bucket -> divergence detected')
                if self.restore_best_weights:
                    print_once('Restoring model weights from when loss was: ', current)
                    self.model.set_weights(self.best_weights)
        # else:
        #     # print(f'Bucket -> no improvement | current {self.best}')
        #     # return
        #     # no restoration of weights
        #     # increment wait counter (for epoch here, for cycle is done in the on_cycle_end method)
        #     self.wait += 1
        #     # if we run out of patience (same logic for cycle patience in on_cycle_end)
        #     if self.wait >= self.patience:
        #         # get the next cyclic learning rate
        #         lr = self.get_lr(tf.keras.backend.get_value(self.model.optimizer.lr))
        #         tf.keras.backend.set_value(self.model.optimizer.lr, lr)
        #         print(f'Bucket -> no improvement -> getting next cyclic learning rate | new lr: {lr}')
        #         if self.restore_best_weights:
        #             print('Restoring model weights from ' + str(self.wait) + ' epochs before')
        #             self.model.set_weights(self.best_weights)
        #         # reset wait counter
        #         self.wait = 0

    def on_train_end(self, logs=None):
        return None
