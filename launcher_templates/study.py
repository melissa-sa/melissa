
from threading import Thread
import os
import time
from ctypes import cdll, create_string_buffer
import socket
import subprocess
from options import *
from simulation import *

get_message = cdll.LoadLibrary('@CMAKE_BINARY_DIR@/utils/libget_message.so')

# Jobs and executions status
PENDING = 0
WAITING = 0
RUNNING = 1
FINISHED = 2
TIMEOUT = 4

simulations = list()
server = Server(GLOBAL_OPTIONS['working_directory'],
                SERVER_OPTIONS['mpi_options'],
                SERVER_OPTIONS['nb_proc'])
output = ''


def check_server_job(job_id):
    if USER_FUNCTIONS['check_server_job']:
        return USER_FUNCTIONS['check_server_job'](job_id)
    else:
        pass

def check_simu_job(job_id):
    if USER_FUNCTIONS['check_simulation_job']:
        return USER_FUNCTIONS['check_simulation_job'](job_id)
    else:
        pass


class Messenger(Thread):
    def __init__(self):
        Thread.__init__(self)
        self.running_study = True

    def run(self):
        global simulations
        global server
        global output
        last_server = 0
        get_message.init_message()
        while self.running_study:
            buff = create_string_buffer('\000' * 256)
            get_message.wait_message(buff)
            print 'message: '+buff.value
            last_server = time.time()
            message = buff.value.split()
            if message[0] == 'stop':
                with server.lock:
                    server.status = FINISHED # finished
            elif message[0] == 'timeout':
                if message[1] != '-1':
                    simu_id = int(message[1])
                    output += 'Simulation ' + simu_id + 'timeout\n'
                    with simulations[simu_id].lock:
                        pass
                    # TODO call reboot simu (timeout)
            elif message[0] == 'simu_state':
                with simulations[int(message[1])].lock:
                    simulations[int(message[1])].status = int(message[2])

            if last_server > 0:
                if (time.time() - last_server) > SERVER_OPTIONS['timeout']:
                    output += 'Server ' + simu_id + 'timeout\n'
                    with server.lock:
                        server.status = TIMEOUT

        get_message.close_message()
        print 'closing messenger thread'

class StateChecker(Thread):
    def __init__(self):
        Thread.__init__(self)
        self.running_study = True

    def run(self):
        global simulations
        global server
        while self.running_study:
            time.sleep(1)
            with server.lock:
                print 'check server state (pid = '+str(server.job_id)+')'
                server.job_status = check_server_job(server.job_id)
                print 'server ' + str(server.job_status)
            for simu in simulations:
                if simu.job_status < 2:
                    print 'check simu '+str(simu.rank)+' state...'
                    with simu.lock:
                        simu.job_status = check_simu_job(simu.job_id)
                        print 'simu ' + str(simu.rank) + \
                                ' state ' + str(server.job_status)
        print 'closing state checker process'


