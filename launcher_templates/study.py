
import os
import time
from ctypes import cdll, create_string_buffer
import socket
from multiprocessing import Process, Value, Array
from options import *
from utils import *
from simulation import *

get_message = cdll.LoadLibrary("@CMAKE_BINARY_DIR@/utils/libget_message.so")

def message_reciever(simu_states, server_state, running_study, server_timeout):
    get_message.init_message()
    while running_study.value == 1:
        buffer = create_string_buffer('\000' * 256)
        get_message.wait_message(buffer)
        print "message: "+buffer.value
        last_recieved_from_server = time.time()
        message = buffer.value.split()
        if message[0] == "stop":
            with server_state.get_lock():
                server_state = 3 # finished
        elif message[0] == "timeout":
            if message[1] != "-1":
                simu_id = int(message[1])
                # TODO call reboot simu (timeout)
        elif (message[0] == "simu_state"):
            with simu_states.get_lock():
                simu_states[int(message[1])] = int(message[2])

        if last_recieved_from_server > 0:
            if (time.time() - last_recieved_from_server) > server_timeout:
                with server_state.get_lock():
                    server_state.value = -1 # timeout

    get_message.close_message()
    print "closing messenger process"

#def state_checker(simu_job_states, server_job_state, running_study):
#    while running_study.value == 1:
#        time.sleep(10)
#        with server_state.get_lock():
#            print "check server state..."
#            server_state = check_server_job()
#            print "server "+server_state
#        for i in range(len(simu_job_id)):
#            if job_states[i] < 3 and job_states[i] > 0:
#                print "check simu "+str(i)+" state..."
#                state = check_simulation_job(global_options.batch_scheduler, global_options.username, simu_job_id[i])
#                if ("running" == state):
#                    with simu_job_states.get_lock():
#                        simu_job_states[i] = 2 # running
#                    print "Simulation "+str(i)+" running"
#                if ("terminated" == state):
#                    with simu_job_states.get_lock():
#                        simu_job_states[i] = 3 # terminated
#                    print "Simulation "+str(i)+" terminated"
#        pass
#    print "closing state checker process"

class Study:
    def __init__(self):
        self.options = Options()
        self.simulations = list()
        self.groups = list()
        self.output = ""
        self.server_options = ""
        self.mpi_options = ""
        self.server_node_name = ""
        self.server_job_id = ""
        self.server_first_job_id = ""
        self.server_node_name = ""
        if (self.options.sobol_indices):
            self.sobol = True
            self.nb_groups = self.options.sampling_size
            self.nb_simu = self.nb_groups*(self.options.nb_parameters+2)
            if self.options.coupling == 1:
                self.simu_states_size = self.options.sampling_size + self.options.max_additional_samples
            else:
                self.simu_states_size = self.options.sampling_size + self.options.max_additional_samples*(self.options.nb_parameters+2)
        else:
            self.sobol = False
            self.nb_groups = self.options.sampling_size
            self.nb_simu = self.nb_groups
            self.simu_states_size = self.options.sampling_size + self.options.max_additional_samples
        self.simu_job_states = Array('i', self.simu_states_size, lock = True) # simu as seen by the batch scheduler
        self.simu_states = Array('i', self.simu_states_size, lock = True) # simu as seen by the Server
        self.server_job_state = Value('i', lock = True)
        self.server_job_state.value = 0
        self.server_state = Value('i', lock = True)
        self.server_state.value = 0
        self.running_study = Value('i', lock = True)
        self.running_study.value = 1
        self.message_reciever = Process(target=message_reciever, args=(self.simu_states,
                                                                       self.server_state,
                                                                       self.running_study,
                                                                       self.options.server_timeout))
#        self.state_checker = Process(target=state_checker, args=(self.simu_job_states,
#                                                                 self.server_job_state,
#                                                                 self.running_study))

        self.check_options()
        for i in range(self.options.sampling_size):
            if self.sobol:
                self.groups.append(Sobol_group(parameter_set_A=self.draw_parameter_set(),
                                               parameter_set_B=self.draw_parameter_set(),
                                               options=self.options))
            else:
                self.simulations.append(Simulation(parameter_set=self.draw_parameter_set(),
                                                   options=self.options,
                                                   sobol_id=0))
        self.create_study()
        self.create_server_options()
        self.launch_server()
        self.server_job_state.value = 1
        self.wait_server_start()
        self.server_state.value = 1
        self.message_reciever.start()
#        self.state_checker.start()
        if self.sobol:
            for group in self.groups:
                self.launch_simulation_group(group)
                self.check_server_state()
#                self.check_simulation_states()
                self.check_scheduler_load()
        else:
            for simu in self.simulations:
                self.launch_simulation(simu)
                self.check_server_state()
#                self.check_simulation_states()
                self.check_scheduler_load()

        self.fault_tolerance()
        with self.running_study.get_lock():
            self.running_study.value = 0
        self.stats_visu()
        self.message_reciever.join()
