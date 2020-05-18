import ctypes
import logging
import socket
import pickle
import zmq

import numpy as np

from enum import Enum
from collections import namedtuple
from threading import Thread

from mpi4py import MPI
from melissa4py.message import MessageType
from melissa4py.message import ServerNodeName
from melissa4py.message import ConnectionRequest, ConnectionResponse
from melissa4py.message import SimulationData
from melissa4py.message import JobDetails
from melissa4py.message import Stop
from melissa4py.message import SimulationStatusMessage
from melissa4py.fault_tolerance import Simulation, SimulationStatus


# logging.basicConfig(filename="melissa_server.log", level=logging.INFO)
# logger = logging.getLogger("melissa_server")


class ServerStatus(Enum):
    CHECKPOINT = 1
    DATA_RECIEVED = 2


class Verbose(Enum):
    MELISSA_INFO = 1


class MelissaServer:

    def __init__(self, nb_time_steps, nb_proc_server, nb_parameters,
                 learning=1, restart=False, fields=None,
                 verbose_level=Verbose.MELISSA_INFO,
                 connection_port=2003, text_pull_port=5556,
                 text_push_port=5555, text_request_port=5554,
                 data_puller_port=2005, linger=-1,
                 node_name='localhost',
                 launcher_node_name='localhost',
                 checkpoint_file='checkpoint.pkl',
                 data_hwm=32,
                 sample_size=None):
        
        # Initlialize MPI
        self.sobol_op = 0
        self.comm = MPI.COMM_WORLD
        self.rank = self.comm.Get_rank()
        self.comm_size = self.comm.Get_size()
        self.fields = fields
        self.checkpoint_file = checkpoint_file
        self.nb_time_steps = nb_time_steps
        self.nb_proc_server = nb_proc_server
        self.nb_parameters = nb_parameters
        self.learning = learning
        self.sample_size = sample_size
        print('Rank {} | Initializing server...'.format(self.rank))
        self.node_name = socket.gethostname()
        self.launcher_node_name = launcher_node_name
        self.connection_port = connection_port
        self.text_pull_port = text_pull_port
        self.text_push_port = text_push_port
        self.text_request_port = text_request_port
        self.linger = linger

        self.data_puller_port = str(data_puller_port) + str(self.rank)
        self.data_puller_port_name = "tcp://{}:{}".format(
            self.node_name, self.data_puller_port,
        )
        self.port_names = self.comm.allgather(self.data_puller_port_name)
        print('port_names', self.port_names)

        # 1. Set up sockets
        self.context = zmq.Context()
        # Simulations (REQ) <-> Server (REP)
        self.connection_responder = self.context.socket(zmq.REP)
        if (self.rank == 0):
            print('Rank {} binding..'.format(self.rank))
            addr = 'tcp://*:{port}'.format(port=self.connection_port)
            try:
                self.connection_responder.bind(addr)
            except Exception as e:
                print('Rank {} could not bind to {}'.format(self.rank, addr))
                raise e
        # Launcher (PUB) -> Server (SUB)
        self.text_puller = self.context.socket(zmq.SUB)
        self.text_puller.setsockopt(zmq.SUBSCRIBE, b"")
        self.text_puller.setsockopt(zmq.LINGER, self.linger)
        self.text_puller_port_name = "tcp://{}:{}".format(
            self.launcher_node_name, self.text_pull_port
        )
        self.text_puller.connect(self.text_puller_port_name)
        # Simulations (PUSH) -> Server (PULL)
        self.data_puller = self.context.socket(zmq.PULL)
        self.data_puller.setsockopt(zmq.RCVHWM, data_hwm)
        addr = "tcp://*:{}".format(self.data_puller_port)
        print('Connecting puller to: ', addr)
        self.data_puller.bind(addr)
        # Server (PUSH) -> Launcher (PULL)
        self.text_pusher = self.context.socket(zmq.PUSH)
        self.text_pusher.setsockopt(zmq.LINGER, self.linger)
        addr = "tcp://{node_name}:{port}".format(
            node_name=self.launcher_node_name, port=self.text_push_port
        )
        print('connected to {}'.format(addr))
        self.text_pusher.connect(addr)
        # Server (REQ) <-> Launcher (REP)
        self.text_requester = self.context.socket(zmq.REQ)
        self.text_requester.setsockopt(zmq.LINGER, self.linger)
        self.text_requester.connect("tcp://{}:{}".format(
            self.launcher_node_name, self.text_request_port)
        )
        # 2. Setup poller
        self.poller = zmq.Poller()
        self.poller.register(self.text_puller, zmq.POLLIN)
        self.poller.register(self.data_puller, zmq.POLLIN)
        
        # Keep track of simulations
        self.simulations = {}
        # TODO: maybe this should be an MPI gather on rank 0.
        self.verbose_level = 0
        # 2. Send node name to the launcher, get options and recover if necesary
        self._send_node_name()
        self.connection_request = None

        if restart:
            self._recover()
            self.first_connection_recieved = True
        else:
            # 3. Non-root nodes will wait for the root's info
            self.first_connection_recieved = False
            if (self.rank > 0) and not restart:
                print('rank: ', self.rank)
                self._wait_for_first_init()
                # self._get_options()

        if (self.rank == 0):
            self.stopping = False
            self.connection_listener_thread = Thread(
                target=self._listen_for_connections,
            )
            self.connection_listener_thread.start()


    def _listen_for_connections(self):
        poller = zmq.Poller()
        poller.register(self.connection_responder, zmq.POLLIN)
        print(f'Rank: {self.rank} | Listening for connections..')
        while True:
            if self.stopping == True:
                print(f'Rank: {self.rank} [Connection Listener Thread] Stopping.')
                break
            # 1. Poll sockets
            pr = poller.poll(timeout=100)
            sockets = dict(pr)
            # 2 Handle simulation connection message
            if (self.connection_responder in sockets
                and sockets[self.connection_responder] == zmq.POLLIN):
                msg = self.connection_responder.recv()
                print('Got connection request')
                rv = self.handle_simulation_connection(msg)
    
    def _send_node_name(self):
        """
        Send the node name to the launcher. This message serve as a signal
        the launcher that the server is online, and it can start simulations.
        """
        print('Sending node name..')
        msg = ServerNodeName(self.rank, self.node_name)
        self.text_pusher.send(msg.encode())
        print('Sent node name')
    
    def _wait_for_first_init(self):
        """
        Simulations will connect only to the root node. The root node
        will then broadcast the simulation's information to the other nodes.
        """
        print('Waithing for first connection..')
        try:
            request = None
            request = self.comm.bcast(request, root=0)
            self.client_comm_size = request.comm_size
        except Exception as e:
            print('Failed to recv simulation connection.')
            raise e
    
    def _get_options(self):
        """Get stats and options from the launcher"""
        request = 'options '.encode('utf-8')
        self.text_requester.send(request)
        response = self.text_requester.recv()
        self.handle_launcher_message(response)
        print(" === Options (not implemented yet) ===\n")

    def _request_simulation_metadata(self, simulation_id):
        # 1. Send request with simulation id to the launcher.
        print(f'Rank: {self.rank} | [Launcher] Requesting simulation metadata')
        request = 'simu_info {}'.format(simulation_id).encode('utf-8')
        self.text_requester.send(request)
        # 2. Recv response with simulation parameters.
        print(f'Rank: {self.rank} | [Launcher] Waiting launcher response')
        response = self.text_requester.recv()
        print(f'Rank: {self.rank} | [Launcher] Recieved metadata from launcher')
        job_details = JobDetails.from_msg(response, self.nb_parameters)
        # 3. Update simulation record.
        simulation = self.simulations.get(job_details.simulation_id)
        simulation.job_id = job_details.job_id
        simulation.parameters = job_details.parameters
    
    def _should_checkpoint(self):
        """
        Condition under which a checkpoint is to be initiated.
        """
        return False

    def _checkpoint(self):
        """
        Write simulation data to a file on pickle format.
        """
        path = '{}.pkl'.format(self.checkpoint_file)
        print('Checkpointing. Writing data to {}'.format(path))
        with open(path, 'wb') as f:
            pickle.dump(self.simulations, f)

    def _recover(self):
        path = '{}.pkl'.format(self.checkpoint_file)
        print('Recovering. Reading data from {}'.format(path))
        try:
            with open(path, 'rb') as f:
                self.simulations = pickle.load(self.simulations, f)
        except Exception as e:
            self.simulations = {}
            print('Could not load checkpoint.')
    
    def _check_progress(self):
        # TODO: implement a way of checking progress to determine
        # when it's time for the server to stop.
        pass

    def run(self, timeout=100):
        # Checkpoint
        if self._should_checkpoint():
            self._checkpoint()
            return Status.CHECKPOINT
        rv = None
        # 1. Poll sockets
        pr = self.poller.poll(timeout=timeout)
        sockets = dict(pr)
        # 2. Handle launcher message
        if (self.text_puller in sockets
            and sockets[self.text_puller] == zmq.POLLIN):
            msg = self.text_puller.recv()
            rv = self.handle_launcher_message(msg)
        # 3. Handle simulation data message
        if (self.data_puller in sockets
            and sockets[self.data_puller] == zmq.POLLIN):
            msg = self.data_puller.recv()
            rv = self.handle_simulation_data(msg)
        # 4. Check progress
        self._check_progress()
        return rv
    
    def stop(self):
        """
        Signal to the launcher that the study has ended.
        """
        if (self.rank == 0):
            self.stopping = True
            print(f'Rank: {self.rank} | [Stop] Stopping listener thread..')
            self.connection_listener_thread.join()
            print(f'Rank: {self.rank} | [Stop] Listener stoped.')
        # 1. Syn server threads
        self.comm.Barrier()
        print(f'Rank: {self.rank} | [Stop] Sending termination message..')
        # 2. Send msg to the launcher
        if (self.rank == 0):
            self.text_pusher.send(Stop().encode())
        print(f'Rank: {self.rank} | [Stop] Done')

    def handle_simulation_connection(self, msg):
        # 1. Deserialize connection message.
        request = ConnectionRequest.recv(msg)
        print(f'Rank: {self.rank} | [Connection] Recieved connection message from simulation {request.simulation_id}.')
        # 2. Broadcast if first connection.
        if not self.first_connection_recieved:
            print(f'Rank: {self.rank} | [Connection] Broadcasting first connection metadata...')
            self.comm.bcast(request, root=0)
            self.first_connection_recieved = True
        # 3. Send response with server metadata.
        print(f'Rank: {self.rank} | [Connection] Sending response to simulation {request.simulation_id}.')
        response = ConnectionResponse(self.comm_size, self.sobol_op,
                                      self.learning, self.nb_parameters,
                                      self.verbose_level, self.port_names)
        self.connection_responder.send(response.encode())
        print(f'Rank: {self.rank} | [Connection] Connection established with simulation {request.simulation_id}')
        return 'Connection'

    def handle_launcher_message(self, msg):
        msg_type = ctypes.c_int32.from_buffer_copy(msg[:4]).value
        print(f'Rank: {self.rank} | [Launcher] Recieved launcher message.')
        if msg_type == MessageType.JOB.value:
            job_details = JobDetails.from_msg(msg, self.nb_parameters)
            simulation = self.simulations.get(job_details.simulation_id)
            if not simulation:
                simulation = Simulation(job_details.simulation_id, self.fields,
                                        self.nb_time_steps)
                self.simulations[job_details.simulation_id] = simulation
            simulation.job_id = job_details.job_id
            simulation.parameters = job_details.parameters
            print(f'Rank: {self.rank} | [Launcher] New simulation '
                  f'{job_details.simulation_id} with '
                  f'parameters {job_details.parameters}')
        elif msg_type == MessageType.DROP.value:
            simulation_id = ctypes.c_int32.from_buffer_copy(msg[4:8]).value
            print(f'Rank: {self.rank} | [Launcher] Drop simulation {simulation_id}')
            job_id = msg[8:].decode()
            simulation = self.simulations.get(simulation_id)
            if simulation:
                del self.simulations[simulation_id]
            self.sample_size -= 1

    def handle_simulation_data(self, msg):
        """
        Parse and validate incoming data messages from simulations
        """
        # 1. Deserialize message
        simulation_data = SimulationData.from_msg(msg)
        simulation_id = simulation_data.simulation_id
        field = simulation_data.field_name
        print(("Rank: {} | Recieved timestep {} from rank {} of group {} "
              "(vect_size: {}, field: {})").format(
                  self.rank,
                  simulation_data.timestep,
                  simulation_data.client_rank,
                  simulation_data.simulation_id,
                  simulation_data.data_size,
                  field))
        # 2. Apply filters
        if not (0 <= simulation_data.timestep < self.nb_time_steps):
            print(f'Rank: {self.rank} | Bad timestamp: {simulation_data.timestep}')
            return None
        if field not in self.fields:
            print(f'Rank: {self.rank} | Bad field: {field}')
            return None
        # 3. Add simulation to the record if we don't have it yet.
        simulation = self.simulations.get(simulation_id)
        if simulation is None:
            simulation = Simulation(simulation_id, self.fields, self.nb_time_steps)
            self.simulations[simulation_id] = simulation
            # Notify the launcher of the connection
            msg = SimulationStatusMessage(simulation_id,
                                          SimulationStatus.RUNNING)
            print(f'Rank: {self.rank} | [Simulation Data] Notifying launcher of connection...')
            self.text_pusher.send(msg.encode())
        # 4. Validate timestep (ignore repeated messages)
        if simulation.timesteps[field][simulation_data.timestep] == 1:
            print('Rank: {} | Duplicate timestep: {} | field {} | simulationID: {}'.format(
                self.rank, simulation_data.timestep, field, simulation_id,
            ))
            return None
        else:
            simulation.mark_message_recieved(field, simulation_data.timestep)
        # 5. Request simulation parameters to the launcher.
        if simulation.parameters is None:
            print(f'Rank: {self.rank} | [Simulation Data] Requesting simulation metadata...')
            self._request_simulation_metadata(simulation_id)
        simulation_data.parameters = simulation.parameters
        # 6. Check if the simulation finished
        if simulation.finished():
            print(f'Rank: {self.rank} | Simulation {simulation_id} finished. Notifying launcher..')
            msg = SimulationStatusMessage(simulation_id,
                                          SimulationStatus.FINISHED)
            self.text_pusher.send(msg.encode())
        # 7. Return simulation data message if it has data.
        if simulation_data.data_size > 0:
            return simulation_data
        else:
            return None

    def all_done(self):
        # All simulations run
        if len(self.simulations) < self.sample_size:
            return False
        # All simulations finished or timedout
        for simulation in self.simulations.values():
            if not (simulation.finished() or simulation.crashed()):
                return False
        return True

    @property
    def status(self):
        connected = self.simulations.values()
        finished = [simulation for simulation in connected
                    if simulation.finished()]
        crashed = [simulation for simulation in connected
                   if simulation.crashed()]
        msgs_recieved = np.sum([simulation.timesteps[field]
                                for simulation in connected
                                for field in self.fields])
        missing_messages = np.sum([(1 - simulation.timesteps[field])
                                   for simulation in crashed
                                   for field in self.fields])
        return {
            'connected': len(connected),
            'finished': len(finished),
            'crashed': len(crashed),
            'messages_recieved': msgs_recieved,
            'messages_missing': missing_messages,
            'total_messages': msgs_recieved + missing_messages,
        }


if __name__ == '__main__':
    server = MelissaServer()
    while True:
        event = server.run()
