from buffer import ReplayBuffer, BucketizedReplayBuffer
import numpy as np
class DummySource:
    count = 0
    def __init__(self):
        return
        # self.count = count + 1
    def safe_to_sample(self):
        return True
    def get_batch(self, batch_size):
        return list(range((batch_size)))

class DummyModel:
    count = 0
    optimizer = {}
    def __init__(self):
        self.optimizer = lambda: None
        setattr(self.optimizer, 'lr', 0.01)
        return
        # self.count = count + 1
    def get_weights(self):
        return 'W'
    def safe_to_sample(self):
        return True
    def get_batch(self, batch_size):
        return list(range((batch_size)))






class Bucket_LR_Scheduler():
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
        print(self.lrs)
        self.lr = self.lrs[init_bucket]
        self.buckets = buckets
        self.reverse_lrs = dict([(self.lrs[i],i) for i in range(buckets)])

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
        
        self.global_patience = 5
        self.cycles_waited = 0
        # if we see no decrease in learning rate for 10 cycles
        # we start searching for the local minima
        # this is not a hyper parameter so that the user is not confused about the
        # two patience variables
        self.is_final_descent = False
        # self.final_descend = tf.keras.callbacks.ReduceLROnPlateau(
        #     monitor='mae', 
        #     factor=0.67, 
        #     patience=100, 
        #     verbose=0, 
        #     mode='auto',
        #     min_delta=0.0001, 
        #     cooldown=0, 
        #     min_lr=10**-7,
        # )
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

    def get_bucket(self, lr):
        return self.reverse_lrs[lr]

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
        self.lr = self.lrs[new_bucket]
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
            print("Setting value of lr: " + str(self.lrs[0]))
            # hvd.K.set_value(self.model.optimizer.lr, self.lrs[0])

            self.is_final_descent = True
            if self.restore_best_weights:
                print('Restoring model weights from ' + str(self.wait) + ' epochs before')
                
                # self.model.set_weights(self.best_weights)


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
        # print(current)
        # How do we measure cycles?
        # how many epochs make a cycle?
        # it depends on the number of buckets iterated
        # 2*buckets -1
        # so we need to keep track of the number of buckets traversed
        # calculation epochs would be an incorrect measure because of earlyUpdate
        

        if self.buckets_travelled % (2*self.buckets -1) == 0:
            self.buckets_travelled += 1
            print("The cycle ends here because the number of buckets travelled is " + str(self.buckets_travelled) + " and 2*buckets-1 divides it")
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
                print("Restoring best weights")
                # self.best_weights = self.model.get_weights()
            
            if self.epochs_in_bucket == self.epochs_per_bucket:
                print("Setting value of lr: " + str(self.get_lr(logs['lr'])))
                # hvd.K.set_value(self.model.optimizer.lr, self.get_lr(self.model.optimizer.lr))
        else:
            # increment wait counter (for epoch here, for cycle is done in the on_cycle_end method)
            self.wait += 1

            # if we run out of patience (same logic for cycle patience in on_cycle_end)
            if self.wait >= self.patience:
                
                # get the next cyclic learning rate
                print("Setting value of lr: " + str(self.get_lr(logs['lr'])))

                # hvd.K.set_value(self.model.optimizer.lr, self.get_lr(self.model.optimizer.lr))
                
                # restore best weights achieved in the previous cycle
                # I don't know how this works in horovod, will it involve
                # a lot of communication
                # does each server contain a reference to the best_weights?

                if self.restore_best_weights:
                    print('Restoring model weights from ' + str(self.wait) + ' epochs before')
                    # self.model.set_weights(self.best_weights)

                # reset wait counter
                self.wait = 0

    def on_train_end(self, logs=None):
        return None
        # if self.stopped_epoch > 0 and self.verbose > 0:
        #     print('Epoch %05d: early stopping' % (self.stopped_epoch + 1))


# class BucketizedReplayBuffer():

#     def __init__(self, number_of_buckets, data_source, max_bucket_size=0, start_state=0, prefetch=10):
        
#         self.queues = [Queue(max_bucket_size) for _ in range(number_of_buckets)]
#         self.number_of_buckets = number_of_buckets
#         self._source = data_source
#         self.lr_bucket = start_state
#         self.prefetch_k = prefetch
#         # some python magic
#         # self._add_lambdas = [(lambda batch, b=i: self.add_batch(batch, b)) for i in range(number_of_buckets)]

#         # some code for helping choose hyperparameters
#         # self._max_queue_size_global = 0
#         # self._max_queue_size =  (0, None)
#         # self._min_queue_size = (max_bucket_size+1, None)
#         # self._biggest_difference = 0


#     # def analytics(self, bucket):
#     #     # uncomment return to disable analytics
#     #     # return

#     #     # potential for walrus op
#     #     temp = self.queues[bucket].qsize()
#     #     if temp > self._max_queue_size_global:
#     #         self._max_queue_size_global = temp
        
#     #     # will add more analytics if we need them
#     #     # dont want to overengineer!
#     #     # if self._max_queue_size[1] == bucket


#     # def add_batch(self, batch, bucket):

#     #     # uncomment below lines if you want your code to be slow but monkeyproof
#     #     # if bucket >= self.number_of_buckets:
#     #     #     raise(IndexError)
        
#     #     # add to bucket
#     #     self.queues[bucket].put_nowait(batch)
#     #     # add to mirror bucket
#     #     self.queues[self.number_of_buckets - bucket - 1].put_nowait(batch)
#     def put(self, item, *args, **kwargs):
#         self._source.put(item)

#     def update_bucket(self, new_bucket):
#         if new_bucket < self.number_of_buckets and new_bucket > -1:
#             self.lr_bucket = new_bucket

#     def get_batch(self, batch_size, ):

#         if not self.safe_to_sample:
#             return 
            
#         if self.buckets[self.lr_bucket].qsize()==0:
#             new_batches = [self.main.get(batch_size) for _ in range(self.prefetch_k)]

#             [self.buckets[self.buckets -1 - self.lr_bucket].put_nowait(batch) for batch in new_batches]
#             [self.buckets[self.lr_bucket].put_nowait(batch) for batch in new_batches[1:]]
#             return new_batches[0]
#         else:
#             return self.buckets[self.lr_bucket].get()


#     @property
#     def safe_to_sample(self):
#         return self._source.safe_to_sample
# class ReplayBuffer:

#     def __init__(self, size=10000, eviction_policy=fifo_policy, safety_wm=0.5):
#         self.next_sid = 0  # Next sampleID
#         self.safety_wm = safety_wm
#         self.size = size
#         self.samples = {}
#         self.eviction_policy = eviction_policy

#     def put(self, item, *args, **kwargs):
#         if self.full:
#             idx = self.eviction_policy(self.samples.keys())
#             del self.samples[idx]
#         self.samples[self.next_sid] = item
#         self.next_sid += 1

#     def get_batch(self, batch_size=32):
#         if self.safe_to_sample:
#             return sample(list(self.samples.values()), batch_size)

#     @property
#     def safe_to_sample(self):
#         return len(self.samples) >= self.size * self.safety_wm

#     @property
#     def full(self):
#         return len(self.samples) == self.size

a = DummySource()
b = BucketizedReplayBuffer(10, a)
c = Bucket_LR_Scheduler(0.01, 10**-6,10)

print(type(c))

lr = 0.01
for i in range(10000):
    c.on_epoch_end(i, {'mae':0.1, 'lr':lr})
    lr = c.lr
    b.update_bucket(c.current_bucket)
    print(c.current_bucket)
    # b.set_bucket(bucket)