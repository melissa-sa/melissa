import numpy as np

from collections import defaultdict
from random import choice, choices, sample


def fifo_policy(sids):
    """FIFO eviction policy"""
    return min(sids)


def random_uniform_policy(sids):
    """Random eviction policy"""
    return choice(sids)


class ReplayBuffer:

    def __init__(self, size=10000, eviction_policy=fifo_policy, safety_wm=0.5):
        self.next_sid = 0  # Next sampleID
        self.safety_wm = safety_wm
        self.size = size
        self.samples = {}
        self.eviction_policy = eviction_policy

    def put(self, item, *args, **kwargs):
        if self.full:
            idx = self.eviction_policy(self.samples.keys())
            del self.samples[idx]
        self.samples[self.next_sid] = item
        self.next_sid += 1

    def get_batch(self, batch_size=32):
        if self.safe_to_sample:
            return sample(list(self.samples.values()), batch_size)

    @property
    def safe_to_sample(self):
        return len(self.samples) >= self.size * self.safety_wm

    @property
    def full(self):
        return len(self.samples) == self.size


class PartitionedReplayBuffer:

    def __init__(self, max_partition_size=300, eviction_policy=fifo_policy, safety_wm=12):
        self.next_sid = 0
        self.safety_wm = safety_wm
        self.partitions = defaultdict(dict)
        self.max_partition_size = max_partition_size
        self.eviction_policy = eviction_policy
    
    def put(self, item, partition=None):
        ps = self.partitions[partition]
        if len(ps) >= self.max_partition_size:
            idx = self.eviction_policy(ps.keys())
            del ps[idx]
        ps[self.next_sid] = item
        self.next_sid += 1
    
    def get_batch(self, batch_size=32):
        if self.safe_to_sample:
            sample = []
            for i in range(batch_size):
                p = choice([p for p, v in self.partitions.items() if len(v) > 0])
                s = choice(list(self.partitions[p].keys()))
                sample.append(self.partitions[p][s])
            return sample
            # print(self.partitions.keys())
            # print([list(p.keys()) for p in self.partitions.values()])
            #print(self.partitions)
            #ps = choices(self.partitions, k=batch_size)
            #print(ps)
            #return [p[choice(list(p.keys()))] for p in ps]
    
    @property
    def safe_to_sample(self):
        return len(self.partitions) >= self.safety_wm




# these replay buffers are in sync with learning rate
# the rule of thumb is that there will be discrete values of learning rates
# and the learning rates will be known to the buffers only via bin levels
# the bin levels must be sorted (agnostic to ascending or descending sort)
# the levels are 0 indexed by default
# each partition in the replay buffer stores training samples that were trained on a specific learning rate

from queue import Queue
# this is a fifo queue by default

# TODO
# do we need a parameter shuffle which can be toggled if training samples should be shuffled in the queue?
class BucketizedReplayBuffer():

    def __init__(self, number_of_buckets, data_source, max_bucket_size=0, start_state=0, prefetch=10):
        
        self.queues = [Queue(max_bucket_size) for _ in range(number_of_buckets)]
        self.number_of_buckets = number_of_buckets
        self._source = data_source
        self.lr_bucket = start_state
        self.prefetch_k = prefetch
        # some python magic
        # self._add_lambdas = [(lambda batch, b=i: self.add_batch(batch, b)) for i in range(number_of_buckets)]

        # some code for helping choose hyperparameters
        # self._max_queue_size_global = 0
        # self._max_queue_size =  (0, None)
        # self._min_queue_size = (max_bucket_size+1, None)
        # self._biggest_difference = 0


    # def analytics(self, bucket):
    #     # uncomment return to disable analytics
    #     # return

    #     # potential for walrus op
    #     temp = self.queues[bucket].qsize()
    #     if temp > self._max_queue_size_global:
    #         self._max_queue_size_global = temp
        
    #     # will add more analytics if we need them
    #     # dont want to overengineer!
    #     # if self._max_queue_size[1] == bucket


    # def add_batch(self, batch, bucket):

    #     # uncomment below lines if you want your code to be slow but monkeyproof
    #     # if bucket >= self.number_of_buckets:
    #     #     raise(IndexError)
        
    #     # add to bucket
    #     self.queues[bucket].put_nowait(batch)
    #     # add to mirror bucket
    #     self.queues[self.number_of_buckets - bucket - 1].put_nowait(batch)
    def put(self, item, *args, **kwargs):
        self._source.put(item)

    def update_bucket(self, new_bucket):
        if new_bucket < self.number_of_buckets and new_bucket > -1:
            self.lr_bucket = new_bucket

    def get_batch(self, batch_size, ):

        if not self.safe_to_sample:
            return 
            
        if self.buckets[self.lr_bucket].qsize()==0:
            new_batches = [self.main.get(batch_size) for _ in range(self.prefetch_k)]

            [self.buckets[self.buckets -1 - self.lr_bucket].put_nowait(batch) for batch in new_batches]
            [self.buckets[self.lr_bucket].put_nowait(batch) for batch in new_batches[1:]]
            return new_batches[0]
        else:
            return self.buckets[self.lr_bucket].get()


    @property
    def safe_to_sample(self):
        return self._source.safe_to_sample