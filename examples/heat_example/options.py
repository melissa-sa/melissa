
"""
    User defined options module
"""

import json
import os
import time
import numpy as np
import subprocess
from matplotlib import pyplot as plt
from matplotlib import cm

BUILD_WITH_MPI = '@BUILD_WITH_MPI@'
BUILD_EXAMPLES_WITH_MPI = '@BUILD_EXAMPLES_WITH_MPI@'

def launch_simu(simulation):
    os.chdir('@CMAKE_BINARY_DIR@/examples/heat_example')
    if BUILD_EXAMPLES_WITH_MPI == 'ON':
        command = ' '.join(('mpirun',
                             simulation.mpi_options,
                             '-n',
                             str(simulation.nproc),
                             './'+simulation.executable,
                             str(simulation.sobol_id),
                             str(simulation.rank),
                             ' '.join(str(i) for i in simulation.param_set)))
        print command
        simulation.job_id = subprocess.Popen(command.split()).pid
    else:
        command = ' '.join(('./'+executable,
                            str(simulation.sobol_id),
                            str(simulation.rank),
                            ' '.join(str(i) for i in simulation.param_set)))
        print command
        simulation.job_id = subprocess.Popen(command.split()).pid

def launch_coupled_group(group):
    os.chdir('@CMAKE_BINARY_DIR@/examples/heat_example')
    command = 'mpirun '
    for i in range(STUDY_OPTIONS['nb_parameters'] + 2):
        command += ' '.join((group.mpi_options,
                             '-n',
                             str(group.nproc),
                             './'+group.executable,
                             str(i),
                             str(group.rank),
                             ' '.join(str(j) for j in group.param_set[i]),
                             ': '))
    print command[:-2]
    group.job_id = subprocess.Popen(command[:-2].split()).pid
    while group.status < 2:
        time.sleep(1)

def launch_sobol_group(group):
    for simulation in group.simulations:
        launch_simu(simulation)

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

#    for i in range(STUDY_OPTIONS['sampling_size']):
#        fig.append(plt.figure(len(fig)))
#        file_name = 'sol000_0000'+str(i)+'.dat'
#        file=open(file_name)
#        value = 0
#        for line in file:
#            matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0][54:])
#            value += 1
#        plt.pcolor(matrix,cmap=cm.coolwarm)
#        plt.colorbar().set_label('Temperature')
#        fig[len(fig)-1].show()
#        file.close()

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
GLOBAL_OPTIONS['home_path'] = '/home/tterraz'
GLOBAL_OPTIONS['user_name'] = 'tterraz'
GLOBAL_OPTIONS['working_directory'] = '/home/tterraz/avido/source/Melissa/build/examples/heat_example'

STUDY_OPTIONS = {}
STUDY_OPTIONS['nb_parameters'] = 5
STUDY_OPTIONS['range_min_param'] = np.zeros(STUDY_OPTIONS['nb_parameters'], float)
STUDY_OPTIONS['range_max_param'] = np.ones(STUDY_OPTIONS['nb_parameters'], float)
STUDY_OPTIONS['sampling_size'] = 3
STUDY_OPTIONS['nb_time_steps'] = 100
STUDY_OPTIONS['threshold_value'] = 0.7

SERVER_OPTIONS = {}
SERVER_OPTIONS['nb_proc'] = 3
SERVER_OPTIONS['path'] = '/home/tterraz/avido/source/Melissa/build/server'
SERVER_OPTIONS['mpi_options'] = ''
SERVER_OPTIONS['timeout'] = 600

SIMULATIONS_OPTIONS = {}
SIMULATIONS_OPTIONS['path'] = '/home/user/simu'
SIMULATIONS_OPTIONS['executable'] = 'heatc'
SIMULATIONS_OPTIONS['nb_proc'] = 1
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
MELISSA_STATS['sobol_indices'] = False

USER_FUNCTIONS = {}
USER_FUNCTIONS['create_study'] = None
USER_FUNCTIONS['draw_parameter'] = np.random.uniform
USER_FUNCTIONS['create_simulation'] = None
USER_FUNCTIONS['create_group'] = None
USER_FUNCTIONS['launch_simulation'] = launch_simu
USER_FUNCTIONS['launch_group'] = launch_coupled_group
USER_FUNCTIONS['launch_server'] = None
USER_FUNCTIONS['check_server_job'] = check_job
USER_FUNCTIONS['check_simulation_job'] = check_job
USER_FUNCTIONS['restart_server'] = None
USER_FUNCTIONS['restart_simulation'] = None
USER_FUNCTIONS['restart_group'] = None
USER_FUNCTIONS['check_scheduler_load'] = check_load
USER_FUNCTIONS['cancel_job'] = kill_job
USER_FUNCTIONS['postprocessing'] = None
USER_FUNCTIONS['finalize'] = None

#if os.path.isfile("options.json"):
#    file=open('options.json', 'r')
#    [str1, GLOBAL_OPTIONS,
#     str1, SERVER_OPTIONS,
#     str1, SIMULATIONS_OPTIONS,
#     str1, MELISSA_STATS] = json.load(file)
#    file.close()