class Study:
    def __init__(self):
        global simulations
        global server
        global output
        self.groups = list()
        self.simulations = simulations
        self.output = output
        self.server = server
        self.nb_groups = STUDY_OPTIONS['sampling_size']
        if MELISSA_STATS['sobol_indices']:
            self.sobol = True
            self.nb_simu = self.nb_groups*(STUDY_OPTIONS['nb_parameters']+2)
        else:
            self.sobol = False
            self.nb_simu = self.nb_groups
        self.messenger = Messenger()
        self.state_checker = StateChecker()


    def run(self):
        self.check_options()
        self.create_job_lists()
        self.create_study()
        self.server.command_line_options = self.create_server_options()
        self.launch_server()
        self.wait_server_start()
        self.server.status = 1
        self.server.job_status = 1
        self.messenger.start()
        self.state_checker.start()
        for simu in self.simulations:
            self.check_server_state()
            self.check_scheduler_load()
            if self.sobol:
                self.launch_group(simu)
            else:
                self.launch_simulation(simu)
        self.fault_tolerance()
        self.messenger.running_study = False
        self.state_checker.running_study = False
        self.messenger.join()
        self.state_checker.join()
        self.postprocessing()
        self.finalize()

    def check_options(self):
        errors = 0
        nb_parameters = STUDY_OPTIONS['nb_parameters']
        if not os.path.isdir(GLOBAL_OPTIONS['home_path']):
            print 'error bad option: home_path: no such directory'
            errors += 1
        if not self.sobol and nb_parameters < 1:
            print 'error bad option: nb_parameters too small'
            errors += 1
        if self.sobol and nb_parameters < 2:
            print 'error bad option: nb_parameters too small'
            errors += 1
        if len(STUDY_OPTIONS['range_min_param']) != nb_parameters:
            print 'error bad option: wrong dimension for range_min_param'
            errors += 1
        if len(STUDY_OPTIONS['range_max_param']) != nb_parameters:
            print 'error bad option: wrong dimension for range_max_param'
            errors += 1
        if STUDY_OPTIONS['sampling_size'] < 2:
            print 'error bad option: sample_size not big enough'
            errors += 1
        if not os.path.isdir(SERVER_OPTIONS['path']):
            print 'error bad option: server_path: no such directory'
            errors += 1
        elif not os.path.isfile(SERVER_OPTIONS['path']+'/server'):
            print 'error bad option: server_path: wrong directory'
            errors += 1
        return errors

    def create_job_lists(self):
        for i in range(STUDY_OPTIONS['sampling_size']):
            if self.sobol and SIMULATIONS_OPTIONS['coupling']:
                self.groups.append(SobolCoupledGroup(
                    self.draw_parameter_set(),
                    self.draw_parameter_set(),
                    SIMULATIONS_OPTIONS['executable']))
            elif self.sobol:
                self.groups.append(SobolMultiJobsGroup(
                    self.draw_parameter_set(),
                    self.draw_parameter_set(),
                    SIMULATIONS_OPTIONS['executable']))
                for simu in self.group[-1].simulations:
                    self.simulations.append(simu)
            else:
                self.simulations.append(Simulation(
                    self.draw_parameter_set(),
                    SIMULATIONS_OPTIONS['executable'],
                    0))
        if self.sobol and SIMULATIONS_OPTIONS['coupling']:
            self.simulations = self.groups

    def create_study(self):
        if USER_FUNCTIONS['create_study']:
            return USER_FUNCTIONS['create_study']()
        else:
            pass

    def draw_parameter_set(self):
        param_set = np.zeros(STUDY_OPTIONS['nb_parameters'])
        draw_param = np.random.uniform
        if USER_FUNCTIONS['draw_parameter']:
            dram_param = USER_FUNCTIONS['draw_parameter']
        for i in range(STUDY_OPTIONS['nb_parameters']):
            param_set[i] = draw_param(STUDY_OPTIONS['range_min_param'][i],
                                      STUDY_OPTIONS['range_max_param'][i])
        return param_set

    def create_server_options(self):
        op_str = ':'.join([x for x in MELISSA_STATS if MELISSA_STATS[x]])
        if op_str == '':
            print 'error bad option: no operation given'
            return
        return ' '.join(('-o', op_str,
                         '-p', str(STUDY_OPTIONS['nb_parameters']),
                         '-s', str(STUDY_OPTIONS['sampling_size']),
                         '-t', str(STUDY_OPTIONS['nb_time_steps']),
                         '-e', str(STUDY_OPTIONS['threshold_value']),
                         '-n', str(socket.gethostname())))

    # server #

    def launch_server(self):
