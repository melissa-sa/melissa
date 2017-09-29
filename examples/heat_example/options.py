
"""
    User defined options module
"""
import os
import time
import numpy as np
import subprocess
import getpass
from matplotlib import pyplot as plt
from matplotlib import cm
from shutil import copyfile

USERNAME = getpass.getuser()
BUILD_WITH_MPI = '@BUILD_WITH_MPI@'
BUILD_EXAMPLES_WITH_MPI = '@BUILD_EXAMPLES_WITH_MPI@'
EXECUTABLE='heatc'
NP_SIMU = 1
NP_SERVER = 1

def draw_param_set():
    param_set = np.zeros(STUDY_OPTIONS['nb_parameters'])
    for i in range(STUDY_OPTIONS['nb_parameters']):
        param_set[i] = np.random.uniform(0, 1)
    return param_set

def launch_server(server):
    server.job_id = subprocess.Popen(('mpirun ' +
                                      ' -n '+str(NP_SERVER) +
                                      ' ' + server.path +
                                      '/server ' +
                                      server.cmd_opt +
                                      ' &').split()).pid

def restart_server(server):
    server.job_id = subprocess.Popen(('mpirun ' +
                                      ' -n ' + str(NP_SERVER) + ' ' +
                                      server.path + '/server ' +
                                      server.cmd_opt +
                                      ' &').split()).pid

def launch_simu(simulation):
    os.chdir('@CMAKE_BINARY_DIR@/examples/heat_example')
    if BUILD_EXAMPLES_WITH_MPI == 'ON':
        command = ' '.join(('mpirun',
                             simulation.mpi_options,
                             '-n',
                             str(NP_SIMU),
                             './'+EXECUTABLE,
                             str(simulation.rank),
                             str(simulation.group.rank),
                             ' '.join(str(i) for i in simulation.param_set)))
        print command
#        copyfile(GLOBAL_OPTIONS['working_directory']+'/server_name.txt' , './server_name.txt')
        simulation.job_id = subprocess.Popen(command.split()).pid
    else:
        command = ' '.join(('./'+executable,
                            str(simulation.rank),
                            str(simulation.group.rank),
                            ' '.join(str(i) for i in simulation.param_set)))
        print command
#        copyfile(GLOBAL_OPTIONS['working_directory']+'/server_name.txt' , './server_name.txt')
        simulation.job_id = subprocess.Popen(command.split()).pid

def launch_coupled_simu(simulation):
    os.chdir('@CMAKE_BINARY_DIR@/examples/heat_example')
    command = 'mpirun '
    for i in range(STUDY_OPTIONS['nb_parameters'] + 2):
        command += ' '.join((simulation.mpi_options,
                             '-n',
                             str(NP_SIMU),
                             './'+EXECUTABLE,
                             str(i),
                             str(simulation.group.rank),
                             ' '.join(str(j) for j in simulation.param_set[i]),
                             ': '))
    print command[:-2]
#    copyfile(GLOBAL_OPTIONS['working_directory']+'/server_name.txt' , './server_name.txt')
    simulation.job_id = subprocess.Popen(command[:-2].split()).pid

def check_job(job):
    if os.system('ps -p ' + str(job.job_id) + ' > /dev/null') == 0:
        job.job_status = 1
    else:
        job.job_status = 2

def check_load():
    time.sleep(1)

def kill_job(job):
    print 'killing job ...'
    os.system('kill '+str(job.job_id))

