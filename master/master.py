import os
import time
import sys
import subprocess
import numpy as np
import numpy.random as rd
import re
import zmq

from threading import Thread, RLock

from fault_tolerance import *
from matrix_sobol import *
from call_bash import *
from batch_scripts import *

#=====================================#
#               options               #
#=====================================#

batch_scheduler       = "Slurm"
nb_parameters         = 6
nb_groups             = 1000
nb_time_steps         = 100
operations            = ["mean","variance","min","max","threshold","sobol"]
threshold             = 0.7
mpi_OAR_options       = "--mca orte_rsh_agent \"oarsh\" --mca btl openib,sm,self --mca pml ^cm --machinefile $OAR_NODE_FILE"
mpi_Slurm_options     = ""
mpi_CCC_options       = ""
home_path             = "/scratch/G95757"
server_path           = home_path+"/Melissa/build/server"
workdir               = "/scratch/G95757/etude_eole"
saturne_path          = home_path+"/Code_Saturne/4.3/arch/eole/ompi/bin"
range_min             = np.zeros(nb_parameters,float)
range_max             = np.ones(nb_parameters,float)
range_min[0:2]        = 0.1
range_max[0:2]        = 0.9
range_min[2:4]        = 0.1
range_max[2:4]        = 0.9
range_min[4:6]        = 0.88
range_max[4:6]        = 1.2
nodes_saturne         = 4
proc_per_node_saturne = 14
openmp_threads        = 2
nodes_melissa         = 3
walltime_saturne      = "1:20:0"
walltime_container    = "1:20:0"
walltime_melissa      = "240:00:0"
frontend              = "eofront2"
coupling              = 1
xml_file_name         = "bundle_3x2_f16_param.xml"
username              = "terrazth"


#=====================================#
#               Threads               #
#=====================================#

lock_jobs_state = RLock()
lock_simu_state = RLock()
lock_server_state = RLock()

class state_checker(Thread):
    def __init__(self):
        Thread.__init__(self)
    def run(self):
        while running_master == True:
            time.sleep(10)
            with lock_server_state:
                server_state = check_job(batch_scheduler, username, melissa_job_id)
            for i in range(len(simu_job_id)):
                if job_states[i] < 3 and job_states[i] > 0:
                    state = check_job(batch_scheduler, username, simu_job_id[i])
                    if ("running" == state):
                        with lock_job_state:
                            job_states[i] = 2 # running
                    if ("terminated" == state):
                        with lock_job_state:
                            job_states[i] = 3 # terminated

class message_reciever(Thread):
    def __init__(self):
        Thread.__init__(self)
    def run(self):
        while running_master == True:
            socks = dict(poller.poll(1000))
            if (rep_melissa_socket in socks.keys() and socks[rep_melissa_socket] == zmq.POLLIN):
                message = rep_melissa_socket.recv_string().split()
                if (message[0] == "timeout"):
                    for simu in message[1:]:
                        with lock_job_state:
                            reboot_simu(int(simu), simu_job_id)
                            job_states[int(simu)] = 1 # pennding or runnning
                if (message[0] == "simu_state"):
                    simu_id = int(message[1])
                    simu_state = int(message[2])
                    simu_states[simu_id] = simu_state

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

#==================================#
#               main               #
#==================================#


running_master = True
output = ""

if (not (("sobol" in operations) or ("sobol_indices" in operations))):
    nb_simu = nb_groups
else:
    nb_simu = nb_groups*(nb_parameters+2)

op_str = ""
if (batch_scheduler == "Slurm"):
    mpi_options = mpi_Slurm_options
elif (batch_scheduler == "CCC"):
    mpi_options = mpi_CCC_options
elif (batch_scheduler == "OAR"):
    mpi_options = mpi_OAR_options
for i in range(len(operations)):
    if (i < len(operations) - 1):
        op_str += operations[i] + ":"
    else:
        op_str += operations[i]
options = " -p " + str(nb_parameters)\
        + " -s " + str(nb_simu)\
        + " -g " + str(nb_groups)\
        + " -t " + str(nb_time_steps)\
        + " -o " + op_str\
        + " -e " + str(threshold)
