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
                 job_id='0', job_status=-1, parameters=None):
        
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
    
    def finished(self):
        rcvd = sum(np.sum(ts) for ts in self.timesteps.values())
        print('rcvd: ', rcvd)
        return rcvd == len(self.timesteps) * self.ntimesteps
