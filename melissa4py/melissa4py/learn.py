import os
import time

import numpy as np
import tensorflow as tf
import horovod.keras as hvd
from collections import defaultdict
from random import choice, sample

from tensorflow.contrib.keras import backend as K

from melissa4py.buffer import ReplayBuffer, PartitionedReplayBuffer
from melissa4py.schedule import build_lr_shedule
from mpi4py import MPI

comm = MPI.COMM_WORLD

def new_loss(y_true, y_pred):
    loss1 = tf.reduce_mean(tf.abs(y_true - y_pred))
    loss2 = tf.reduce_mean(y_pred)
    y = tf.cond(tf.equal(tf.constant(1), tf.size(y_pred)), lambda: loss2, lambda: loss1)
    return y


class BaseLearner:

    def __init__(self, batch_size=32, lr_start=0.01, lr_decrease_every=1,
                 lr_decrease_factor=1, lr_min_lr=1e-6, checkpoint_path='.',
                 checkpoint_every=20, lr_schedule=None, learn_every=None,
                 replay_buffer=None, callbacks=None, loss='mae', feed_y=False, 
                 *args, **kwargs):
        self.feed_y = feed_y
        self.checkpoint_every = checkpoint_every
        self.checkpoint_path = checkpoint_path
        self.batch_size = batch_size
        self.samples_seen = 0
        self.learn_every = learn_every or self.batch_size
        self._buffer = replay_buffer if replay_buffer is not None else ReplayBuffer()
        self.history = defaultdict(list)
        print('Initialize horovod..')
        hvd.init()
        self.rank = hvd.rank()
        print('Rank: {} | Initializing session..'.format(self.rank))
        tfconfig = tf.ConfigProto(allow_soft_placement=True)
        tfconfig.gpu_options.allow_growth=True
        tfconfig.gpu_options.visible_device_list = str(hvd.local_rank())
        sess = tf.Session(config=tfconfig)
        tf.keras.backend.set_session(sess)

        print('Initializing model..')
        self.model = self.build_model(*args, **kwargs)
        self.optimizer = tf.keras.optimizers.Adam(lr_start)
        self.hvd_optimizer = hvd.DistributedOptimizer(self.optimizer)
        print('Broadcasting global variables..')
        hvd.callbacks.BroadcastGlobalVariablesCallback(0)
        print('Compiling model..')
        self.model.compile(optimizer=self.hvd_optimizer, loss=loss)
        lr_schedule = lr_schedule or build_lr_shedule(
            decrease_every=lr_decrease_every,
            factor=lr_decrease_factor,
            min_lr=lr_min_lr
        )
        # Setup callbacks
        self.callbacks = callbacks
        if not self.callbacks:
            self.callbacks = [tf.keras.callbacks.LearningRateScheduler(lr_schedule)]
        # Set model
        print('callbacks: ', self.callbacks)
        for callback in self.callbacks:
            callback.set_model(self.model)
    
    def build_model(self, *args, **kwargs):
        inputs = tf.keras.Input(shape=(nb_parameters,))
        outputs = tf.keras.layers.Dense(vect_size, activation='sigmoid')(inputs)
        return tf.keras.Model(inputs, outputs)

    def learn(self, x, y, comm=None, disable_learning=False):
        self.samples_seen += 1
        # Add sample to buffer
        # partition here is the timestep (last index of x)
        self._buffer.put((x, y), partition=x[-1])
        if self._buffer.safe_to_sample and ((self.samples_seen % self.learn_every) == 0):
            if comm is not None and not disable_learning:
                comm.Barrier()
            batch = self.samples_seen // self.batch_size
            # 1. Build batch.
            samples = self._buffer.get_batch(self.batch_size)
            xs, ys = [e[0] for e in samples], [e[1] for e in samples]
            X, Y = np.array(xs), np.array(ys)
            print('Batch timesteps: ', X[:, -1])
            # 2. Fit model
            start = time.time()
            for callback in self.callbacks:
                callback.on_epoch_begin(batch)
            if self.feed_y:
                # NOTE: for second-order loss
                score = self.model.train_on_batch((X,Y), (Y, [1 for _ in range(len(Y))]))
            else:
                if not disable_learning:
                    score = self.model.train_on_batch(X, Y)
                else:
                    # TODO: remove this after debugging
                    score = 0
            for callback in self.callbacks:
                callback.on_epoch_end(batch, logs={'score': score, 'mae': score})
            end = time.time()
            self.on_batch_end(batch, score, samples)
            # 3. Update history
            self.history['batch_mean'].append(np.mean(X[:, -1]))
            self.history['batch_variance'].append(np.var(X[:, -1]))
            self.history['training_time'].append((start, end))
            self.history['score'].append(score)
            self.history['learning_rate'].append(
                tf.keras.backend.get_value(self.hvd_optimizer.lr)
                # @Juan 
                # this works then? 
                # i thought we needed hvd.K.get_value() but they are both pointing to
                # the same piece of code so great
                # however I am using hvd.K instead of tf.keras.backend
            )
            # 4. Checkpoint model if necesary
            if (batch + 1) % self.checkpoint_every == 0:
                self.checkpoint(batch)
            print(f'Rank: {self.rank} | [Learner] Training done.')
            return score
        print(f'Rank: {self.rank} | [Learner] done.')
    
    # TODO: remove repeated code
    def replay(self):
        self.samples_seen += self.batch_size
        batch = self.samples_seen // self.batch_size
        # 1. Sample from buffer
        samples = self._buffer.get_batch(self.batch_size)
        xs, ys = [e[0] for e in samples], [e[1] for e in samples]
        X, Y = np.array(xs), np.array(ys)
        start = time.time()
        # 2. Train model
        for callback in self.callbacks:
            callback.on_epoch_begin(batch)
        if self.feed_y:
            # NOTE: for second-order loss
            score = self.model.train_on_batch((X,Y), (Y, [1 for _ in range(len(Y))]))
        else:
            score = self.model.train_on_batch(X, Y)
        for callback in self.callbacks:
            callback.on_epoch_end(batch, logs={'score': score, 'mae': score})
        end = time.time()
        self.on_batch_end(batch, score, samples)
        # 3. Update history
        self.history['batch_mean'].append(np.mean(X[:, -1]))
        self.history['batch_variance'].append(np.var(X[:, -1]))
        self.history['training_time'].append((start, end))
        self.history['score'].append(score)
        self.history['learning_rate'].append(
            tf.keras.backend.get_value(self.hvd_optimizer.lr)
        )
        # 4. Checkpoint model if necesary
        if (batch + 1) % self.checkpoint_every == 0:
            self.checkpoint(batch)
        return score

    def on_batch_end(self, batch, score, samples):
        
        # get learning rate
        # 

        pass

    def checkpoint(self, checkpoint_id):
        model_path = '{}/batch_{}'.format(self.checkpoint_path, checkpoint_id)
        model_name = 'model_weights'
        try:
            os.mkdir(model_path)
        except OSError as e:
            print("Creation of the directory {} failed with error {}".format(model_path, e))
        self.save(model_path, model_name)

    def save(self, model_path, model_name):
        if self.rank == 0:
            print(f'Rank: {self.rank} | Checkpointing..')
            outfile = '{}/{}'.format(model_path, model_name)
            self.model.save_weights(outfile)
            print(f'Rank: {self.rank} | Checkpoint done.')

    def read(self, dirname, filename):
        self.model.build((1, self.nb_parameters))
        path = dirname+"/rank"+str(MPI.COMM_WORLD.Get_rank())+"/"+filename
        self.model.load_weights(path)