if (not os.path.isdir(workdir+"/STATS")):
    os.mkdir(workdir+"/STATS")
os.chdir(workdir+"/STATS")
create_run_study (workdir, frontend, nodes_melissa, server_path, walltime_melissa, mpi_options, options)
if ("sobol" in operations) or ("sobol_indices" in operations):
    create_run_coupling (workdir, nodes_saturne, proc_per_node_saturne, nb_parameters, openmp_threads, saturne_path, walltime_container)
if (not os.path.isdir(workdir+"/case1/SCRIPTS")):
    os.mkdir(workdir+"/case1/SCRIPTS")
os.chdir(workdir+"/case1/SCRIPTS")
if ("sobol" in operations) or ("sobol_indices" in operations):
    create_runcase_sobol (workdir, nodes_saturne, proc_per_node_saturne, nb_parameters, openmp_threads, saturne_path, xml_file_name)
else:
    create_runcase (workdir, nodes_saturne, proc_per_node_saturne, openmp_threads, saturne_path, walltime_saturne, xml_file_name)
os.chdir(workdir+"/STATS")
A = create_matrix(nb_parameters, nb_groups, range_min, range_max)
np.save("Amatrix",A)
if ("sobol" in operations) or ("sobol_indices" in operations):
    B = create_matrix(nb_parameters, nb_groups, range_min, range_max)
    np.save("Bmatrix",B)
    C = [create_matrix_k(A, B, i) for i in range(nb_parameters)]
ret = np.zeros(nb_parameters + 2)
for i in range(nb_groups):
    os.chdir(workdir)
    if (not os.path.isdir(workdir+"/group"+str(i))):
        create_case_str = saturne_path+"/code_saturne create --noref -s group"+str(i)+" -c rank0"
        if ("sobol" in operations) or ("sobol_indices" in operations):
            for j in range(nb_parameters+1):
                create_case_str += " -c rank"+str(j+1)
#        create_case_str
        os.system(create_case_str)
    ret[0] = create_case(A[i,:], 0, i, workdir, xml_file_name)
    if ("sobol" in operations) or ("sobol_indices" in operations):
        ret[1] = create_case(B[i,:], 1, i, workdir, xml_file_name)
        for j in range(nb_parameters):
            ret[j+2] = create_case(C[j][i,:], j+2, i, workdir, xml_file_name)
    for k in range(len(ret)):
        if (ret[k] != 0):
            output += "error creating simulation "+str(k)+" of group "+str(i)+"\n"
os.chdir(workdir+"/STATS")
if (batch_scheduler == "Slurm"):
    melissa_job_id = call_bash('sbatch "./run_study.sh"').split()[-1]
elif (batch_scheduler == "CCC"):
    melissa_job_id = call_bash('ccc_msub "./run_study.sh"').split()[-1]
elif (batch_scheduler == "OAR"):
    melissa_job_id = call_bash('oarsub -S "./run_study.sh" --project=avido').split("OAR_JOB_ID=")[1]
melissa_first_job_id = melissa_job_id

if (batch_scheduler == "Slurm") or (batch_scheduler == "CCC"):
    while (not "RUNNING" in call_bash("squeue --job="+melissa_job_id+" -l")):
        time.sleep(20)
if (batch_scheduler == "OAR"):
    while (not "Melissa" in call_bash("oarstat -u --sql \"state = 'Running'\"")):
        time.sleep(20)
time.sleep(5) # to give time to retrieve the name of the main server node

simu_job_id = list()
job_states = np.zeros(nb_groups) # not submitted
simu_states = np.zeros(nb_groups) # simu as seen by the Server

context = zmq.Context()
rep_melissa_socket = context.socket(zmq.REP)
rep_melissa_socket.bind("tcp://*:5555")

thread1 = state_checker()
thread2 = message_reciever()

thread1.start()
thread2.start()

