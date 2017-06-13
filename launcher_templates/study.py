
import os
from ctypes import cdll, create_string_buffer
import socket
from threading import Thread, RLock
from options import *
from utils import *
from simulation import *

get_message = cdll.LoadLibrary("@CMAKE_BINARY_DIR@/launcher/libget_message.so")

class message_reciever(Thread):
    def __init__(self):
        Thread.__init__(self)

    def run(self):
        while True:
            message = create_string_buffer('\000' * 256)
            get_message.wait_message(message)
            print "message: "+message.value
            if message.value == "stop":
                return 0

class Study:
    def __init__(self):
        self.options = Options()
        self.simulations = list()
        self.output = ""
        self.server_options = ""
        self.mpi_options = ""
        self.server_job_id = ""
        self.server_first_job_id = ""
        self.server_node_name = ""
        if (self.options.sobol_indices):
            self.nb_groups = self.options.sampling_size
            self.nb_simu = self.nb_groups*(self.options.nb_parameters+2)
            self.sobol = True
        else:
            self.sobol = False
            self.nb_groups = self.options.sampling_size
            self.nb_simu = self.nb_groups
        self.thread_message_reciever = message_reciever()

        self.check_options()
        for i in range(self.options.sampling_size):
            self.simulations.append(Simulation(parameter_set=self.draw_parameter_set(),options=self.options, sobol_id=0))
        self.create_study()
        self.create_server_options()
        self.launch_server()
        self.wait_server_start()
        get_message.init_message()
        self.thread_message_reciever.start()
        for simu in self.simulations:
            self.launch_simulation(simu)
            self.check_server_state()
            self.options.check_scheduler_load()

        self.stats_visu()
        self.thread_message_reciever.join()
        get_message.close_message()
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
        if self.options.launch_server != None:
            self.server_job_id = self.options.launch_server()
        else:
            os.system("mpirun "+self.options.mpi_options+" -n "+str(self.options.nb_proc_server)+" "+self.options.server_path+"/server "+self.server_options+" &")

    def wait_server_start(self):
        if self.options.wait_server_start != None:
            return self.options.wait_server_start()
        else:
            get_message.init_message()
            c_msg = create_string_buffer('\000' * 256)
            message = "nothing".split()
            while message[0] != "server":
                get_message.wait_message(c_msg)
                message = c_msg.value.split()
                print "message: "+c_msg.value
                if message[0] == "stop":
                    return 0
                if message[0] == "server":
                    print "server node name:"+message[1]
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

    def launch_simulation(self, simulation):
        print "launch_simulation"+str(simulation.id)
        if self.options.launch_simulation != None:
            return self.options.launch_simulation(simulation)
        else:
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
