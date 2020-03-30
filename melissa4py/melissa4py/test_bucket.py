from learn import BaseLearner, Bucket_LR_Scheduler
from buffer import BucketizedReplayBuffer, ReplayBuffer

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

a = DummySource()
b = BucketizedReplayBuffer(10, a)
c = Bucket_LR_Scheduler(0.01, 10**-6,10, model=DummyModel())

print(type(c))

for i in range(10000):
    c.on_epoch_end(i, {'mae':0.1, 'lr':0.1})
    b.update_bucket(c.current_bucket)
    print(c.current_bucket)
    # b.set_bucket(bucket)