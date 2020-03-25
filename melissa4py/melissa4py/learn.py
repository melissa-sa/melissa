import os
import time

import numpy as np
import tensorflow as tf
import horovod.keras as hvd

from collections import defaultdict
from random import choice, sample
from melissa4py.buffer import ReplayBuffer


def build_lr_shedule(decrease_every=200, factor=0.5, min_lr=1e-6):
    def lr_schedule(epoch, lr):
        if epoch % decrease_every == 0:
            lr =  max(lr * factor, min_lr)
        return lr
    return lr_schedule


class BaseLearner:

    def __init__(self, batch_size=32, lr_start=0.001, lr_decrease_every=1,
                 lr_decrease_factor=1, lr_min_lr=1e-6, checkpoint_path='.',
                 checkpoint_every=20, lr_schedule=None, learn_every=None,
                 replay_buffer=None, *args, **kwargs):
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
        self.optimizer = tf.keras.optimizers.Adam(lr_start * hvd.size())
        self.hvd_optimizer = hvd.DistributedOptimizer(self.optimizer)
        print('Broadcasting global variables..')
        hvd.callbacks.BroadcastGlobalVariablesCallback(0)
        print('Compiling model..')
        self.model.compile(optimizer=self.hvd_optimizer, loss='mae')
        lr_schedule = lr_schedule or build_lr_shedule(
            decrease_every=lr_decrease_every,
            factor=lr_decrease_factor,
            min_lr=lr_min_lr
        )
        # Setup callbacks
        self.callbacks = [
            tf.keras.callbacks.LearningRateScheduler(lr_schedule),
        ]
        if kwargs.get('ReduceLROnPlateau', None):
            self.callbacks.append(tf.keras.callbacks.ReduceLROnPlateau(**kwargs.get('ReduceLROnPlateau', None)))
        # Set model
        for callback in self.callbacks:
            callback.set_model(self.model)
    
    def build_model(self, *args, **kwargs):
        inputs = tf.keras.Input(shape=(nb_parameters,))
        outputs = tf.keras.layers.Dense(vect_size, activation='sigmoid')(inputs)
        return tf.keras.Model(inputs, outputs)

    def learn(self, x, y):
        self.samples_seen += 1
        # Add sample to buffer
        self._buffer.put((x, y), partition=x[-1])
        if self._buffer.safe_to_sample and ((self.samples_seen % self.learn_every) == 0):
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
            score = self.model.train_on_batch(X, Y)
            for callback in self.callbacks:
                callback.on_epoch_end(batch, logs={'score': score})
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
            # if batch % self.checkpoint_every == 0:
            #     self.checkpoint(batch)
            return score

    def on_batch_end(self, batch, score, samples):
        pass

    def checkpoint(self, checkpoint_id):
        model_path = '{}/batch_{}'.format(self.checkpoint_path, batch)
        model_name = 'model_weights'
        try:
            os.mkdir(model_path)
        except OSError as e:
            print("Creation of the directory {} failed with error {}".format(model_path, e))
        self.save(model_path, model_name)

    def save(self, model_path, model_name):
        if self.rank == 0:
            outfile = '{}/{}'.format(model_path, model_name)
            self.model.save_weights(outfile)

    def read(self, dirname, filename):
        self.model.build((1, self.nb_parameters))
        path = dirname+"/rank"+str(MPI.COMM_WORLD.Get_rank())+"/"+filename
        self.model.load_weights(path)
