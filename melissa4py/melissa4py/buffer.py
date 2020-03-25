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



