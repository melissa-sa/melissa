import time
import numpy as np

from enum import Enum


class SimulationStatus(Enum):
    CONNECTED = 0
    RUNNING = 1
    FINISHED = 2


class Simulation:

    def __init__(self, simulation_id, fields, ntimesteps,
                 status=SimulationStatus.CONNECTED.value,
                 last_timestep=0, timeout=0, last_message=0.0,
                 job_id='0', job_status=-1, parameters=None,
                 max_delay=60 * 10):

        self.simulation_id = simulation_id
        self.status = status
        self.last_timestep = last_timestep
        self.timeout = timeout
        self.last_message = last_message
        self.job_id = job_id
        self.job_status = job_status
        self.parameters = parameters
        self.ntimesteps = ntimesteps
        self.timesteps = {field: np.zeros(ntimesteps) for field in fields}
        self.max_delay = max_delay
    
    def crashed(self):
        timeout = (time.time() - self.last_message) > self.max_delay
        return (not self.finished()) and timeout

    def mark_message_recieved(self, field, timestep):
        self.timesteps[field][timestep] = 1
        # Update last message rcvd time
        self.last_message = time.time()

    def finished(self):
        rcvd = sum(np.sum(ts) for ts in self.timesteps.values())
        return rcvd == len(self.timesteps) * self.ntimesteps
