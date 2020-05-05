import numpy as np

from collections import defaultdict
from queue import Queue
from random import choice, choices, sample

from melissa4py.stats import Statistic


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
        self.stats = {'batch_appearances': Statistic()}

    def put(self, item, *args, **kwargs):
        if self.full:
            sid = self.eviction_policy(self.samples.keys())
            self.stats['batch_appearances'].add(self.samples[sid]['sampled'])
            del self.samples[sid]
        self.samples[self.next_sid] = {'item': item, 'sampled': 0}
        self.next_sid += 1

    def get_batch(self, batch_size=32):
        if self.safe_to_sample:
            samples = []
            for sid in sample(list(self.samples.keys()), batch_size):
                samples.append(self.samples[sid]['item'])
                self.samples[sid]['sampled'] += 1
            return samples

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
    
    @property
    def safe_to_sample(self):
        return len(self.partitions) >= self.safety_wm



def lowest_error_policy(priorities):
    return min(priorities, key=priorities.get)


class PriorityReplayBuffer:

    def __init__(self, size=10000, eviction_policy=lowest_error_policy, safety_wm=0.1):
        self.next_sid = 0
        self.safety_wm = safety_wm
        self.samples = {}
        self.priorities = {}
        self.size = size
        self.eviction_policy = eviction_policy

    def put(self, item, *args, **kwargs):
        if len(self.samples) >= self.size:
            idx = self.eviction_policy(self.priorities)
            del self.samples[idx], self.priorities[idx]
        self.samples[self.next_sid] = item
        self.priorities[self.next_sid] = max(self.priorities.values()) if self.priorities else 1
        self.next_sid += 1

    def get_batch(self, batch_size=32):
        if self.safe_to_sample:
            weights = np.array(list(self.priorities.items()))
            p = weights[:, 1] / np.sum(weights[:, 1])
            sids = np.random.choice(weights[:, 0], batch_size, p=p)
            return [(*self.samples[sid], sid) for sid in sids]
    
    def update_priority(self, sid, score):
        self.priorities[sid] = score

    @property
    def safe_to_sample(self):
        return len(self.samples) >= 5000


class BucketizedReplayBuffer:

    def __init__(self, number_of_buckets, data_source, max_bucket_size=0, start_state=0, prefetch=10):
        self.buckets = [Queue(max_bucket_size) for _ in range(number_of_buckets)]
        self.number_of_buckets = number_of_buckets
        self._source = data_source
        self.lr_bucket = start_state
        self.prefetch_k = prefetch
    def put(self, item, *args, **kwargs):
        self._source.put(item)

    def update_bucket(self, new_bucket):
        if new_bucket < self.number_of_buckets and new_bucket > -1:
            self.lr_bucket = new_bucket

    def get_batch(self, batch_size):
        if not self.safe_to_sample:
            return 
        if self.buckets[self.lr_bucket].qsize()==0:
            new_batches = [self._source.get_batch(batch_size) for _ in range(self.prefetch_k)]
            [self.buckets[self.number_of_buckets -1 - self.lr_bucket].put_nowait(batch) for batch in new_batches]
            [self.buckets[self.lr_bucket].put_nowait(batch) for batch in new_batches[1:]]
            return new_batches[0]
        else:
            return self.buckets[self.lr_bucket].get()

    @property
    def safe_to_sample(self):
        return self._source.safe_to_sample
