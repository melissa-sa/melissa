###################################################################
#                            Melissa                              #
#-----------------------------------------------------------------#
#   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    #
#                                                                 #
# This source is covered by the BSD 3-Clause License.             #
# Refer to the  LICENCE file for further information.             #
#                                                                 #
#-----------------------------------------------------------------#
#  Original Contributors:                                         #
#    Theophile Terraz,                                            #
#    Bruno Raffin,                                                #
#    Alejandro Ribes,                                             #
#    Bertrand Iooss,                                              #
###################################################################


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
BUILD_WITH_MPI = '@BUILD_WITH_MPI@'.upper()
BUILD_WITH_FLOWVR = '@BUILD_WITH_FLOWVR@'.upper()
BUILD_EXAMPLES_WITH_MPI = '@BUILD_EXAMPLES_WITH_MPI@'.upper()
EXECUTABLE='heatc'
BATCH_SCHEDULER = "local"
WALLTIME_SERVER = 600
NODES_SERVER = 3
WALLTIME_SIMU = 300
NODES_GROUP = 2


def create_flowvr_group(executable, args, group_id, nb_proc_simu, nb_parameters):
    content = ""
    file=open("@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/scripts/flowvr_group.py", "r")
    content = file.read().substitute(args=str(args),
                                     group_id=str(group_id),
                                     np_simu=str(nb_proc_simu),
                                     nb_param=str(nb_parameters),
                                     executable=str(executable))
    file.close()
    file=open("create_group"+str(group_id)+".py", "w")
    file.write(content)
    file.close()

def create_run_server(server):
    # signal handler definition
    signal_handler="handler() {                            \n"
    signal_handler+="echo \"### CLEAN-UP TIME !!!\"        \n"
    signal_handler+="STOP=1                                \n"
    signal_handler+="sleep 1                               \n"
    signal_handler+="killall -USR1 melissa_server          \n"
    signal_handler+="wait %1                               \n"
    signal_handler+="}                                     \n"
    content = ""
    file=open("run_server.sh", "w")
    content += "#!/bin/bash                                                        \n"
    if (BATCH_SCHEDULER == "Slurm"):
        content += "#SBATCH -N "+str(NODES_SERVER)+"                                   \n"
        content += "#SBATCH --ntasks-per-node=14                                       \n"
        content += "#SBATCH --mem=0                                                    \n"
        content += "#SBATCH --time="+str(WALLTIME_SERVER)+"                            \n"
        content += "#SBATCH -o melissa.%j.log                                          \n"
        content += "#SBATCH -e melissa.%j.err                                          \n"
        content += "#SBATCH --job-name=Melissa                                         \n"
        content += "#SBATCH --signal=B:SIGUSR2@300                                     \n"
        content += "module load openmpi/2.0.1                                          \n"
        content += "module load ifort/2017                                             \n"
    elif (BATCH_SCHEDULER == "OAR"):
        content += "#OAR -l nodes="+str(NODES_SERVER)+",walltime="+str(WALLTIME_SERVER)+ "\n"
        content += "#OAR -O melissa.%jobid%.log                                        \n"
        content += "#OAR -E melissa.%jobid%.err                                        \n"
        content += "#OAR -n Melissa                                                    \n"
        content += "#OAR --checkpoint 300                                              \n"
        content += "#OAR --signal=SIGUSR2                                              \n"
        content += "module load openmpi/1.8.5_gcc-4.4.6                                \n"
        content += "ulimit -s unlimited                                                \n"
        content += "export OMPI_MCA_orte_rsh_agent=oarsh                               \n"
    elif (BATCH_SCHEDULER == "CCC"):
        content += "#MSUB -n "+str(NODES_SERVER*16)+"                                  \n"
        content += "#MSUB -o melissa.%I.log                                            \n"
        content += "#MSUB -e melissa.%I.err                                            \n"
        content += "#MSUB -T "+str(WALLTIME_SERVER)+"                                  \n"
        content += "#MSUB -A gen10064                                                  \n"
        content += "#MSUB -r Melissa                                                   \n"
        content += "#MSUB -q standard                                                  \n"
        content += "#MSUB --signal=B:SIGUSR2@300                                       \n"
    content += signal_handler
    content += "date +\"%d/%m/%y %T\"                                              \n"
    content += "STOP=0                                                             \n"
    content += "# run Melissa                                                      \n"
    content += "echo  \"### Launch Melissa\"                                       \n"
    if (BATCH_SCHEDULER == "Slurm") or (BATCH_SCHEDULER == "CCC"):
        content += "mkdir stats${SLURM_JOB_ID}.resu                                    \n"
        content += "cd stats${SLURM_JOB_ID}.resu                                       \n"
    elif (BATCH_SCHEDULER == "OAR"):
        content += "mkdir stats${OAR_JOB_ID}.resu                                      \n"
        content += "cd stats${OAR_JOB_ID}.resu                                         \n"
    content += "trap handler USR2                                                  \n"
    content += "mpirun "+server.path+"/melissa_server "+server.cmd_opt+" &         \n"
    content += "wait %1                                                            \n"
    content += "date +\"%d/%m/%y %T\"                                              \n"
    content += "cd ..                                                              \n"
    file.write(content)
    file.close()
    os.system("chmod 744 run_server.sh")

