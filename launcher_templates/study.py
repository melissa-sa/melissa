import os

from options import *
from simulation import *

class Study:
    def __init__(self):
        self.options = Options()
        self.simulations = list()
        self.output = ""
        self.server_options = ""
        self.mpi_options = ""
        self.server_job_id = ""
        if (self.options.sobol_indices):
            self.nb_groups = self.options.sampling_size
            self.nb_simu = self.nb_groups*(self.options.nb_parameters+2)
            self.sobol = True
        else:
            self.sobol = False
            self.nb_groups = self.options.sampling_size
            self.nb_simu = self.nb_groups
        self.init()
        self.launch()

    def init(self):
        self.check_options()
        for i in range(self.options.sampling_size):
            self.simulations.append(Simulation(self.draw_parameter_set()))
        self.create_study()
        self.create_server_options()

    def launch(self):
        self.launch_server()

    def check_options(self):
        errors = 0
        if not os.path.isdir(self.options.home_path):
            print "error bad option: home_path: no such directory"
            errors += 1
        if self.options.nb_parameters < 1 or (self.sobol and self.options.nb_parameters < 2):
            print "error bad option: nb_parameters too small
            errors += 1
        if (len(self.options.range_min_param) != self.options.nb_parameters) or (len(self.options.range_max_param) != self.options.nb_parameters):
            print "error bad option: wrong dimension for range_min_param or range_max_param
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
        if self.options.draw_parameter == None:
            self.options.draw_parameter = np.random.uniform
        else:
            for i in range(self.options.nb_parameters):
                param_set[i] = self.options.draw_parameter(self.options.range_min[i], self.options.range_max[i])
        return param_set

    def create_server_options(self):
        op_str = ""
        for i in range(len(self.options.operations)):
            if (i < len(self.options.operations) - 1 and 1 < len(self.options.operations[i])):
                op_str += self.options.operations[i] + ":"
            else:
                op_str += self.options.operations[i]
        self.server_options = " -p " + str(options.nb_parameters)\
                            + " -s " + str(options.sampling_size)\
                            + " -t " + str(options.nb_time_steps)\
                            + " -o " + op_str\
                            + " -e " + str(options.threshold)\
                            + " -n " + str(socket.gethostname())

    # server #

    def launch_server(self):
        self.create_server_options()
        if self.options.launch_server != None:
            self.server_job_id = self.options.launch_server()
        else:
            call_bash("mpirun "+self.options.mpi_options+" -n "+str(self.options.nb_proc_server)+" "+self.options.server_path+"/server "+self.server_options+" &")
            self.server_job_id = call_bash("pidof "+self.options.server_path+"/server")[out]
            self.output += "Melissa job id: "+melissa_job_id
        return melissa_job_id

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

    def finalize(self, file_name = "melissa_master.out"):
        fichier=open(file_name, "w")
        fichier.write(self.value)
        fichier.close()
