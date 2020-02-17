import time

import numpy as np
import horovod.keras as hvd
import tensorflow as tf


class BaseLearner:

    def __init__(self, batch_size=32, *args, **kwargs):
        self.batch_size = batch_size
        self.samples_seen = 0
        self.xs, self.ys = [], []
        self.train_times = []
        self.scores = []

        print('Initialize horovod..')
        hvd.init()
        self.rank = hvd.local_rank()
        print('Rank: {} | Initializing session..'.format(self.rank))
        tfconfig = tf.ConfigProto(allow_soft_placement=True)
        tfconfig.gpu_options.allow_growth=True
        tfconfig.gpu_options.visible_device_list = str(hvd.local_rank())
        sess = tf.Session(config=tfconfig)
        tf.keras.backend.set_session(sess)

        print('Initializing model..')
        self.model = self.build_model(*args, **kwargs)
        self.optimizer = tf.keras.optimizers.Adam(0.01 * hvd.size())
        self.hvd_optimizer = hvd.DistributedOptimizer(self.optimizer,
                                                      device_dense='/cpu:0')
        print('Broadcasting global variables..')
        hvd.callbacks.BroadcastGlobalVariablesCallback(0)
        print('Compiling model..')
        self.model.compile(optimizer=self.hvd_optimizer,
                           loss='mae',
                           metrics=[])

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
            print("------------> Rank: {} | Training samples: {}".format(
                self.rank, len(self.xs)
            ))
            # 2. Fit model
            start = time.time()
            score = self.model.train_on_batch(X, Y)
            end = time.time()
            self.train_times.append((start, end))
            self.scores.append(score)
            # 3. Clear
            self.xs, self.ys = [], []
            return score

    def save(self):
        output_folder = self.config.dirname()
        self.model.save_weights(output_folder + "/" + "model_weights")

    def read(self, dirname, filename):
        self.model.build((1, self.nb_parameters))
        path = dirname+"/rank"+str(MPI.COMM_WORLD.Get_rank())+"/"+filename
        self.model.load_weights(path)