def create_run_group(simulation, command):
    content = ""
    file=open("run_group.sh", "w")
    content += "#!/bin/bash                                                        \n"
    if (BATCH_SCHEDULER == "Slurm"):
        content += "#SBATCH -N "+str(NODES_GROUP)+"                                    \n"
        content += "#SBATCH --wckey=P11UK:AVIDO                                        \n"
        content += "#SBATCH --partition=cn                                             \n"
        content += "#SBATCH --time="+str(WALLTIME_SIMU)+"                              \n"
        content += "#SBATCH -o simu.%j.log                                          \n"
        content += "#SBATCH -e simu.%j.err                                          \n"
        content += "module load openmpi/2.0.1                                          \n"
        content += "module load ifort/2017                                             \n"
    elif (BATCH_SCHEDULER == "OAR"):
        content += "#OAR -l nodes="+str(NODES_GROUP)+",walltime="+str(WALLTIME_SIMU)+ "\n"
        content += "#OAR -O simu.%jobid%.log                                        \n"
        content += "#OAR -E simu.%jobid%.err                                        \n"
        content += "module load openmpi/1.8.5_gcc-4.4.6                                \n"
        content += "ulimit -s unlimited                                                \n"
    elif (BATCH_SCHEDULER == "CCC"):
        content += "#MSUB -n "+str(16*NODES_GROUP)+"                \n"
        content += "#MSUB -o simu.%I.log                                                               \n"
        content += "#MSUB -e simu.%I.err                                                               \n"
        content += "#MSUB -T "+str(WALLTIME_SIMU)+"                                                       \n"
        content += "#MSUB -A gen10064                                                                     \n"
        content += "#MSUB -q standard                                                                     \n"
    content += "date +\"%d/%m/%y %T\"                                              \n"
    content += command + "\n"
    content += "date +\"%d/%m/%y %T\"                                              \n"
    content += "exit $?                                                            \n"
    file.write(content)
    file.close()
    os.system("chmod 744 run_group.sh")

def draw_param_set():
    param_set = np.zeros(STUDY_OPTIONS['nb_parameters'])
    for i in range(STUDY_OPTIONS['nb_parameters']):
        param_set[i] = np.random.uniform(0, 1)
    return param_set

def launch_server(server):
    if (not os.path.isdir(GLOBAL_OPTIONS['working_directory'])):
        os.mkdir(GLOBAL_OPTIONS['working_directory'])
    os.chdir(GLOBAL_OPTIONS['working_directory'])
    if BATCH_SCHEDULER == "local":
        print 'mpirun ' + ' -n '+str(NODES_SERVER) + ' ' + server.path + '/melissa_server ' + server.cmd_opt + ' &'
        server.job_id = subprocess.Popen(('mpirun ' +
                                          ' -n '+str(NODES_SERVER) +
                                          ' ' + server.path +
                                          '/melissa_server ' +
                                          server.cmd_opt +
                                          ' &').split()).pid
    else:
        create_run_server(server)
        if (BATCH_SCHEDULER == "Slurm"):
            proc = subprocess.Popen('sbatch "./run_server.sh"',
                                          stdout=subprocess.PIPE,
                                          stderr=subprocess.PIPE,
                                          shell=True,
                                          universal_newlines=True)
            # get the job ID
            (out, err) = proc.communicate()
            server.job_id = out.split()[-1]
        elif (BATCH_SCHEDULER == "CCC"):
            proc = subprocess.Popen('ccc_msub -r Melissa "./run_server.sh"',
                                          stdout=subprocess.PIPE,
                                          stderr=subprocess.PIPE,
                                          shell=True,
                                          universal_newlines=True)
            # get the job ID
            (out, err) = proc.communicate()
            server.job_id = out.split()[-1]
        elif (BATCH_SCHEDULER == "OAR"):
            proc = subprocess.Popen('oarsub -S "./run_server.sh"',
                                          stdout=subprocess.PIPE,
                                          stderr=subprocess.PIPE,
                                          shell=True,
                                          universal_newlines=True)
            # get the job ID
            (out, err) = proc.communicate()
            server.job_id = out.split("OAR_JOB_ID=")[1]
    os.chdir(GLOBAL_OPTIONS['working_directory'])

