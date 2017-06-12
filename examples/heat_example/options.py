
import os
import numpy as np
from matplotlib import pyplot as plt
from matplotlib import cm

class Options:
    def __init__(self):
        # user #
        self.home_path                = "/home/tterraz"
        self.user_name                = "tterarz"
        # parameter sets #
        self.nb_parameters            = 5
        self.range_min_param          = np.zeros(self.nb_parameters,float)
        self.range_max_param          = np.ones(self.nb_parameters,float)
        self.sampling_size            = 10
        self.nb_time_steps            = 1
        # operations #
        self.mean                     = True
        self.variance                 = True
        self.min                      = True
        self.max                      = True
        self.threshold_exceedance     = True
        self.threshold                = 0.7
        self.quantile                 = True
        self.sobol_indices            = False
        # Melissa Server #
        self.nb_proc_server           = 3
        self.server_path              = self.home_path+"/avido/source/Melissa/build/server"
        self.mpi_options              = ""
        # simulations #
        self.simulation_path          = self.home_path+"/avido/source/Melissa/build/example/heat_example"
        self.simulation_executable    = "heatc"
        self.nb_proc_simulation       = 2
        self.coupling                 = 1
        # functions #
        self.create_study             = None
        self.draw_parameter           = np.random.uniform
        self.create_simulation        = None
        self.launch_simulation        = launch_simu
        self.reboot_simulation        = None
        self.check_simulation_job     = None
        self.check_simulation_timeout = None
        self.launch_server            = None
        self.reboot_server            = None
        self.wait_server_start        = None
        self.check_server_job         = None
        self.check_server_timeout     = None
        self.stats_visu               = heat_visu
        self.finalize                 = None

        # add all the options you need.


def launch_simu(simulation):
    os.chdir("@CMAKE_BINARY_DIR@/examples/heat_example")
    parameters = ""
    parameters += str(simulation.sobol_id) + " "
    parameters += str(simulation.id)
    for i in simulation.parameter_set:
        parameters += " " + str(i)
    print "mpirun -n "+str(simulation.options.nb_proc_simulation)+" ./"+simulation.options.simulation_executable+" "+parameters+" "
    if (simulation.options.sobol_indices and sobol_rank < nb_parameters+1):
        return os.system("mpirun -n "+str(simulation.options.nb_proc_simulation)+" ./"+simulation.options.simulation_executable+" "+parameters+" &")
    else:
        return os.system("mpirun -n "+str(simulation.options.nb_proc_simulation)+" ./"+simulation.options.simulation_executable+" "+parameters+" ")
    simulation.job_id = call_bash("pidof "+self.options.server_path+"/server")["out"].split()[0]
    os.system("cd -")

def heat_visu(options):
    os.chdir("@CMAKE_BINARY_DIR@/examples/heat_example")
    fig = list()
    matrix = np.zeros((100,100))

    if (options.mean):
        fig.append(plt.figure(len(fig)))
        file_name = "results.heat_mean."+str(options.nb_time_steps)
        file=open(file_name)
        value = 0
        for line in file:
            matrix [int(value)/100, int(value)%100] = float(line.split("\n")[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Means')
        fig[len(fig)-1].show()
        file.close()

    if (options.variance):
        fig.append(plt.figure(len(fig)))
        file_name = "results.heat_variance."+str(options.nb_time_steps)
        file=open(file_name)
        value = 0
        for line in file:
            matrix [int(value)/100, int(value)%100] = float(line.split("\n")[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Variances')
        fig[len(fig)-1].show()
        file.close()

    if (options.min or options.max):
        fig.append(plt.figure(len(fig)))
        file_name = "results.heat_min."+str(options.nb_time_steps)
        file=open(file_name)
        value = 0
        for line in file:
            matrix [int(value)/100, int(value)%100] = float(line.split("\n")[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Minimum')
        fig[len(fig)-1].show()
        file.close()
        fig.append(plt.figure(len(fig)))
        file_name = "results.heat_max."+str(options.nb_time_steps)
        file=open(file_name)
        value = 0
        for line in file:
            matrix [int(value)/100, int(value)%100] = float(line.split("\n")[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Maximum')
        fig[len(fig)-1].show()
        file.close()

    if (options.threshold_exceedance):
        fig.append(plt.figure(len(fig)))
        file_name = "results.heat_threshold."+str(options.nb_time_steps)
        file=open(file_name)
        value = 0
        for line in file:
            matrix [int(value)/100, int(value)%100] = int(line.split("\n")[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Threshold exceedance')
        fig[len(fig)-1].show()
        file.close()

    if (options.quantile):
        fig.append(plt.figure(len(fig)))
        file_name = "results.heat_quantile."+str(options.nb_time_steps)
        file=open(file_name)
        value = 0
        for line in file:
            matrix [int(value)/100, int(value)%100] = float(line.split("\n")[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Quantiles')
        fig[len(fig)-1].show()
        file.close()

    if (options.sobol_indices):
        for param in range(options.nb_parameters):
            fig.append(plt.figure(len(fig)))
            file_name = "results.heat_sobol"+str(param)+"."+str(options.nb_time_steps)
            file=open(file_name)
            value = 0
            for line in file:
                matrix [int(value)/100, int(value)%100] = float(line.split("\n")[0])
                value += 1
            plt.pcolor(matrix,cmap=cm.coolwarm)
            plt.colorbar().set_label('Sobol\'s indices '+str(param))
            fig[len(fig)-1].show()
            file.close()
        for param in range(options.nb_parameters):
            fig.append(plt.figure(len(fig)))
            file_name = "results.heat_sobol_tot"+str(param)+"."+str(options.nb_time_steps)
            file=open(file_name)
            value = 0
            for line in file:
                matrix [int(value)/100, int(value)%100] = float(line.split("\n")[0])
                value += 1
            plt.pcolor(matrix,cmap=cm.coolwarm)
            plt.colorbar().set_label('Total order Sobol\'s indices '+str(param))
            fig[len(fig)-1].show()
            file.close()

    raw_input()