def heat_visu():
    os.chdir('@CMAKE_BINARY_DIR@/examples/heat_example')
    fig = list()
    nb_time_steps = str(STUDY_OPTIONS['nb_time_steps'])
    matrix = np.zeros((100,100))

    for i in range(STUDY_OPTIONS['sampling_size']):
        fig.append(plt.figure(len(fig)))
        file_name = 'sol000_0000'+str(i)+'.dat'
        file=open(file_name)
        value = 0
        for line in file:
            matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0][54:])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Temperature')
        fig[len(fig)-1].show()
        file.close()

    if (MELISSA_STATS['mean']):
        fig.append(plt.figure(len(fig)))
        file_name = 'results.heat_mean.'+nb_time_steps
        file=open(file_name)
        value = 0
        for line in file:
            matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Means')
        fig[len(fig)-1].show()
        file.close()

    if (MELISSA_STATS['variance']):
        fig.append(plt.figure(len(fig)))
        file_name = 'results.heat_variance.'+nb_time_steps
        file=open(file_name)
        value = 0
        for line in file:
            matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Variances')
        fig[len(fig)-1].show()
        file.close()

    if (MELISSA_STATS['min']):
        fig.append(plt.figure(len(fig)))
        file_name = 'results.heat_min.'+nb_time_steps
        file=open(file_name)
        value = 0
        for line in file:
            matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Minimum')
        fig[len(fig)-1].show()
        file.close()

    if (MELISSA_STATS['max']):
        fig.append(plt.figure(len(fig)))
        file_name = 'results.heat_max.'+nb_time_steps
        file=open(file_name)
        value = 0
        for line in file:
            matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Maximum')
        fig[len(fig)-1].show()
        file.close()

    if (MELISSA_STATS['threshold_exceedance']):
        fig.append(plt.figure(len(fig)))
        file_name = 'results.heat_threshold.'+nb_time_steps
        file=open(file_name)
        value = 0
        for line in file:
            matrix[int(value)/100, int(value)%100] = int(line.split('\n')[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Threshold exceedance')
        fig[len(fig)-1].show()
        file.close()

    if (MELISSA_STATS['quantile']):
        fig.append(plt.figure(len(fig)))
        file_name = 'results.heat_quantile.'+nb_time_steps
        file=open(file_name)
        value = 0
        for line in file:
            matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0])
            value += 1
        plt.pcolor(matrix,cmap=cm.coolwarm)
        plt.colorbar().set_label('Quantiles')
        fig[len(fig)-1].show()
        file.close()

    if (MELISSA_STATS['sobol_indices']):
        for param in range(STUDY_OPTIONS['nb_parameters']):
            fig.append(plt.figure(len(fig)))
            file_name = 'results.heat_sobol'+str(param)+'.'+nb_time_steps
            file=open(file_name)
            value = 0
            for line in file:
                matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0])
                value += 1
            plt.pcolor(matrix,cmap=cm.coolwarm)
            plt.colorbar().set_label('Sobol\'s indices '+str(param))
            fig[len(fig)-1].show()
            file.close()
        for param in range(STUDY_OPTIONS['nb_parameters']):
            fig.append(plt.figure(len(fig)))
            file_name = 'results.heat_sobol_tot'+str(param)+'.'+nb_time_steps
            file=open(file_name)
            value = 0
            for line in file:
                matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0])
                value += 1
            plt.pcolor(matrix,cmap=cm.coolwarm)
            plt.colorbar().set_label('Total order Sobol\'s indices '+str(param))
            fig[len(fig)-1].show()
            file.close()

    raw_input()

GLOBAL_OPTIONS = {}
GLOBAL_OPTIONS['user_name'] = USERNAME
GLOBAL_OPTIONS['working_directory'] = '@CMAKE_BINARY_DIR@/examples/heat_example'

STUDY_OPTIONS = {}
STUDY_OPTIONS['nb_parameters'] = 5
STUDY_OPTIONS['sampling_size'] = 3
STUDY_OPTIONS['nb_time_steps'] = 100
STUDY_OPTIONS['threshold_value'] = 0.7
STUDY_OPTIONS['field_names'] = ["heat"]

SERVER_OPTIONS = {}
SERVER_OPTIONS['timeout'] = 600

SIMULATIONS_OPTIONS = {}
SIMULATIONS_OPTIONS['coupling'] = True
SIMULATIONS_OPTIONS['mpi_options'] = ''
SIMULATIONS_OPTIONS['timeout'] = 300

MELISSA_STATS = {}
MELISSA_STATS['mean'] = True
MELISSA_STATS['variance'] = True
MELISSA_STATS['min'] = True
MELISSA_STATS['max'] = True
MELISSA_STATS['threshold_exceedance'] = False
MELISSA_STATS['quantile'] = True
MELISSA_STATS['sobol_indices'] = True

USER_FUNCTIONS = {}
USER_FUNCTIONS['create_study'] = None
USER_FUNCTIONS['draw_parameter_set'] = draw_param_set
USER_FUNCTIONS['create_simulation'] = None
USER_FUNCTIONS['create_group'] = None
if MELISSA_STATS['sobol_indices'] and SIMULATIONS_OPTIONS['coupling']:
    USER_FUNCTIONS['launch_simulation'] = launch_coupled_simu
else:
    USER_FUNCTIONS['launch_simulation'] = launch_simu
USER_FUNCTIONS['launch_server'] = launch_server
USER_FUNCTIONS['check_server_job'] = check_job
USER_FUNCTIONS['check_simulation_job'] = check_job
USER_FUNCTIONS['restart_server'] = restart_server
USER_FUNCTIONS['restart_simulation'] = None
USER_FUNCTIONS['check_scheduler_load'] = check_load
USER_FUNCTIONS['cancel_job'] = kill_job
USER_FUNCTIONS['postprocessing'] = None
USER_FUNCTIONS['finalize'] = None