def launch_simu(simulation):
    if MELISSA_STATS['sobol_indices']:
        if (not os.path.isdir(GLOBAL_OPTIONS['working_directory']+"/simu"+str(simulation.rank))):
            os.mkdir(GLOBAL_OPTIONS['working_directory']+"/simu"+str(simulation.rank))
        os.chdir(GLOBAL_OPTIONS['working_directory']+"/simu"+str(simulation.rank))
        copyfile(GLOBAL_OPTIONS['working_directory']+'/server_name.txt' , './server_name.txt')
        command = 'mpirun '
        for i in range(STUDY_OPTIONS['nb_parameters'] + 2):
            command += ' '.join(('-n',
                                 str(NODES_GROUP),
                                 '@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/bin/'+EXECUTABLE,
                                 str(simulation.simu_id[i]), str(simulation.coupling),
                                 ' '.join(str(j) for j in simulation.param_set[i]),
                                 ': '))
        print command[:-2]
        if BATCH_SCHEDULER == "local":
#            if BUILD_WITH_FLOWVR == 'ON':
#                args = []
#                for i in range(STUDY_OPTIONS['nb_parameters'] + 2):
#                    args.append(str(simulation.simu_id[i])+" "+' '.join(str(j) for j in simulation.param_set[i]))
#                create_flowvr_group('@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/bin/'+EXECUTABLE,
#                                    args,
#                                    simulation.rank,
#                                    int(NODES_GROUP),
#                                    STUDY_OPTIONS['nb_parameters'])
#                os.system('python create_group'+str(simulation.rank)+'.py')
#                simulation.job_id = subprocess.Popen('flowvr group'+str(simulation.rank), shell=True).pid
#            else:
                simulation.job_id = subprocess.Popen(command[:-2].split()).pid
        else:
            create_run_group(simulation, command)
            if (BATCH_SCHEDULER == "Slurm"):
                proc = subprocess.Popen('sbatch "./run_group.sh"',
                                              stdout=subprocess.PIPE,
                                              stderr=subprocess.PIPE,
                                              shell=True,
                                              universal_newlines=True)
                # get the job ID
                (out, err) = proc.communicate()
                simulation.job_id = out.split()[-1]
            elif (BATCH_SCHEDULER == "CCC"):
                proc = subprocess.Popen('ccc_msub -r Simu'+str(simulation.rank)+' "./run_group.sh"',
                                              stdout=subprocess.PIPE,
                                              stderr=subprocess.PIPE,
                                              shell=True,
                                              universal_newlines=True)
                # get the job ID
                (out, err) = proc.communicate()
            elif (BATCH_SCHEDULER == "OAR"):
                proc = subprocess.Popen('oarsub -S "./run_group.sh"',
                                              stdout=subprocess.PIPE,
                                              stderr=subprocess.PIPE,
                                              shell=True,
                                              universal_newlines=True)
                # get the job ID
                (out, err) = proc.communicate()
                simulation.job_id = out.split("OAR_JOB_ID=")[1]


    else:
        if (not os.path.isdir(GLOBAL_OPTIONS['working_directory']+"/simu"+str(simulation.rank))):
            os.mkdir(GLOBAL_OPTIONS['working_directory']+"/simu"+str(simulation.rank))
        os.chdir(GLOBAL_OPTIONS['working_directory']+"/simu"+str(simulation.rank))
        copyfile(GLOBAL_OPTIONS['working_directory']+'/server_name.txt' , './server_name.txt')
        if BATCH_SCHEDULER == "local":
            if BUILD_EXAMPLES_WITH_MPI == 'ON':
                command = ' '.join(('mpirun',
                                     '-n',
                                     str(NODES_GROUP),
                                     '@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/bin/'+EXECUTABLE,
                                     str(simulation.simu_id),
                                     ' '.join(str(i) for i in simulation.param_set)))
                print command
                simulation.job_id = subprocess.Popen(command.split()).pid
            else:
                command = ' '.join(('@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/bin/'+EXECUTABLE,
                                    str(0),
                                    str(simulation.rank),
                                    ' '.join(str(i) for i in simulation.param_set)))
                print command
                simulation.job_id = subprocess.Popen(command.split()).pid

    os.chdir(GLOBAL_OPTIONS['working_directory'])

def check_job(job):
    state = 0
    if BATCH_SCHEDULER == "local":
        try:
            subprocess.check_output(["ps",str(job.job_id)])
            state = 1
        except:
            state = 2
    elif (BATCH_SCHEDULER == "OAR"):
        proc = subprocess.Popen("oarstat -u --sql \"state = 'Waiting'\"",
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                shell=True,
                                universal_newlines=True)
        (out, err) = proc.communicate()
        if (not str(job.job_id) in out):
            state = 1
            proc = subprocess.Popen("oarstat -u --sql \"state = 'Running'\"",
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE,
                                    shell=True,
                                    universal_newlines=True)
            (out, err) = proc.communicate()
            if (not str(job.job_id) in out):
                state = 2
    elif (BATCH_SCHEDULER == "Slurm") or (BATCH_SCHEDULER == "CCC"):
        proc = subprocess.Popen("squeue --job="+str(job.job_id)+" -l",
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                shell=True,
                                universal_newlines=True)
        (out, err) = proc.communicate()
        if (not "PENDING" in out):
            state = 1
            proc = subprocess.Popen("squeue --job="+str(job.job_id)+" -l",
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE,
                                    shell=True,
                                    universal_newlines=True)
            (out, err) = proc.communicate()
            if (not "RUNNING" in out):
                state = 2
    job.job_status = state

