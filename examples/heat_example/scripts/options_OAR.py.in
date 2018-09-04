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
    User defined scripts for Grid5000
"""

EXECUTABLE='heatc'
NODES_SERVER = 3
NODES_GROUP = 2
WALLTIME_SERVER = "0:10:00"
WALLTIME_SIMU = "0:05:00"
NODES_SIMU = NODES_GROUP
PROC_PER_NODE = 4


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
    content += "#!/bin/bash                                                           \n"
    content += "#OAR -l nodes="+str(NODES_SERVER)+",walltime="+str(WALLTIME_SERVER)+" \n"
    content += "#OAR -O melissa.%jobid%.log                                           \n"
    content += "#OAR -E melissa.%jobid%.err                                           \n"
    content += "#OAR -n Melissa                                                       \n"
    content += "#OAR --checkpoint 300                                                 \n"
#    content += "#OAR --signal=SIGUSR2                                                 \n"
    content += "ulimit -s unlimited                                                   \n"
#    content += signal_handler
    content += "date +\"%d/%m/%y %T\"                                                 \n"
    content += "STOP=0                                                                \n"
    content += "source @CMAKE_INSTALL_PREFIX@/melissa_set_env.sh                      \n"
    content += "# run Melissa                                                         \n"
    content += "echo  \"### Launch Melissa\"                                          \n"
    content += "mkdir stats${OAR_JOB_ID}.resu                                         \n"
    content += "cd stats${OAR_JOB_ID}.resu                                            \n"
    content += "trap handler USR2                                                     \n"
    content += "mpirun "+server.path+"/melissa_server "+server.cmd_opt+" &            \n"
    content += "wait %1                                                               \n"
    content += "date +\"%d/%m/%y %T\"                                                 \n"
    content += "cd ..                                                                 \n"
    file.write(content)
    file.close()
    os.system("chmod 744 run_server.sh")

def create_run_simu(simulation, command):
    content = ""
    file=open("run_simu.sh", "w")
    content += "#!/bin/bash                                                     \n"
    content += "#OAR -l nodes="+str(NODES_SIMU)+"                              \n"
    content += "#OAR -s ulimit=unlimited                                        \n"
    content += "#OAR -O simu.%jobid%.log                                             \n"
    content += "#OAR -E simu.%jobid%.err                                             \n"
    content += "source @CMAKE_INSTALL_PREFIX@/melissa_set_env.sh                \n"
    content += "date +\"%d/%m/%y %T\"                                           \n"
    content += command + "                                                      \n"
    content += "date +\"%d/%m/%y %T\"                                           \n"
    content += "exit $?                                                         \n"
    file.write(content)
    file.close()
    os.system("chmod 744 run_simu.sh")

def launch_server(server):
    if (not os.path.isdir(STUDY_OPTIONS['working_directory'])):
        os.mkdir(STUDY_OPTIONS['working_directory'])
    os.chdir(STUDY_OPTIONS['working_directory'])
    create_run_server(server)
    proc = subprocess.Popen('oarsub -S "./run_server.sh"',
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE,
                                  shell=True,
                                  universal_newlines=True)
    # get the job ID
    (out, err) = proc.communicate()
    if len(out.split()) > 0:
        server.job_id = out.split("OAR_JOB_ID=")[1]
    else:
        print err
    os.chdir(STUDY_OPTIONS['working_directory'])

def launch_simu(simulation):
    if (not os.path.isdir(STUDY_OPTIONS['working_directory']+"/simu"+str(simulation.rank))):
        os.mkdir(STUDY_OPTIONS['working_directory']+"/simu"+str(simulation.rank))
    os.chdir(STUDY_OPTIONS['working_directory']+"/simu"+str(simulation.rank))
    copyfile(STUDY_OPTIONS['working_directory']+'/server_name.txt' , './server_name.txt')
    if MELISSA_STATS['sobol_indices']:
        command = 'mpirun '
        for i in range(STUDY_OPTIONS['nb_parameters'] + 2):
            command += ' '.join(('-n',
                                 str(PROC_PER_NODE),
                                 '@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/bin/'+EXECUTABLE,
                                 str(simulation.simu_id[i]), str(simulation.coupling),
                                 ' '.join(str(j) for j in simulation.param_set[i]),
                                 ': '))
    else:
        command = ' '.join(('mpirun',
                             '-n',
                             str(PROC_PER_NODE),
                             '@CMAKE_INSTALL_PREFIX@/share/examples/heat_example/bin/'+EXECUTABLE,
                             str(simulation.simu_id), str(simulation.coupling),
                             ' '.join(str(i) for i in simulation.param_set)))
    print command[:-2]
    create_run_simu(simulation, command)
    proc = subprocess.Popen('oarsub -S "./run_simu.sh"',
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE,
                                  shell=True,
                                  universal_newlines=True)
    # get the job ID
    (out, err) = proc.communicate()
    if len(out.split()) > 0:
        simulation.job_id = out.split("OAR_JOB_ID=")[1]
    else:
        print err
    os.chdir(STUDY_OPTIONS['working_directory'])

def check_job(job):
    state = 0
    time.sleep(1)
    proc = subprocess.Popen("oarstat -s -j "+str(job.job_id),
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            shell=True,
                            universal_newlines=True)
    time.sleep(1)
    (out, err) = proc.communicate()
    if "Waiting" in out:
        state = 0
    elif "Running" in out:
        state = 1
    else:
        state = 2
    job.job_status = state

def check_load():
    return True

def kill_job(job):
    print 'killing job ...'
    os.system("oardel "+str(job.job_id))

def heat_visu():
    pass
