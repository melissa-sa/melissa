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

 WALLTIME_SERVER = 600
 NODES_SERVER = 3
 WALLTIME_SIMU = 300
 NODES_GROUP = 2


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
    content += "#OAR --signal=SIGUSR2                                                 \n"
    content += "module load openmpi/1.8.5_gcc-4.4.6                                   \n"
    content += "ulimit -s unlimited                                                   \n"
    content += "export OMPI_MCA_orte_rsh_agent=oarsh                                  \n"
    content += signal_handler
    content += "date +\"%d/%m/%y %T\"                                                 \n"
    content += "STOP=0                                                                \n"
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
    server.job_id = out.split("OAR_JOB_ID=")[1]
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
        print command
        simulation.job_id = subprocess.Popen(command.split()).pid
    print command[:-2]
    create_run_group(simulation, command)
    os.chdir(STUDY_OPTIONS['working_directory'])

def check_job(job):
    state = 0
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
    job.job_status = state

def check_load():
    return True

def kill_job(job):
    print 'killing job ...'
    os.system("oardel "+str(job.job_id))

def heat_visu():
    pass
