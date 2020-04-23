import numpy as np


class Statistic:

    def __init__(self, memory=1):
        self.n = 0
        self.sum = 0
        self.rm = 0
        self.memory = memory

    def add(self, x):
        self.n += 1
        self.sum += x
        # Robbins-Monro
        self.rm += self.memory * self.rm + (1 - self.memory) * self.rm

    @property
    def mean(self):
        if self.memory == 1:
            return self.sum / self.n if self.n > 0 else float('nan')
        else:
            return self.rm

    @property
    def var(self):
        # TODO: maybe add online estimator for the variance
        return 0