# custom callback

class Bucket_LR_Scheduler(tf.keras.callbacks.Callback):
    def __init__(self,
                 maxlr,
                 minlr,
                 buckets,
                 epochs_per_bucket=100,
                 patience=100,
                 monitor='mae',
                 restore_best_weights=True,
                 init_bucket=0):
        
        super(Bucket_LR_Scheduler, self).__init__()

        # self.lr_min = minlr
        self.init_bucket = init_bucket
        # self.lr_max = maxlr
        self.lrs = np.linspace(minlr, maxlr, buckets)
        self.lr = self.lrs[init_bucket]
        self.buckets = buckets
        self.reverse_lrs = {(self.lrs[i],i) for i in range(buckets)}

        # TODO use this
        self.epochs_per_bucket = 100


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

    def get_bucket(self, lr):
        return np.argmin(np.abs(self.lrs - lr))
        # for _lr in self.lrs:
            
        # return self.reverse_lrs(lr)

    def get_lr(self, lr):
        self.buckets_travelled += 1
        self.epochs_in_bucket = 0

        current_bucket = self.get_bucket(lr)
        if current_bucket == 0:
            self.going = 1
        
        if current_bucket == self.buckets - 1:
            self.going = -1

        new_bucket = current_bucket+self.going
        self.current_bucket = new_bucket
        return self.lrs[new_bucket]

    def schedule(self, epoch):
        
        # which bucket am I on?

        # this logic needs to be tied to current_bucket 
        # bucket = (self.init_bucket + epoch)%self.buckets
        # the above line of code will not work as buckets are not tied to epochs
        bucket = self.init_bucket + self.buckets_travelled
        return self.lrs[bucket]
    
    def on_train_begin(self, logs=None):
        # if we need to allow instances to be re-used
        # use arguments to update members here
        return None

        # self.wait = 0
        # self.stopped_epoch = 0
        # self.best = np.Inf if self.monitor_op == np.less else -np.Inf

    def on_cycle_end(self, epoch, logs):
        


        self.cycles_waited += 1
        if self.cycles_waited > self.global_patience:
            # set learning rate to max minimum as ReduceLROnPla.. doesnt have bestweights concept
            # maybe it is a good idea to implement RLRoP here separately
            hvd.K.set_value(self.model.optimizer.lr, self.lrs[0])
            self.is_final_descent = True
            if self.restore_best_weights:
                print('Restoring model weights from ' + str(self.wait) + ' epochs before')
                self.model.set_weights(self.best_weights)


        return None
    def on_epoch_end(self, epoch, logs=None):
        
        # if we have switched our learning rate scheduler mechanism
        # from cyclic to final descent which is extreme decay to squeeze out best local minima

        if self.is_final_descent:
            # I don't know if this callback will be able to access the model
            # will this work? :thinks:
            self.final_descend.set_model(self.model)
            # if it doesnt work we basically have to implement reduceLRonPlateau here
            return self.final_descend.on_epoch_end(epoch, logs)
        
        # current mae loss
        current = logs.get(self.monitor)
        # np_loss = np.float64(current)
        np_loss = np.ones(current, dtype=np.float64)
        np_sum_loss = np.zeros(1, dtype=np.float64)
        comm.Allreduce([np_loss, MPI.DOUBLE],[np_sum_loss, MPI.DOUBLE],op=MPI.SUM)
        print("Just before the barrier", flush=True)
        MPI.barrier()
        # hvd reduce has an issue as we work with tensors
        # mpi reduce will only work for two nodes?
        # they are probably all separate mpi ranks?
        # verify that they are same in all ranks
        print("score is summed" + str(np_sum_loss[0]))
        # log summed score in rank 0
        
        # we shouldn't be using tensors as they require a sess to get the values
        # loss_tensor = tf.constant(current)
        # # average=False results in summation
        # # the op used internally is reduce_ops.Sum
        # summed_loss = hvd.allreduce(loss_tensor, average=False)
        # if(hvd.rank() == 0):
        #     print(summed_loss.)


        # print(current)
        # How do we measure cycles?
        # how many epochs make a cycle?
        # it depends on the number of buckets iterated
        # 2*buckets -1
        # so we need to keep track of the number of buckets traversed
        # calculation epochs would be an incorrect measure because of earlyUpdate
        

        if self.buckets_travelled % (2*self.buckets -1) == 0:
            self.on_cycle_end(epoch, logs)
        
        # this condition will only be true if we forget to measure mae or changed our loss function
        if current is None:
            print("Monitoring Mean Absolute Error but mae not found in logs")
            return

        # if current mae is better than the previous best
        if self.monitor_op(current - self.min_delta, self.best):
            # update previous best
            self.best = current
            # reset wait counters
            self.cycles_waited = 0
            self.wait = 0
            # update best weights
            if self.restore_best_weights:
                self.best_weights = self.model.get_weights()
            
            if self.epochs_in_bucket == self.epochs_per_bucket:
                hvd.K.set_value(self.model.optimizer.lr, self.get_lr(self.model.optimizer.lr))
        else:
            # increment wait counter (for epoch here, for cycle is done in the on_cycle_end method)
            self.wait += 1

            # if we run out of patience (same logic for cycle patience in on_cycle_end)
            if self.wait >= self.patience:
                
                # get the next cyclic learning rate
                hvd.K.set_value(self.model.optimizer.lr, self.get_lr(self.model.optimizer.lr))
                
                # restore best weights achieved in the previous cycle
                # I don't know how this works in horovod, will it involve
                # a lot of communication
                # does each server contain a reference to the best_weights?

                if self.restore_best_weights:
                    print('Restoring model weights from ' + str(self.wait) + ' epochs before')
                    self.model.set_weights(self.best_weights)

                # reset wait counter
                self.wait = 0

    def on_train_end(self, logs=None):
        return None
        # if self.stopped_epoch > 0 and self.verbose > 0:
        #     print('Epoch %05d: early stopping' % (self.stopped_epoch + 1))