for i in range(nb_groups):
    for j in range(nb_parameters+2):
        casedir = workdir+"/group"+str(i)+"/rank"+str(j)
        os.system("cp "+workdir+"/case1/DATA/server_name.txt "+casedir+"/DATA")
    # scripts to launch coupled simulation groups
    os.chdir(workdir+"/group"+str(i))
    if ("sobol" in operations) or ("sobol_indices" in operations):
        create_coupling_parameters (nb_parameters, "None", nodes_saturne*proc_per_node_saturne, "None")
        if (batch_scheduler == "Slurm"):
            simu_job_id.append(call_bash('sbatch "../STATS/run_cas_couple.sh" --exclusive --job-name=Saturnes'+str(i))['out'].split()[-1])
        elif (batch_scheduler == "CCC"):
            simu_job_id.append(call_bash('ccc_msub "../STATS/run_cas_couple.sh"')['out'].split()[-1])
        elif (batch_scheduler == "OAR"):
            simu_job_id.append(call_bash('oarsub -S "../STATS/run_cas_couple.sh" -n Saturnes'+str(i)+' --project=avido')['out'].split("OAR_JOB_ID=")[1])
    else:
        os.chdir("./rank0/SCRIPTS")
        if (batch_scheduler == "Slurm"):
            simu_job_id.append(call_bash('sbatch "./runcase" --exclusive --job-name=Saturne'+str(i))['out'].split()[-1])
        elif (batch_scheduler == "CCC"):
            simu_job_id.append(call_bash('ccc_msub "./runcase"')['out'].split()[-1])
        elif (batch_scheduler == "OAR"):
            simu_job_id.append(call_bash('oarsub -S "./runcase" -n Saturne'+str(i)+' --project=avido')['out'].split("OAR_JOB_ID=")[1])
    with lock_state:
        job_states[i] = 1 # pending
    if (server_state != running)
        with lock_jobs_state
            for i in range(simu_job_id):
                if (simu_states[i] < 3) # submited and not terminated
                    if (batch_scheduler == "Slurm" or batch_scheduler == "CCC"):
                        call_bash("scancel "+simu_job_id[])
                    elif (batch_scheduler == "OAR"):
                        call_bash("oardel "+simu_job_id)

            melissa_job_id = reboot_server(workdir, melissa_first_job_id, melissa_job_id)
            if (batch_scheduler == "Slurm") or (batch_scheduler == "CCC"):
                while (not "RUNNING" in call_bash("squeue --name=Melissa -l")['out']):
                    time.sleep(30)
            if (batch_scheduler == "OAR"):
                while (not "Melissa" in call_bash("oarstat -u --sql \"state = 'Running'\"")['out']):
                    time.sleep(30)
     if (batch_scheduler == "Slurm") or (batch_scheduler == "CCC"):
        while (int(call_bash("squeue -u "+username+" | wc -l")['out']) >= 250):
            time.sleep(30)