#        self.server.command_line_options = self.create_server_options()
        os.chdir(self.server.directory)
        if  USER_FUNCTIONS['launch_server']:
            print 'pas cool'
            return USER_FUNCTIONS['launch_server'](self.server)
        else:
            print 'launch server : '+'mpirun '+self.server.mpi_options + \
                    ' -n '+str(self.server.nproc) + \
                    ' ' + SERVER_OPTIONS['path'] + \
                    '/server ' + \
                    self.server.command_line_options + ' &'
            self.server.job_id = subprocess.Popen(
                ('mpirun '+self.server.mpi_options +
                 ' -n '+str(self.server.nproc) +
                 ' ' + SERVER_OPTIONS['path'] +
                 '/server ' +
                 self.server.command_line_options +
                 ' &').split()).pid
            self.server.first_job_id = self.server.job_id

    def restart_server(self):
        self.server.command_line_options += ' -r '+self.server.directory
        os.chdir(self.server.directory)
        if USER_FUNCTIONS['restart_server']:
            return USER_FUNCTIONS['restart_server'](self.server)
        else:
            self.server.job_id = subprocess.Popen(
                ('mpirun ' + self.server.mpi_options +
                 ' -n ' + str(self.server.nproc) + ' ' +
                 SERVER_OPTIONS['path'] + '/server ' +
                 self.server.command_line_options +
                 ' &').split()).pid

    def wait_server_start(self):
        if  USER_FUNCTIONS['wait_server_start']:
            return USER_FUNCTIONS['wait_server_start']()
        else:
            get_message.init_message()
            buff = create_string_buffer('\000' * 256)
            message = 'nothing'.split()
            while message[0] != 'server':
                get_message.wait_message(buff)
                message = buff.value.split()
                print 'message: '+buff.value
                if message[0] == 'stop':
                    return 0
            print 'server node name: '+message[1]
            print 'server job id: '+str(self.server.job_id)
            self.server.node_name = message[1]
            self.output += 'Melissa Server node name: ' + \
                           str(self.server.node_name) + '\n' +\
                           'Melissa Server job id: ' + \
                           str(self.server.job_id) + '\n'
            get_message.close_message()
            return 0

    def check_server_state(self):
        if self.server.status != RUNNING:
            for simu in self.simulations:
                if simu.job_status < FINISHED:
                    self.cancel_job(simu.job_id)
                    simu.job_status = 0
            print " =============== server status: "+str(self.server.status)
            self.restart_server()

    def check_scheduler_load(self):
        if USER_FUNCTIONS['check_scheduler_load']:
            return USER_FUNCTIONS['check_scheduler_load']()
        else:
            pass

    def cancel_job(self, job_id):
        if USER_FUNCTIONS['cancel_job']:
            return USER_FUNCTIONS['cancel_job'](job_id)
        else:
            pass

    def check_server_timeout(self):
        if USER_FUNCTIONS['check_server_timeout']:
            return USER_FUNCTIONS['check_server_timeout']()
        else:
            pass

    # simulations #

    def create_simulation(self, simulation):
        if USER_FUNCTIONS['create_simulation']:
            return USER_FUNCTIONS['create_simulation'](simulation)
        else:
            pass

    def create_group(self, simulation):
        if self.sobol:
            if USER_FUNCTIONS['create_group']:
                return USER_FUNCTIONS['create_group'](simulation)
            else:
                pass

    def launch_simulation(self, simulation):
        if USER_FUNCTIONS['launch_simulation']:
            return USER_FUNCTIONS['launch_simulation'](simulation)
        else:
            pass

    def launch_group(self, group):
        if USER_FUNCTIONS['launch_group']:
            return USER_FUNCTIONS['launch_group'](group)
        else:
            pass

    def check_simulation_states(self):
        for simu in self.simulations:
            pass

    def check_simulation_timeout(self, simulation):
        if USER_FUNCTIONS['check_simulation_timeout']:
            return USER_FUNCTIONS['check_simulation_timeout'](simulation)
        else:
            pass

    # fault tolerance #

    def fault_tolerance(self):
        while (not self.server.status == FINISHED
               and not all([i.status == FINISHED for i in self.simulations])):
            time.sleep(1)
#        while not(all(i == 2 for i in self.simu_states[0:self.nb_jobs])
#                and (self.server_state.value == 2)):
#            with self.server_state.get_lock():
#                print 'check server state'
#                self.server_state.value = check_server_job()
#            for i in range(self.nb_jobs):
#                if self.job_states[i] < 3 and self.job_states[i] > 0:
#                    print 'check simu '+str(i)+' state'
#                    state = self.check_simulation_job(self.simulation[i])
#                    with self.job_states.get_lock():
#                        self.job_states[i] = state


    # end #

    def postprocessing(self):
        if USER_FUNCTIONS['postprocessing']:
            return USER_FUNCTIONS['postprocessing']()
        else:
            pass

    def finalize(self, file_name='melissa_launcher.out'):
        if USER_FUNCTIONS['finalize']:
            return USER_FUNCTIONS['finalize']()
        else:
            pass
        fichier = open(file_name, 'w')
        fichier.write(self.output)
        fichier.close()

