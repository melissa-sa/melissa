import os
import time
import sys
import subprocess
import numpy as np
import numpy.random as rd
import re
import zmq
from fault_tolerance import *
from matrix_sobol import *
from call_bash import *
from batch_scripts import *
from options import *
from threading import Thread, RLock


#=====================================#
#               options               #
#         (global variables)          #
#=====================================#

global_options = options()
timeout_server = 600
sys.path.append(global_options.home_path+"/../master")
server_state   = ""
melissa_job_id = ""
simu_job_id    = []
job_states     = np.zeros(global_options.sampling_size) # not submitted
simu_states    = np.zeros(global_options.sampling_size) # simu as seen by the Server


#=====================================#
#               ZeroMQ                #
#=====================================#

context = zmq.Context()
pull_melissa_socket = context.socket(zmq.PULL)
pull_melissa_socket.bind("tcp://*:5555")
poller = zmq.Poller()

#=====================================#
#               Threads               #
#=====================================#

lock_job_state = RLock()
lock_simu_state = RLock()
lock_server_state = RLock()

class state_checker(Thread):
    def __init__(self):
        Thread.__init__(self)
        self.running_master = True
    def run(self):
        global poller
        global job_states
        global server_state
        global global_options
        global melissa_job_id
        global simu_job_id
        while self.running_master == True:
            time.sleep(10)
            with lock_server_state:
                server_state = check_job(global_options.batch_scheduler, global_options.username, melissa_job_id)
            for i in range(len(simu_job_id)):
                if job_states[i] < 3 and job_states[i] > 0:
                    state = check_job(global_options.batch_scheduler, global_options.username, simu_job_id[i])
                    if ("running" == state):
                        with lock_job_state:
                            job_states[i] = 2 # running
                    if ("terminated" == state):
                        with lock_job_state:
                            job_states[i] = 3 # terminated

class message_reciever(Thread):
    def __init__(self):
        Thread.__init__(self)
        self.running_master = True
    def run(self):
        global poller
        global simu_states
        global simu_job_id
        global job_states
        global simu_states
        global server_state
        last_recieved_from_master = 0
        while self.running_master == True:
            socks = dict(poller.poll(1000))
            if (pull_melissa_socket in socks.keys() and socks[pull_melissa_socket] == zmq.POLLIN):
                message = pull_melissa_socket.recv_string().split()
                print "message: "+message[0]+" "+message[1]
                if (message[0] == "timeout"):
                    if message[1] != "-1":
                        with lock_job_state:
                            reboot_simu(int(simu), simu_job_id, global_options.batch_scheduler, global_options.workdir)
                            job_states[int(simu)] = 1 # pennding or runnning
                if (message[0] == "simu_state"):
                    simu_id = int(message[1])
                    simu_state = int(message[2])
                    simu_states[simu_id] = simu_state
                last_recieved_from_master = time.time()
            if last_recieved_from_master > 0:
                if (time.time() - last_recieved_from_master) > timeout_server:
                    with lock_server_state:
                        server_state = "timeout"

#=======================================#
#               functions               #
#=======================================#

def create_case (Ai, sobol_rank, sobol_group, workdir, xml_file_name):
    if (sobol_rank > 0):
        parameters = str(sobol_rank)+":"+str(sobol_group)
        casedir = workdir+"/group"+str(sobol_group)+"/rank"+str(sobol_rank)
    else:
        parameters = "0:"+str(sobol_group)
        casedir = workdir+"/group"+str(sobol_group)+"/rank0"
    os.system("cp "+workdir+"/case1/DATA/server_name.txt "+casedir+"/DATA")
    if (sobol_rank > 0):
        os.system("cp "+workdir+"/case1/SCRIPTS/run_saturne.sh "+casedir+"/SCRIPTS/runcase")
    else:
        os.system("cp "+workdir+"/case1/SCRIPTS/run_saturne_master.sh "+casedir+"/SCRIPTS/runcase")
    # modif xml file
    os.chdir(casedir+"/DATA")
    fichier=open(workdir+"/case1/DATA/"+xml_file_name, "r")
    contenu = ""
    for ligne in fichier:
        if not("melissa" in ligne):
            contenu += ligne
        else:
            contenu += re.sub('options=".*"','options="'+parameters+'"',ligne)
    fichier.close()
    fichier = open(xml_file_name, 'w')
    fichier.write(contenu)
    fichier.close()
    os.chdir(casedir+"/SRC")
    #modif fortran routine
    fichier = open(workdir+'/case1/SRC/cs_user_boundary_conditions.f90', 'r')
    contenu = ""
    for ligne in fichier:
        if ("param_intensite_haut=" in ligne):
            contenu += 'param_intensite_haut='+str(Ai[0])+'\n'
        elif ("param_intensite_bas=" in ligne):
            contenu += 'param_intensite_bas='+str(Ai[1])+'\n'
        elif ("param_largeur_haut=" in ligne):
            contenu += 'param_largeur_haut='+str(Ai[2])+'\n'
        elif ("param_largeur_bas=" in ligne):
            contenu += 'param_largeur_bas='+str(Ai[3])+'\n'
        elif ("param_duree_injection_haut=" in ligne):
            contenu += 'param_duree_injection_haut='+str(Ai[4])+'\n'
        elif ("param_duree_injection_bas=" in ligne):
            contenu += 'param_duree_injection_bas='+str(Ai[5])+'\n'
        else:
            contenu += ligne
    fichier.close()
    fichier = open('cs_user_boundary_conditions.f90', 'w')
    fichier.write(contenu)
    fichier.close()
    os.system("cp "+workdir+"/case1/SRC/cs_user_mesh.c "+casedir+"/SRC/cs_user_mesh.c")
    return 0

