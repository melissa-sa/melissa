import ctypes
import numpy as np

from enum import Enum


# C prototypes
c_char_ptr = ctypes.POINTER(ctypes.c_char)
c_double_ptr = ctypes.POINTER(ctypes.c_double)
c_void_ptr_ptr = ctypes.POINTER(ctypes.c_void_p)


# This should maybe go into a constants file
MAX_FIELD_NAME_SIZE = 128
MPI_MAX_PROCESSOR_NAME = 256


def get_str(buff):
    return buff.split(b'\x00')[0].decode('utf-8')


def encode(msg_parts):
    map(bytes, msg_parts)

class MessageType(Enum):
    HELLO = 0
    JOB = 1
    DROP = 2
    STOP = 3
    TIMEOUT = 4
    SIMU_STATUS = 5
    SERVER = 6
    ALIVE = 7
    CONFIDENCE_INTERVAL = 8
    OPTIONS = 9


class ServerNodeName:

    def __init__(self, rank, node_name):
        self.msg_prefix = ctypes.c_int32(MessageType.SERVER.value)
        self.rank = ctypes.c_int32(rank)
        self.node_name = node_name.encode('utf-8')
    
    def encode(self):
        return bytes(self.msg_prefix) + bytes(self.rank) + self.node_name

    def recv(self):
        pass


class ConnectionRequest:

    def __init__(self, simulation_id, comm_size):
        self.simulation_id = simulation_id
        self.comm_size = comm_size

    def encode(self):
        parts = [ctypes.c_int32.from_buffer_copy(buff[:4]),
                 ctypes.c_int32.from_buffer_copy(buff[4:])]
        
        return b''.join(map(bytes, parts))
    
    @classmethod
    def recv(cls, buff):
        comm_size = ctypes.c_int32.from_buffer_copy(buff[:4])
        simulation_id = ctypes.c_int32.from_buffer_copy(buff[4:])
        return cls(simulation_id.value, comm_size.value)


class ConnectionResponse(ctypes.Structure):

    def __init__(self, comm_size, sobol_op, learning, nb_parameters,
                 verbose_lvl, port_names):
        self.comm_size = ctypes.c_int32(comm_size)
        self.sobol_op = ctypes.c_int32(sobol_op)
        self.learning = ctypes.c_int32(learning)
        self.nb_parameters = ctypes.c_int32(nb_parameters)
        self.verbose_lvl = ctypes.c_int32(verbose_lvl)
        print('port_names: ', port_names)
        self.port_names = [
            port.ljust(MPI_MAX_PROCESSOR_NAME, '\x00').encode('utf-8')
            for port in port_names
        ]
        self.port_names = b''.join(self.port_names)
    
    def encode(self):
        parts = [self.comm_size, self.sobol_op, self.learning,
                 self.nb_parameters, self.verbose_lvl, self.port_names]
        msg = b''.join(map(bytes, parts))
        return msg


class SimulationData:

    def __init__(self, timestep, simulation_id, client_rank, data_size,
                 field_name, data):
        self.timestep = timestep
        self.simulation_id = simulation_id
        self.client_rank = client_rank
        self.data_size = data_size
        self.field_name = field_name
        self.data = data

    @classmethod
    def from_msg(cls, msg):
        timestep, simulation_id, client_rank, data_size = np.frombuffer(msg[:4 * 4], np.int32)
        print('msg header: {} | {} | {} | {} '.format(
            timestep, simulation_id, client_rank, data_size
        ))
        field_name = get_str(msg[4 * 4: 4 * 4 + MAX_FIELD_NAME_SIZE])
        # Unpack data
        offset = 4 * 4 + MAX_FIELD_NAME_SIZE
        # print('data msg: ', msg[offset: offset + data_size * 8])
        data = np.frombuffer(msg[offset: offset + data_size * 8], np.double)
        # print('data: ', data)
        return cls(timestep, simulation_id, client_rank, data_size, field_name, data)


class JobDetails:

    def __init__(self, simulation_id, job_id, parameters):
        self.simulation_id = simulation_id
        self.job_id = job_id
        self.parameters = parameters

    @classmethod
    def from_msg(cls, msg, nb_parameters):
        # TODO: validate message ?
        print('nb_parameters: {} | msg: {}'.format(nb_parameters, msg))
        msg_type = ctypes.c_int32.from_buffer_copy(msg[:4])
        simulation_id = ctypes.c_int32.from_buffer_copy(msg[4:8])
        job_id = msg[8: (-nb_parameters * 8)].decode('utf-8')
        # Get parameters
        params = msg[(-nb_parameters * 8):]
        parameters = np.frombuffer(params, dtype=np.double)
        return cls(simulation_id.value, job_id, parameters)


class Stop:
    
    def encode(self):
        return bytes(ctypes.c_int32(MessageType.STOP.value))


class SimulationStatusMessage:

    def __init__(self, simulation_id, status):
        self.status = ctypes.c_int32(status.value)
        self.simulation_id = ctypes.c_int32(simulation_id)

    def encode(self):
        parts = [ctypes.c_int32(MessageType.SIMU_STATUS.value),
                 self.simulation_id,
                 self.status]
        msg = b''.join(map(bytes, parts))
        return msg