while !((simu_states.all == 2) and (server_state == "terminated")):
    time.sleep(30)

    with lock_server_state:
        if (server_state != running)
            with lock_jobs_state
                for i in range(simu_job_id):
                    if (simu_states[i] < 3) # not terminated
                        if (batch_scheduler == "Slurm" or batch_scheduler == "CCC"):
                            call_bash("scancel "+simu_job_id[])
                        elif (batch_scheduler == "OAR"):
                            call_bash("oardel "+simu_job_id)

                melissa_job_id = reboot_server(workdir, melissa_first_job_id, melissa_job_id)
                if (batch_scheduler == "Slurm") or (batch_scheduler == "CCC"):
                    while (not "RUNNING" in call_bash("squeue --name=Melissa -l")['out']):
                        time.sleep(30)
                if (batch_scheduler == "OAR"):
                    while (not "Melissa" in call_bash("oarstat -u --sql \"state = 'Running'\"")['out']):
                        time.sleep(30)
                for i in range(simu_job_id):
                    if (simu_states[i] < 3) # not terminated
                    os.chdir(workdir+"/group"+str(i))
                    if ("sobol" in operations) or ("sobol_indices" in operations):
                        create_coupling_parameters (nb_parameters, "None", nodes_saturne*proc_per_node_saturne, "None")
                        if (batch_scheduler == "Slurm"):
                            simu_job_id.append(call_bash('sbatch "../STATS/run_cas_couple.sh" --exclusive --job-name=Saturnes'+str(i))['out'].split()[-1])
                        elif (batch_scheduler == "CCC"):
                            simu_job_id.append(call_bash('ccc_msub "../STATS/run_cas_couple.sh"')['out'].split()[-1])
                        elif (batch_scheduler == "OAR"):
                            simu_job_id.append(call_bash('oarsub -S "../STATS/run_cas_couple.sh" -n Saturnes'+str(i)+' --project=avido')['out'].split("OAR_JOB_ID=")[1])
                    else:
                        os.chdir("./rank0/SCRIPTS")
                        if (batch_scheduler == "Slurm"):
                            simu_job_id.append(call_bash('sbatch "./runcase" --exclusive --job-name=Saturne'+str(i))['out'].split()[-1])
                        elif (batch_scheduler == "CCC"):
                            simu_job_id.append(call_bash('ccc_msub "./runcase"')['out'].split()[-1])
                        elif (batch_scheduler == "OAR"):
                            simu_job_id.append(call_bash('oarsub -S "./runcase" -n Saturne'+str(i)+' --project=avido')['out'].split("OAR_JOB_ID=")[1])
                        if (batch_scheduler == "Slurm" or batch_scheduler == "CCC"):
                            call_bash("scancel "+simu_job_id[])
                        elif (batch_scheduler == "OAR"):
                            call_bash("oardel "+simu_job_id)

    for i in range(simu_job_id):
        with lock_simu_state
            with lock_job_state
                if (simu_states[i] != job_states[i]-1):
                    if (simu_states[i] == 1 && job_states[i] == 3):
                        reboot_simu(i, simu_job_id))
                    if (simu_states[simu_id] == 0 && job_states[simu_id] == 3):
                        reboot_simu(simu_id, simu_job_id))
                    if (simu_states[simu_id] == 0 && job_states[simu_id] == 2):
                        out=check_timeout(simu_id, simu_job_id)
                        if (out == True):
                            reboot_simu(simu_id, simu_job_id))



running_master = False

thread1.join()
thread2.join()

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
#    if (rep_melissa_socket in socks.keys() and socks[rep_melissa_socket] == zmq.POLLIN):
#        message = rep_melissa_socket.recv_string().split()
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
#    rep_melissa_socket = context.socket(zmq.REP)
#    rep_melissa_socket.bind("tcp://*:5555")
#    poller = zmq.Poller()
#    poller.register(rep_melissa_socket, zmq.POLLIN)
#    snd_message = "continue"
#    while True:
#        socks = dict(poller.poll(1000))
#        if (rep_melissa_socket in socks.keys() and socks[rep_melissa_socket] == zmq.POLLIN):
#            message = dict([rep_melissa_socket.recv_string().split()])
#            if (converged in message):
#                rep_melissa_socket.send_string(snd_message)
#                converged_sobol[int(message[converged])] = 1
#            elif (finished in message):
#                rep_melissa_socket.send_string(snd_message)
#                finished_server[int(message[finished])] = 1
#                if (not 0 in finished_server):
#                    break
#            elif (iteration in message):
#                rep_melissa_socket.send_string(snd_message)
#                iteration_server[int(message[iteration])] += 1
#        if (not 0 in converged_sobol):
#            print "Cancel pending simulation jobs..."
#            os.system("oardel "+re.sub('\n',' ',call_bash("oarstat -u --sql \"state = 'Waiting'\" | grep 'Saturne' | grep -o '^[[:digit:]]\+'")))
#            running_jobs = call_bash("oarstat -u --sql \"state = 'Running'\" | grep 'Saturne' | grep -o '^[[:digit:]]\+'").split("\n")
#            if (range(running_jobs) == 0):
#                snd_message = "continue"
#            else:
#                snd_message = "stop"




