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
import imp
from matplotlib import pyplot as plt
from matplotlib import cm
from shutil import copyfile

from options import *

EXECUTABLE='heatc'

# PARALLEL SERVER PBS CONFIGURATION
WALLTIME_SERVER = '30:00'
NODES_SERVER = 1
CPUS_PER_NODE_SERVER = 1
QUEUE_SERVER = 'PBS_QUEUE1'
TOT_CPUS_SERVER = NODES_SERVER * CPUS_PER_NODE_SERVER
# PARALLEL RUNNER PBS CONFIGURATION
WALLTIME_GROUP = '10:00'
NODES_GROUP = 1
CPUS_PER_NODE_GROUP = 1
QUEUE_GROUP = 'PBS_QUEUE1'
TOT_CPUS_GROUP = NODES_GROUP * CPUS_PER_NODE_GROUP

"""
    User defined scripts for Razor cluster
"""

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
    content += "#!/bin/bash                                \n"
    content += "#PBS -S /bin/bash                                               \n"
    content += "#PBS -N MelissaServer                                           \n"
    content += "#PBS -l select="+str(NODES_SERVER)+":ncpus="+str(CPUS_PER_NODE_SERVER)+":mpiprocs="+str(CPUS_PER_NODE_SERVER)+" \n"
    content += "#PBS -q "+str(QUEUE_SERVER)+"                                   \n"
    content += "#PBS -l walltime="+str(WALLTIME_SERVER)+"                       \n"
#    content += "#PBS -l place=scatter:excl                                      \n"
#    content += "#PBS -o melissa.server.${PBS_JOBID}.log                         \n"
#    content += "#PBS -e melissa.server.${PBS_JOBID}.err                         \n"
    content += "module purge                                                    \n"
#    content += "module load use.own                                             \n"
    content += "module load melissa/0.3                                         \n"
    content += "echo Working directory of job ${PBS_JOBID} is ${PBS_O_WORKDIR}  \n"
    content += "cd ${PBS_O_WORKDIR}                                             \n"
    content += signal_handler
    content += "date +\"%d/%m/%y %T\"                                              \n"
    content += "STOP=0                                                             \n"
    content += "# run Melissa                                                      \n"
    content += "echo  \"### Launch Melissa\"                                       \n"
    content += "mkdir stats${PBS_JOBID}.resu                                    \n"
    content += "cd stats${PBS_JOBID}.resu                                       \n"
    content += "trap handler USR2                                                  \n"
    content += "mpirun -n "+str(TOT_CPUS_SERVER)+" "+server.path+"/melissa_server "+server.cmd_opt+" & \n"
    content += "wait %1                                                            \n"
    content += "date +\"%d/%m/%y %T\"                                              \n"
    content += "cd ..                                                              \n"
    file.write(content)
    file.close()
    os.system("chmod 744 run_server.sh")

def create_run_group(simulation, command):
    content = ""
    file=open("run_group.sh", "w")
    content += "#!/bin/bash                                \n"
    content += "#PBS -S /bin/bash                                               \n"
    content += "#PBS -N MelissaRunner                                           \n"
    content += "#PBS -l select="+str(NODES_GROUP)+":ncpus="+str(CPUS_PER_NODE_GROUP)+":mpiprocs="+str(CPUS_PER_NODE_GROUP)+" \n"
    content += "#PBS -q "+str(QUEUE_GROUP)+"                                    \n"
    content += "#PBS -l walltime="+str(WALLTIME_GROUP)+"                        \n"
#    content += "#PBS -l place=scatter:excl                                      \n"
#    content += "#PBS -o melissa.simu.${PBS_JOBID}.log                           \n"
#    content += "#PBS -e melissa.simu.${PBS_JOBID}.err                           \n"
    content += "echo Working directory of job ${PBS_JOBID} is ${PBS_O_WORKDIR}  \n"
    content += "cd ${PBS_O_WORKDIR}                                             \n"
    content += "module purge                                                    \n"
#    content += "module load use.own                                             \n"
    content += "module load melissa/0.3                                         \n"
    content += "date +\"%d/%m/%y %T\"                                              \n"
    content += command + "                                                         \n"
    content += "date +\"%d/%m/%y %T\"                                              \n"
    content += "exit $?                                                            \n"
    file.write(content)
    file.close()
    os.system("chmod 744 run_group.sh")

def launch_server(server):
    if (not os.path.isdir(STUDY_OPTIONS['working_directory'])):
        os.mkdir(STUDY_OPTIONS['working_directory'])
    os.chdir(STUDY_OPTIONS['working_directory'])
    create_run_server(server)
    proc = subprocess.Popen('qsub "./run_server.sh"',
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE,
                                  shell=True,
                                  universal_newlines=True)
    # get the job ID
    (out, err) = proc.communicate()
    server.job_id = out.split()[-1]
    os.chdir(STUDY_OPTIONS['working_directory'])

def launch_simu(simulation):
    if (not os.path.isdir(STUDY_OPTIONS['working_directory']+"/simu"+str(simulation.rank))):
        os.mkdir(STUDY_OPTIONS['working_directory']+"/simu"+str(simulation.rank))
    os.chdir(STUDY_OPTIONS['working_directory']+"/simu"+str(simulation.rank))
    copyfile(STUDY_OPTIONS['working_directory']+'/server_name.txt' , './server_name.txt')
    os.system("chmod 744 server_name.txt")
    if MELISSA_STATS['sobol_indices']:
        command = 'mpirun '
        for i in range(STUDY_OPTIONS['nb_parameters'] + 2):
            command += ' '.join(('-n',
                                 str(TOT_CPUS_GROUP),
                                 'PATH_TO_MELISSA/install/share/examples/heat_example/bin/'+EXECUTABLE,
                                 str(simulation.simu_id[i]), str(simulation.coupling),
                                 ' '.join(str(j) for j in simulation.param_set[i]),
                                 ': '))
    else:
        command = ' '.join(('mpirun',
                             '-n',
                             str(TOT_CPUS_GROUP),
                             'PATH_TO_MELISSA/install/share/examples/heat_example/bin/'+EXECUTABLE,
                             str(simulation.simu_id), str(simulation.coupling),
                             ' '.join(str(i) for i in simulation.param_set)))
    print command[:-2]
    create_run_group(simulation, command)
    proc = subprocess.Popen('qsub "./run_group.sh"',
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE,
                                  shell=True,
                                  universal_newlines=True)
    # get the job ID
    (out, err) = proc.communicate()
    simulation.job_id = out.split()[-1]
    os.chdir(STUDY_OPTIONS['working_directory'])


def check_job(job):
    state = 0
    proc = subprocess.Popen("qstat "+str(job.job_id)+" | tail -n1 | awk '{print $5}'",
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            shell=True,
                            universal_newlines=True)
    (out, err) = proc.communicate()
    if (not "Q" in out):
        state = 1
        proc = subprocess.Popen("qstat "+str(job.job_id)+" | tail -n1 | awk '{print $5}'",
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                shell=True,
                                universal_newlines=True)
        (out, err) = proc.communicate()
        if (not "R" in out):
            state = 2
    job.job_status = state

def kill_job(job):
    print 'killing job ...'
    os.system("qdel "+str(job.job_id))

def heat_visu():
    pass
