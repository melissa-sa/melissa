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
    User defined scripts for Curie supercomputer
"""

WALLTIME_SERVER = 6:00:00
NODES_SERVER = 32
WALLTIME_SIMU = 20:00
NODES_GROUP = 32
EXECUTABLE='heatc'

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
    content += "#!/bin/bash
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
    content += "mkdir stats${SLURM_JOB_ID}.resu                                    \n"
    content += "cd stats${SLURM_JOB_ID}.resu                                       \n"
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
    content += "#MSUB -n "+str(16*NODES_GROUP)+"                                   \n"
    content += "#MSUB -o simu.%I.log                                               \n"
    content += "#MSUB -e simu.%I.err                                               \n"
    content += "#MSUB -T "+str(WALLTIME_SIMU)+"                                    \n"
    content += "#MSUB -A gen10064                                                  \n"
    content += "#MSUB -q standard                                                  \n"
    content += "date +\"%d/%m/%y %T\"                                              \n"
    content += command + "                                                         \n"
    content += "date +\"%d/%m/%y %T\"                                              \n"
    content += "exit $?                                                            \n"
    file.write(content)
    file.close()
    os.system("chmod 744 run_group.sh")

def launch_server(server):
    if (not os.path.isdir(GLOBAL_OPTIONS['working_directory'])):
        os.mkdir(GLOBAL_OPTIONS['working_directory'])
    os.chdir(GLOBAL_OPTIONS['working_directory'])
    create_run_server(server)
    proc = subprocess.Popen('ccc_msub -r Melissa "./run_server.sh"',
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE,
                                  shell=True,
                                  universal_newlines=True)
    # get the job ID
    (out, err) = proc.communicate()
    server.job_id = out.split()[-1]
    os.chdir(GLOBAL_OPTIONS['working_directory'])

def launch_simu(simulation):
    if (not os.path.isdir(GLOBAL_OPTIONS['working_directory']+"/simu"+str(simulation.rank))):
        os.mkdir(GLOBAL_OPTIONS['working_directory']+"/simu"+str(simulation.rank))
    os.chdir(GLOBAL_OPTIONS['working_directory']+"/simu"+str(simulation.rank))
    copyfile(GLOBAL_OPTIONS['working_directory']+'/server_name.txt' , './server_name.txt')
    if MELISSA_STATS['sobol_indices']:
        command = 'mpirun '
        for i in range(STUDY_OPTIONS['nb_parameters'] + 2):
            command += ' '.join(('-n',
                                 str(NODES_GROUP),
                                 '@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/bin/'+EXECUTABLE,
                                 str(simulation.simu_id[i]), str(simulation.coupling),
                                 ' '.join(str(j) for j in simulation.param_set[i]),
                                 ': '))
    else:
        command = ' '.join(('mpirun',
                             '-n',
                             str(NODES_GROUP),
                             '@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/bin/'+EXECUTABLE,
                             str(simulation.simu_id), str(simulation.coupling),
                             ' '.join(str(i) for i in simulation.param_set)))

    print command[:-2]
    create_run_group(simulation, command)
    proc = subprocess.Popen('ccc_msub -r Simu'+str(simulation.rank)+' "./run_group.sh"',
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE,
                                  shell=True,
                                  universal_newlines=True)
    # get the job ID
    (out, err) = proc.communicate()
    simulation.job_id = out.split()[-1]
    os.chdir(GLOBAL_OPTIONS['working_directory'])

def check_job(job):
    state = 0
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
    os.system("scancel "+str(job.job_id))

def heat_visu():
    pass