def launch_study():


    output = ""

    if (not (("sobol" in global_options.operations) or ("sobol_indices" in global_options.operations))):
        nb_simu = global_options.sampling_size
        nb_groups = global_options.sampling_size
    else:
        nb_groups = global_options.sampling_size
        nb_simu = nb_groups*(global_options.nb_parameters+2)

    op_str = ""
#    if (batch_scheduler == "Slurm"):
#        mpi_options = mpi_Slurm_options
#    elif (batch_scheduler == "CCC"):
#        mpi_options = mpi_CCC_options
#    elif (batch_scheduler == "OAR"):
#        mpi_options = mpi_OAR_options
    for i in range(len(global_options.operations)):
        if (i < len(global_options.operations) - 1 and 1 < len(global_options.operations[i])):
            op_str += global_options.operations[i] + ":"
        else:
            op_str += global_options.operations[i]
    options = " -p " + str(global_options.nb_parameters)\
            + " -s " + str(nb_simu)\
            + " -g " + str(nb_groups)\
            + " -t " + str(global_options.nb_time_steps)\
            + " -o " + op_str\
            + " -e " + str(global_options.threshold)
    output += "Options: "+options+"\n"
    if (not os.path.isdir(global_options.workdir+"/STATS")):
        os.mkdir(global_options.workdir+"/STATS")
    os.chdir(global_options.workdir+"/STATS")
    create_run_study (global_options.workdir,
                      global_options.frontend,
                      global_options.nodes_melissa,
                      global_options.openmp_threads,
                      global_options.server_path,
                      global_options.walltime_melissa,
                      global_options.mpi_options,
                      options,
                      global_options.batch_scheduler)
    if ("sobol" in global_options.operations) or ("sobol_indices" in global_options.operations):
        create_run_coupling (global_options.workdir,
                             global_options.nodes_saturne,
                             global_options.proc_per_node_saturne,
                             global_options.nb_parameters,
                             global_options.openmp_threads,
                             global_options.saturne_path,
                             global_options.walltime_container,
                             global_options.batch_scheduler)
    if (not os.path.isdir(global_options.workdir+"/case1/SCRIPTS")):
        os.mkdir(global_options.workdir+"/case1/SCRIPTS")
    os.chdir(global_options.workdir+"/case1/SCRIPTS")
    if ("sobol" in global_options.operations) or ("sobol_indices" in global_options.operations):
        create_runcase_sobol (global_options.workdir,
                              global_options.nodes_saturne,
                              global_options.proc_per_node_saturne,
                              global_options.nb_parameters,
                              global_options.openmp_threads,
                              global_options.saturne_path,
                              global_options.xml_file_name,
                              global_options.batch_scheduler)
    else:
        create_runcase (global_options.workdir,
                        global_options.nodes_saturne,
                        global_options.proc_per_node_saturne,
                        global_options.openmp_threads,
                        global_options.saturne_path,
                        global_options.walltime_saturne,
                        global_options.xml_file_name,
                        global_options.batch_scheduler)
    os.chdir(global_options.workdir+"/STATS")
    A = create_matrix(global_options.nb_parameters,
                      nb_groups,
                      global_options.range_min,
                      global_options.range_max)
    np.save("Amatrix",A)
    if ("sobol" in global_options.operations) or ("sobol_indices" in global_options.operations):
        B = create_matrix(global_options.nb_parameters,
                          nb_groups,
                          global_options.range_min,
                          global_options.range_max)
        np.save("Bmatrix",B)
        C = [create_matrix_k(A, B, i) for i in range(global_options.nb_parameters)]
    ret = np.zeros(global_options.nb_parameters + 2)
    for i in range(nb_groups):
        os.chdir(global_options.workdir)
        if (not os.path.isdir(global_options.workdir+"/group"+str(i))):
            create_case_str = global_options.saturne_path+"/code_saturne create --noref -s group"+str(i)+" -c rank0"
            if ("sobol" in global_options.operations) or ("sobol_indices" in global_options.operations):
                for j in range(global_options.nb_parameters+1):
                    create_case_str += " -c rank"+str(j+1)
    #        create_case_str
            os.system(create_case_str)
        ret[0] = create_case(A[i,:], 0, i, global_options.workdir, global_options.xml_file_name)
        if ("sobol" in global_options.operations) or ("sobol_indices" in global_options.operations):
            ret[1] = create_case(B[i,:], 1, i, global_options.workdir, global_options.xml_file_name)
            for j in range(global_options.nb_parameters):
                ret[j+2] = create_case(C[j][i,:], j+2, i, global_options.workdir, global_options.xml_file_name)
        for k in range(len(ret)):
            if (ret[k] != 0):
                output += "error creating simulation "+str(k)+" of group "+str(i)+"\n"
    os.chdir(global_options.workdir+"/STATS")
    if (global_options.batch_scheduler == "Slurm"):
        melissa_job_id = call_bash('sbatch "./run_study.sh"')['out'].split()[-1]
    elif (global_options.batch_scheduler == "CCC"):
        melissa_job_id = call_bash('ccc_msub "./run_study.sh"')['out'].split()[-1]
    elif (global_options.batch_scheduler == "OAR"):
        melissa_job_id = call_bash('oarsub -S "./run_study.sh" --project=avido')['out'].split("OAR_JOB_ID=")[1]
    elif (global_options.batch_scheduler == "local"):
        melissa_job_id = call_bash('./run_study.sh & echo $!')['out']
    melissa_first_job_id = melissa_job_id
    print melissa_job_id

    if (global_options.batch_scheduler == "Slurm") or (global_options.batch_scheduler == "CCC"):
        while (not "RUNNING" in call_bash("squeue --job="+melissa_job_id+" -l")['out']):
            time.sleep(20)
    if (global_options.batch_scheduler == "OAR"):
        while (not "Melissa" in call_bash("oarstat -u --sql \"state = 'Running'\"")['out']):
            time.sleep(20)
    time.sleep(5) # to give time to retrieve the name of the main server node

    server_state = "running"
    poller.register(pull_melissa_socket, zmq.POLLIN)

    thread1 = state_checker()
    thread2 = message_reciever()

    thread1.start()
    thread2.start()

    for i in range(nb_groups):
        for j in range(global_options.nb_parameters+2):
            casedir = global_options.workdir+"/group"+str(i)+"/rank"+str(j)
            os.system("cp "+global_options.workdir+"/case1/DATA/server_name.txt "+casedir+"/DATA")
        # scripts to launch coupled simulation groups
        os.chdir(global_options.workdir+"/group"+str(i))
        if ("sobol" in global_options.operations) or ("sobol_indices" in global_options.operations):
            create_coupling_parameters (global_options.nb_parameters,
                                        "None",
                                        global_options.nodes_saturne*global_options.proc_per_node_saturne,
                                        "None")
            if (global_options.batch_scheduler == "Slurm"):
                simu_job_id.append(call_bash('sbatch "../STATS/run_cas_couple.sh" --exclusive --job-name=Saturnes'+str(i))['out'].split()[-1])
            elif (global_options.batch_scheduler == "CCC"):
                simu_job_id.append(call_bash('ccc_msub "../STATS/run_cas_couple.sh"')['out'].split()[-1])
            elif (global_options.batch_scheduler == "OAR"):
                simu_job_id.append(call_bash('oarsub -S "../STATS/run_cas_couple.sh" -n Saturnes'+str(i)+' --project=avido')['out'].split("OAR_JOB_ID=")[1])
            elif (global_options.batch_scheduler == "local"):
                simu_job_id.append = call_bash('../STATS/run_cas_couple.sh & echo $!')['out']
        else:
            os.chdir("./rank0/SCRIPTS")
            if (global_options.batch_scheduler == "Slurm"):
                simu_job_id.append(call_bash('sbatch "./runcase" --exclusive --job-name=Saturne'+str(i))['out'].split()[-1])
            elif (global_options.batch_scheduler == "CCC"):
                simu_job_id.append(call_bash('ccc_msub "./runcase"')['out'].split()[-1])
            elif (global_options.batch_scheduler == "OAR"):
                simu_job_id.append(call_bash('oarsub -S "./runcase" -n Saturne'+str(i)+' --project=avido')['out'].split("OAR_JOB_ID=")[1])
            elif (global_options.batch_scheduler == "local"):
                simu_job_id.append = call_bash('./runcase & echo $!')['out']
        with lock_job_state:
            job_states[i] = 1 # pending
        if (server_state != "running"):
            with lock_job_state:
                for i in range(len(simu_job_id)):
                    if (simu_states[i] < 3): # submited and not terminated
                        if (global_options.batch_scheduler == "Slurm" or global_options.batch_scheduler == "CCC"):
                            call_bash("scancel "+simu_job_id[i])
                        elif (global_options.batch_scheduler == "OAR"):
                            call_bash("oardel "+simu_job_id[i])
                        elif (global_options.batch_scheduler == "OAR"):
                            call_bash("kill "+simu_job_id[i])

                melissa_job_id = reboot_server(global_options.workdir, melissa_first_job_id, melissa_job_id, global_options.batch_scheduler)
                if (global_options.batch_scheduler == "Slurm") or (global_options.batch_scheduler == "CCC"):
                    while (not "RUNNING" in call_bash("squeue --name=Melissa -l")['out']):
                        time.sleep(30)
                if (global_options.batch_scheduler == "OAR"):
                    while (not "Melissa" in call_bash("oarstat -u --sql \"state = 'Running'\"")['out']):
                        time.sleep(30)
        if (global_options.batch_scheduler == "Slurm") or (global_options.batch_scheduler == "CCC"):
            while (int(call_bash("squeue -u "+global_options.username+" | wc -l")['out']) >= 250):
                time.sleep(30)


    while not((simu_states.all == 2) and (server_state == "terminated")):
        time.sleep(30)

        with lock_server_state:
            if (server_state != "running"):
                with lock_job_state:
                    for i in range(len(simu_job_id)):
                        if (simu_states[i] < 3): # not terminated
                            if (global_options.batch_scheduler == "Slurm" or global_options.batch_scheduler == "CCC"):
                                call_bash("scancel "+simu_job_id[i])
                            elif (global_options.batch_scheduler == "OAR"):
                                call_bash("oardel "+simu_job_id[i])
                            elif (global_options.batch_scheduler == "OAR"):
                                call_bash("kill "+simu_job_id[i])

                    melissa_job_id = reboot_server(global_options.workdir, melissa_first_job_id, melissa_job_id, global_options.batch_scheduler)
                    if (global_options.batch_scheduler == "Slurm") or (global_options.batch_scheduler == "CCC"):
                        while (not "RUNNING" in call_bash("squeue --name=Melissa -l")['out']):
                            time.sleep(30)
                    if (global_options.batch_scheduler == "OAR"):
                        while (not "Melissa" in call_bash("oarstat -u --sql \"state = 'Running'\"")['out']):
                            time.sleep(30)
                    for i in range(len(simu_job_id)):
                        if (simu_states[i] < 3): # not terminated
                            os.chdir(global_options.workdir+"/group"+str(i))
                            if ("sobol" in global_options.operations) or ("sobol_indices" in global_options.operations):
                                if (global_options.batch_scheduler == "Slurm"):
                                    simu_job_id[i] = call_bash('sbatch "../STATS/run_cas_couple.sh" --exclusive --job-name=Saturnes'+str(i))['out'].split()[-1]
                                elif (global_options.batch_scheduler == "CCC"):
                                    simu_job_id[i] = call_bash('ccc_msub "../STATS/run_cas_couple.sh"')['out'].split()[-1]
                                elif (global_options.batch_scheduler == "OAR"):
                                    simu_job_id[i] = call_bash('oarsub -S "../STATS/run_cas_couple.sh" -n Saturnes'+str(i)+' --project=avido')['out'].split("OAR_JOB_ID=")[1]
                                elif (global_options.batch_scheduler == "local"):
                                    simu_job_id[i] = call_bash('../STATS/run_cas_couple.sh & echo $!')['out']
                            else:
                                os.chdir("./rank0/SCRIPTS")
                                if (global_options.batch_scheduler == "Slurm"):
                                    simu_job_id[i] = call_bash('sbatch "./runcase" --exclusive --job-name=Saturne'+str(i))['out'].split()[-1]
                                elif (global_options.batch_scheduler == "CCC"):
                                    simu_job_id[i] = call_bash('ccc_msub "./runcase"')['out'].split()[-1]
                                elif (global_options.batch_scheduler == "OAR"):
                                    simu_job_id[i] = call_bash('oarsub -S "./runcase" -n Saturne'+str(i)+' --project=avido')['out'].split("OAR_JOB_ID=")[1]
                                elif (global_options.batch_scheduler == "local"):
                                    simu_job_id[i] = call_bash('./runcase & echo $!')['out']

        for i in range(len(simu_job_id)):
            with lock_simu_state:
                with lock_job_state:
                    if (simu_states[i] != job_states[i]-1):
                        if (simu_states[i] == 1 and job_states[i] == 3):
                            reboot_simu(i, simu_job_id, output, global_options.batch_scheduler, global_options.workdir)
                        if (simu_states[i] == 0 and job_states[i] == 3):
                            reboot_simu(i, simu_job_id, output, global_options.batch_scheduler, global_options.workdir)
                        if (simu_states[i] == 0 and job_states[i] == 2):
                            out=check_timeout(i, simu_job_id, output, global_options.batch_scheduler)
                            if (out == True):
                                reboot_simu(i, simu_job_id, output, global_options.batch_scheduler, global_options.workdir)



    thread1.running_master = False

    thread2.running_master = False

    thread1.join()
    thread2.join()

    fichier=open("master.out", "w")
    fichier.write(output)
    fichier.close()



    #while True:
    ##    time.sleep(300)
    #    server_state = check_job(batch_scheduler, username, melissa_job_id)
    #    for i in range(len(simu_job_id)):
    #        if job_states[i] < 3 and job_states[i] > 0:
    #            state = check_job(batch_scheduler, username, simu_job_id[i])
    #            if ("running" == state):
    #                job_states[i] = 2 # running
    #            if ("terminated" == state):
    #                job_states[i] = 3 # terminated
    #while True:
    #    socks = dict(poller.poll(1000))
    #    if (pull_melissa_socket in socks.keys() and socks[pull_melissa_socket] == zmq.POLLIN):
    #        message = pull_melissa_socket.recv_string().split()
    #        if (message[0] == "timeout"):
    #            for simu in message[1:]:
    #                reboot_job(int(simu), simu_job_id, job_states)
    #        if (message[0] == "simu_state"):
    #            simu_id = int(message[1])
    #            simu_state = int(message[2])
    #            simu_states[simu_id] = simu_state
    #            if (simu_state != job_states[simu_id]):
    #                if (simu_state == 1 && job_states[simu_id] == 3):
    #                    reboot_simu(simu_id, simu_job_id))

    #    converged_sobol = np.zeros(nb_proc_server,int)
    #    iterations_server = np.zeros(nb_proc_server,int)
    #    finished_server = np.zeros(nb_proc_server,int)
    #    context = zmq.Context()
    #    pull_melissa_socket = context.socket(zmq.REP)
    #    pull_melissa_socket.bind("tcp://*:5555")
    #    poller = zmq.Poller()
    #    poller.register(pull_melissa_socket, zmq.POLLIN)
    #    snd_message = "continue"
    #    while True:
    #        socks = dict(poller.poll(1000))
    #        if (pull_melissa_socket in socks.keys() and socks[pull_melissa_socket] == zmq.POLLIN):
    #            message = dict([pull_melissa_socket.recv_string().split()])
    #            if (converged in message):
    #                pull_melissa_socket.send_string(snd_message)
    #                converged_sobol[int(message[converged])] = 1
    #            elif (finished in message):
    #                pull_melissa_socket.send_string(snd_message)
    #                finished_server[int(message[finished])] = 1
    #                if (not 0 in finished_server):
    #                    break
    #            elif (iteration in message):
    #                pull_melissa_socket.send_string(snd_message)
    #                iteration_server[int(message[iteration])] += 1
    #        if (not 0 in converged_sobol):
    #            print "Cancel pending simulation jobs..."
    #            os.system("oardel "+re.sub('\n',' ',call_bash("oarstat -u --sql \"state = 'Waiting'\" | grep 'Saturne' | grep -o '^[[:digit:]]\+'")))
    #            running_jobs = call_bash("oarstat -u --sql \"state = 'Running'\" | grep 'Saturne' | grep -o '^[[:digit:]]\+'").split("\n")
    #            if (range(running_jobs) == 0):
    #                snd_message = "continue"
    #            else:
    #                snd_message = "stop"

#==================================#
#               main               #
#==================================#

if __name__ == '__main__':
    launch_study()