def check_load():
    if BATCH_SCHEDULER == "local":
        try:
            subprocess.check_output(["pidof",EXECUTABLE])
            return False
        except:
            return True
    elif (BATCH_SCHEDULER == "Slurm") or (BATCH_SCHEDULER == "CCC"):
        proc = subprocess.Popen("squeue -u "+GLOBAL_OPTIONS['user_name']+" | wc -l",
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                shell=True,
                                universal_newlines=True)
        (out, err) = proc.communicate()
        running_jobs = int(out)
        return running_jobs < 250

def kill_job(job):
    print 'killing job ...'
    if BATCH_SCHEDULER == "local":
        os.system('kill '+str(job.job_id))
    elif (BATCH_SCHEDULER == "Slurm" or BATCH_SCHEDULER == "CCC"):
        os.system("scancel "+str(job.job_id))
    elif (BATCH_SCHEDULER == "OAR"):
        os.system("oardel "+str(job.job_id))

def heat_visu():
    if BATCH_SCHEDULER == "local":
        os.chdir('@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/STATS')
        fig = list()
        nb_time_steps = str(STUDY_OPTIONS['nb_time_steps'])
        matrix = np.zeros((100,100))

#        for i in range(STUDY_OPTIONS['sampling_size']):
#            fig.append(plt.figure(len(fig)))
#            file_name = 'sol000_0000'+str(i)+'.dat'
#            file=open(file_name)
#            value = 0
#            for line in file:
#                matrix[int(value)/100, int(value)%100] = float(line.split('\n')[0][54:])
#                value += 1
#            plt.pcolor(matrix,cmap=cm.coolwarm)
#            plt.colorbar().set_label('Temperature')
#            fig[len(fig)-1].show()
#            file.close()

        if (MELISSA_STATS['mean']):
            fig.append(plt.figure(len(fig)))
            file_name = 'results.heat1_mean.'+nb_time_steps
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
            file_name = 'results.heat1_variance.'+nb_time_steps
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
            file_name = 'results.heat1_min.'+nb_time_steps
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
            file_name = 'results.heat1_max.'+nb_time_steps
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
            file_name = 'results.heat1_threshold.'+nb_time_steps
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
            file_name = 'results.heat1_quantile.'+nb_time_steps
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
                file_name = 'results.heat1_sobol'+str(param)+'.'+nb_time_steps
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
                file_name = 'results.heat1_sobol_tot'+str(param)+'.'+nb_time_steps
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
GLOBAL_OPTIONS['working_directory'] = '@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/STATS'

STUDY_OPTIONS = {}
STUDY_OPTIONS['nb_parameters'] = 5                 # number of varying parameters of the study
STUDY_OPTIONS['sampling_size'] = 6                 # initial number of parameter sets
STUDY_OPTIONS['nb_time_steps'] = 100               # number of timesteps, from Melissa point of view
STUDY_OPTIONS['threshold_value'] = 0.7
STUDY_OPTIONS['field_names'] = ["heat1"]           # list of field names
STUDY_OPTIONS['simulation_timeout'] = 40           # simulations are restarted if no life sign for 40 seconds
STUDY_OPTIONS['checkpoint_interval'] = 30          # server checkpoints every 30 seconds
STUDY_OPTIONS['coupling'] = "MELISSA_COUPLING_MPI" # option for Sobol' simulation groups coupling

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
USER_FUNCTIONS['create_group'] = None
#if MELISSA_STATS['sobol_indices']:
#    USER_FUNCTIONS['launch_group'] = launch_group
#else:
USER_FUNCTIONS['launch_group'] = launch_simu
USER_FUNCTIONS['launch_server'] = launch_server
USER_FUNCTIONS['check_server_job'] = check_job
USER_FUNCTIONS['check_group_job'] = check_job
USER_FUNCTIONS['restart_server'] = launch_server
USER_FUNCTIONS['restart_group'] = None
USER_FUNCTIONS['check_scheduler_load'] = check_load
USER_FUNCTIONS['cancel_job'] = kill_job
USER_FUNCTIONS['postprocessing'] = @HEAT_VISU@
USER_FUNCTIONS['finalize'] = None
