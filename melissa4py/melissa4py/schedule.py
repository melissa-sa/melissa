import os
import time
import numpy as np
import tensorflow as tf
import horovod.keras as hvd

from collections import defaultdict
from random import choice, sample

from melissa4py.buffer import ReplayBuffer, BucketizedReplayBuffer
from melissa4py.stats import Statistic


def build_lr_shedule(decrease_every=200, factor=0.5, min_lr=1e-6):
    def lr_schedule(epoch, lr):
        if epoch % decrease_every == 0:
            lr =  max(lr * factor, min_lr)
        return lr
    return lr_schedule


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
            patience=100, 
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
        self.buckets_travelled = 0
        self.cycles_waited = -1
        self.current_bucket = self.init_bucket
        self.epochs_in_bucket = 0
        self.going = 1
        self.current_score = Statistic(memory=0.9)

    def get_bucket(self, lr):
        return self.reverse_lrs[lr]

    def get_lr(self, lr):
        self.buckets_travelled += 1
        self.epochs_in_bucket = 0
        current_bucket = self.get_bucket(lr)
        if current_bucket == 0:
            self.going = 1
        if current_bucket == (self.buckets - 1):
            self.going = -1
        new_bucket = current_bucket + self.going
        print('Bucket -> current_bucket: {current_bucket} | going {self.going} | new: {new_bucket}')
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
            # set learning rate to max minimum as ReduceLROnPla.. doesnt have bestweights concept
            # maybe it is a good idea to implement RLRoP here separately
            tf.keras.backend.set_value(self.model.optimizer.lr, self.lrs[0])
            # hvd.K.set_value(self.model.optimizer.lr, self.lrs[0])
            self.is_final_descent = True
            if self.restore_best_weights:
                print('Restoring model weights from ' + str(self.wait) + ' epochs before')
                self.model.set_weights(self.best_weights)
        return None

    def on_epoch_end(self, epoch, logs=None):
        if self.is_final_descent:
            self.final_descend.set_model(self.model)
            return self.final_descend.on_epoch_end(epoch, logs)
        # current mae loss
        current = logs.get(self.monitor)
        self.current_score.add(current)
        if self.buckets_travelled % (2*self.buckets -1) == 0:
            self.on_cycle_end(epoch, logs)
        # this condition will only be true if we forget to measure mae or changed our loss function
        if current is None:
            print("Monitoring Mean Absolute Error but mae not found in logs")
            return

        if self.epochs_in_bucket == self.epochs_per_bucket:
            lr = self.get_lr(tf.keras.backend.get_value(self.model.optimizer.lr))
            tf.keras.backend.set_value(self.model.optimizer.lr, lr)
        else:
            self.epochs_in_bucket += 1

        # if current mae is better than the previous best
        if self.current_score.n < 10:
            return

        print(f'Bucket -> current score: {current} | best: {self.best}')
        if self.monitor_op(current - self.min_delta, self.best):
            print(f'Bucket -> imporovement {self.best} -> {current}')
            # update previous best
            self.best = current
            # reset wait counters
            self.cycles_waited = 0
            self.wait = 0
            # update best weights
            if self.restore_best_weights:
                self.best_weights = self.model.get_weights()
        else:
            print(f'Bucket -> no imporovement | current {self.best}')
            # increment wait counter (for epoch here, for cycle is done in the on_cycle_end method)
            self.wait += 1
            # if we run out of patience (same logic for cycle patience in on_cycle_end)
            if self.wait >= self.patience:
                # get the next cyclic learning rate
                lr = self.get_lr(tf.keras.backend.get_value(self.model.optimizer.lr))
                tf.keras.backend.set_value(self.model.optimizer.lr, lr)
                print(f'Bucket -> no improvement -> getting next cyclic learning rate | new lr: {lr}')
                if self.restore_best_weights:
                    print('Restoring model weights from ' + str(self.wait) + ' epochs before')
                    self.model.set_weights(self.best_weights)
                # reset wait counter
                self.wait = 0

    def on_train_end(self, logs=None):
        return None