#        self.state_checker.join()
        self.finalize()

    def check_options(self):
        errors = 0
        if not os.path.isdir(self.options.home_path):
            print "error bad option: home_path: no such directory"
            errors += 1
        if self.options.nb_parameters < 1 or (self.sobol and self.options.nb_parameters < 2):
            print "error bad option: nb_parameters too small"
            errors += 1
        if (len(self.options.range_min_param) != self.options.nb_parameters) or (len(self.options.range_max_param) != self.options.nb_parameters):
            print "error bad option: wrong dimension for range_min_param or range_max_param"
            errors += 1
        if self.options.sampling_size < 2:
            print "error bad option: sample_size not big enough"
            errors += 1
        if not os.path.isdir(self.options.server_path):
            print "error bad option: server_path: no such directory"
            errors += 1
        elif not os.path.isfile(self.options.server_path+"/server"):
            print "error bad option: server_path: wrong directory"
            errors += 1
        return errors


    def create_study(self):
        if self.options.create_study != None:
            return self.options.create_study()
        else:
            pass

    def draw_parameter_set(self):
        param_set = np.zeros(self.options.nb_parameters)
        if self.options.draw_parameter == None:
            self.options.draw_parameter = np.random.uniform
        else:
            for i in range(self.options.nb_parameters):
                param_set[i] = self.options.draw_parameter(self.options.range_min_param[i], self.options.range_max_param[i])
        return param_set

    def create_server_options(self):
        op_str = ""
        if self.options.mean == True:
            op_str += "mean:"
        if self.options.variance == True:
            op_str += "variance:"
        if self.options.min == True or self.options.max == True:
            op_str += "min:max:"
        if self.options.threshold_exceedance == True:
            op_str += "threshold:"
        if self.options.quantile == True:
            op_str += "quantile:"
        if self.options.sobol_indices == True:
            op_str += "sobol:"
        op_str = op_str[:-1]
        self.server_options = " -p " + str(self.options.nb_parameters)\
                            + " -s " + str(self.options.sampling_size)\
                            + " -t " + str(self.options.nb_time_steps)\
                            + " -o " + op_str\
                            + " -e " + str(self.options.threshold)\
                            + " -n " + str(socket.gethostname())

    # server #

    def launch_server(self):
        self.create_server_options()
        os.chdir(self.options.working_directory)
        if self.options.launch_server != None:
            self.server_job_id = self.options.launch_server()
        else:
            os.system("mpirun "+self.options.mpi_options+" -n "+str(self.options.nb_proc_server)+" "+self.options.server_path+"/server "+self.server_options+" &")

    def wait_server_start(self):
        if self.options.wait_server_start != None:
            return self.options.wait_server_start()
        else:
            get_message.init_message()
            buffer = create_string_buffer('\000' * 256)
            message = "nothing".split()
            while message[0] != "server":
                get_message.wait_message(buffer)
                message = buffer.value.split()
                print "message: "+buffer.value
                if message[0] == "stop":
                    return 0
                if message[0] == "server":
                    print "server node name:"+message[1]
                    self.server_node_name = message[1]
                    self.server_job_id = call_bash("pidof "+self.options.server_path+"/server")["out"].split()[0]
                    self.output += "Melissa Server job id: "+self.server_job_id
                    get_message.close_message()
                    return 0

    def check_server_state(self):
        pass

    def check_scheduler_load(self):
        if self.options.check_scheduler_load != None:
            return self.options.check_scheduler_load()
        else:
            pass

    def check_server_job(self):
        if self.options.check_server_job != None:
            return self.options.check_server_job()
        else:
            pass

    def check_server_timeout(self):
        if self.options.check_server_timeout != None:
            return self.options.check_server_timeout()
        else:
            pass

    def stats_visu(self):
        if self.options.stats_visu != None:
            return self.options.stats_visu(self.options)
        else:
            pass

    def finalize(self, file_name = "melissa_master.out"):
        if self.options.finalize != None:
            return self.options.finalize()
        else:
            pass
        fichier=open(file_name, "w")
        fichier.write(self.output)
        fichier.close()

    # simulations #

    def create_simulation(self, simulation):
        if self.options.create_simulation != None:
            return self.options.create_simulation(simulation)
        else:
            pass

    def create_simulation_group(self, simulation):
        if self.sobol:
            if self.options.create_simulation != None:
                return self.options.create_simulation_group(simulation)
            else:
                pass

    def launch_simulation(self, simulation):
        self.output += "launch simulation "+str(simulation.id)
        if self.options.launch_simulation != None:
            return self.options.launch_simulation(simulation)
        else:
            pass

    def launch_simulation_group(self, group):
        self.output += "launch simulation "+str(group.id)
        if self.options.launch_simulation_group != None:
            return self.options.launch_simulation_group(group)
        else:
            pass

    def launch_simulation(self, simulation):
        self.output += "launch simulation "+str(simulation.id)
        if self.options.launch_simulation != None:
            return self.options.launch_simulation(simulation)
        else:
            pass

    def check_simulation_states(self):
        for simu in self.simulations:
            pass

    def check_simulation_job(self, simulation):
        if self.options.check_simulation_job != None:
            return self.options.check_simulation_job(simulation)
        else:
            pass

    def check_simulation_timeout(self, simulation):
        if self.options.check_simulation_timeout != None:
            return self.options.check_simulation_timeout(simulation)
        else:
            pass

    # fault tolerance #

    def fault_tolerance(self):
        pass

