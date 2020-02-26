import time

import numpy as np
import horovod.keras as hvd
import tensorflow as tf

from mpi4py import MPI


class BaseLearner:

    def __init__(self, learning_rate=0.01, batch_size=32, use_horovod=True,
                 *args, **kwargs):
        self.learning_rate = learning_rate
        self.batch_size = batch_size
        self.samples_seen = 0
        self.xs, self.ys = [], []
        self.train_times = []
        self.scores = []
        self.use_horovod = use_horovod
        self.rank = MPI.COMM_WORLD.Get_rank()
        print('Initialize horovod..')
        if use_horovod:
            hvd.init()
            self.local_rank = hvd.local_rank()
        else:
            self.local_rank = self.rank
        
        print('Rank: {} | Initializing session..'.format(self.local_rank))
        tfconfig = tf.ConfigProto(allow_soft_placement=True)
        tfconfig.gpu_options.allow_growth=True
        tfconfig.gpu_options.visible_device_list = str(self.local_rank)
        sess = tf.Session(config=tfconfig)
        tf.keras.backend.set_session(sess)

        print('Initializing model..')
        self.model = self.build_model(*args, **kwargs)
        self.optimizer = tf.keras.optimizers.Adam(self.learning_rate * hvd.size())
        if use_horovod:
            optimizer = hvd.DistributedOptimizer(self.optimizer)
            print('Broadcasting global variables..')
            hvd.callbacks.BroadcastGlobalVariablesCallback(0)
        else:
            optimizer = self.optimizer
        print('Compiling model..')
        self.model.compile(optimizer=optimizer, loss='mae', metrics=[])

    def build_model(self, *args, **kwargs):
        inputs = tf.keras.Input(shape=(nb_parameters,))
        outputs = tf.keras.layers.Dense(vect_size, activation='sigmoid')(inputs)
        return tf.keras.Model(inputs, outputs)
    
    def learn(self, x, y):
        self.samples_seen += 1
        self.xs.append(x); self.ys.append(y)
        if len(self.xs) >= self.batch_size:
            # 1. Build batch.
            X, Y = np.array(self.xs), np.array(self.ys)
            print("Rank: {} | Training with batch of size {}".format(self.rank, len(self.xs)))
            # 2. Fit model
            start = time.time()
            score = self.model.train_on_batch(X, Y)
            end = time.time()
            self.train_times.append((start, end))
            self.scores.append(score)
            # 3. Clear
            self.xs, self.ys = [], []
            return score

    def save(self, path, name='model_weights'):
        self.model.save_weights('{}/{}'.format(path, name))

    def read(self, dirname, filename):
        self.model.build((1, self.nb_parameters))
        path = dirname+"/rank"+str(MPI.COMM_WORLD.Get_rank())+"/"+filename
        self.model.load_weights(path)
