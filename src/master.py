import os
import time
import sys
import subprocess
import numpy as np
import numpy.random as rd
import re

#=====================================#
#               options               #
#=====================================#

batch_scheduler       = "Slurm"
nb_parameters         = 6
nb_groups             = 1000
nb_simu               = nb_groups*(nb_parameters+2)
nb_time_steps         = 100
operations            = ["mean","variance","min","max","threshold","sobol"]
threshold             = 0.7
mpi_OAR_options       = "--mca orte_rsh_agent \"oarsh\" --mca btl openib,sm,self --mca pml ^cm --machinefile $OAR_NODE_FILE"
mpi_Slurm_options     = ""
home_path             = "/scratch/G95757"
server_path           = home_path+"/Melissa/build/src"
workdir               = "/scratch/G95757/etude_eole"
saturne_path          = home_path+"/Code_Saturne/4.3/arch/eole/ompi/bin"
range_min             = np.zeros(nb_parameters,float)
range_max             = np.ones(nb_parameters,float)
range_min[0:2]        = 0.1
range_max[0:2]        = 0.9
range_min[2:4]        = 0.1
range_max[2:4]        = 0.9
range_min[4:6]        = 0.08
range_max[4:6]        = 0.8
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

#=======================================#
#               functions               #
#=======================================#

def call_bash(s):
    proc = subprocess.Popen(s, stdout=subprocess.PIPE, shell=True)
    (out, err) = proc.communicate()
    return out[:-1]

def create_matrix(nb_parameters, nb_groups, range_min, range_max):
    A = np.zeros((nb_groups, nb_parameters))
    for i in range(nb_parameters):
        A[:,i] = rd.uniform(range_min[i], range_max[i], nb_groups)
    return A

def create_matrix_k(A, B, k):
    Ck = np.copy(A)
    Ck[:,k] = B[:,k]
    return Ck

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
            contenu += 'param_duree_injection_haut='+str(Ai[5])+'\n'
        else:
            contenu += ligne
    fichier.close()
    fichier = open('cs_user_boundary_conditions.f90', 'w')
    fichier.write(contenu)
    fichier.close()
    os.system("cp "+workdir+"/case1/SRC/cs_user_mesh.c "+casedir+"/SRC/cs_user_mesh.c")
    return 0

def create_coupling_parameters (nb_parameters, n_procs_weight, n_procs_min, n_procs_max):
    contenu=""
    fichier=open("coupling_parameters.py", "w")
    contenu += "# -*- coding: utf-8 -*-                                                          \n"
    contenu += "                                                                                 \n"
    contenu += "#=============================================================================== \n"
    contenu += "# User variable settings to specify a coupling computation environnement.        \n"
    contenu += "                                                                                 \n"
    contenu += "# A coupling case is defined by a dictionnary, containing the following:         \n"
    contenu += "                                                                                 \n"
    contenu += "# Solver type ('Code_Saturne', 'SYRTHES', 'NEPTUNE_CFD' or 'Code_Aster')         \n"
    contenu += "# Domain directory name                                                          \n"
    contenu += "# Run parameter setting file                                                     \n"
    contenu += "# Number of processors (or None for automatic setting)                           \n"
    contenu += "# Optional command line parameters. If not useful = None                         \n"
    contenu += "#=============================================================================== \n"
    contenu += "                                                                                 \n"
    contenu += "# Define coupled domains                                                         \n"
    contenu += "                                                                                 \n"
    contenu += "domains = [                                                                      \n"
    contenu += "                                                                                 \n"
    contenu += "    {'solver': 'Code_Saturne',                                                   \n"
    contenu += "     'domain': 'rank"+str(0)+"',                                                 \n"
    contenu += "     'script': 'runcase',                                                        \n"
    contenu += "     'n_procs_weight': "+str(n_procs_weight)+",                                  \n"
    contenu += "     'n_procs_min': "+str(n_procs_min)+",                                        \n"
    contenu += "     'n_procs_max': "+str(n_procs_max)+"}                                        \n"
    contenu += "                                                                                 \n"
    for j in range(nb_parameters+1):
        contenu += "    ,                                                                            \n"
        contenu += "    {'solver': 'Code_Saturne',                                                   \n"
        contenu += "     'domain': 'rank"+str(j+1)+"',                                               \n"
        contenu += "     'script': 'runcase',                                                        \n"
        contenu += "     'n_procs_weight': "+str(n_procs_weight)+",                                  \n"
        contenu += "     'n_procs_min': "+str(n_procs_min)+",                                        \n"
        contenu += "     'n_procs_max': "+str(n_procs_max)+"}                                        \n"
        contenu += "                                                                                 \n"
    contenu += "    ]                                                                            \n"
    contenu += "                                                                                 \n"
    fichier.write(contenu)
    fichier.close()
    os.system("chmod 744 coupling_parameters.py")

def create_run_coupling (workdir, nodes_saturne, proc_per_node_saturne, nb_parameters, openmp_threads, saturne_path, walltime_container):
    contenu=""
    fichier=open("run_cas_couple.sh", "w")
    contenu += "#!/bin/bash                                                                     \n"
    if (batch_scheduler == "Slurm"):
        contenu += "#SBATCH -N "+str(nodes_saturne*(nb_parameters+2))+"                             \n"
        contenu += "#SBATCH --ntasks="+str(proc_per_node_saturne*nodes_saturne*(nb_parameters+2))+" \n"
        contenu += "#SBATCH --cpus-per-task="+str(openmp_threads)+"                                 \n"
        contenu += "#SBATCH --wckey=P11UK:AVIDO                                                     \n"
        contenu += "#SBATCH --partition=cn                                                          \n"
        contenu += "#SBATCH --time="+walltime_container+"                                           \n"
        contenu += "#SBATCH -o coupling.%j.log                                                      \n"
        contenu += "#SBATCH -e coupling.%j.err                                                      \n"
        contenu += "module load openmpi/2.0.1                                                       \n"
        contenu += "module load ifort/2017                                                          \n"
        contenu += "env | grep SLURM                                                                \n"
    elif (batch_scheduler == "OAR"):
        contenu += "#OAR -l nodes="+str(nodes_saturne*(nb_parameters+2))+",walltime="+walltime_container+ "\n"
        contenu += "#OAR -O coupling.%jobid%.log                                                           \n"
        contenu += "#OAR -E coupling.%jobid%.err                                                           \n"
        contenu += "module load openmpi/1.8.5_gcc-4.4.6                                                    \n"
        contenu += "ulimit -s unlimited                                                                    \n"
    contenu += "GROUP=$(basename `pwd` | cut -dp -f2)                                           \n"
    contenu += "cd "+workdir+"/group${GROUP}                                                    \n"
    contenu += "export OMP_NUM_THREADS="+str(openmp_threads)+"                                  \n"
    contenu += "export PATH="+saturne_path+"/:$PATH                                             \n"
    contenu += "date +\"%d/%m/%y %T\"                                                           \n"
    contenu += "\code_saturne run --coupling coupling_parameters.py                             \n"
    contenu += "date +\"%d/%m/%y %T\"                                                           \n"
    contenu += "exit $?                                                                         \n"
    fichier.write(contenu)
    fichier.close()
    os.system("chmod 744 run_cas_couple.sh")

def create_run_study (workdir, frontend, nodes_melissa, server_path, walltime_melissa, mpi_options, options):
    contenu = ""
    fichier=open("run_study.sh", "w")
    contenu += "#!/bin/bash                                                        \n"
    if (batch_scheduler == "Slurm"):
        contenu += "#SBATCH -N "+str(nodes_melissa)+"                                  \n"
        contenu += "#SBATCH --ntasks-per-node=2                                        \n"
        contenu += "#SBATCH --wckey=P11UK:AVIDO                                        \n"
        contenu += "#SBATCH --partition=cn                                             \n"
        contenu += "#SBATCH --time="+walltime_melissa+"                                \n"
        contenu += "#SBATCH -o melissa.%j.log                                          \n"
        contenu += "#SBATCH -e melissa.%j.err                                          \n"
        contenu += "#SBATCH --job-name=Melissa                                         \n"
        contenu += "module load openmpi/2.0.1                                          \n"
        contenu += "module load ifort/2017                                             \n"
    elif (batch_scheduler == "OAR"):
        contenu += "#OAR -l nodes="+str(nodes_melissa)+",walltime="+walltime_melissa+ "\n"
        contenu += "#OAR -O melissa.%jobid%.log                                        \n"
        contenu += "#OAR -E melissa.%jobid%.err                                        \n"
        contenu += "module load openmpi/1.8.5_gcc-4.4.6                                \n"
        contenu += "ulimit -s unlimited                                                \n"
        contenu += "export OMPI_MCA_orte_rsh_agent=oarsh                               \n"
    contenu += "date +\"%d/%m/%y %T\"                                              \n"
    contenu += "FRONTEND="+frontend+"                                              \n"
    contenu += "WORK_DIR="+workdir+"/STATS                                         \n"
    contenu += "STOP=0                                                             \n"
    contenu += "# generate server name files                                       \n"
    contenu += "cd "+workdir+"/case1/DATA                                          \n"
    contenu += server_path+"/../examples/create_file_server_name                   \n"
    contenu += "cd $WORK_DIR                                                       \n"
    contenu += "# launch simulation jobs                                           \n"
    contenu += "echo  \"### Launch saturne jobs\"                                  \n"
    if (batch_scheduler == "Slurm"):
        if ("sobol" in options) or ("sobol_indices" in options):
            contenu += "python "+workdir+"/master.py container                             \n"
        else:
            contenu += "python "+workdir+"/master.py simu                                  \n"
        contenu += "mkdir stats${SLURM_JOB_ID}.resu                                    \n"
        contenu += "cd stats${SLURM_JOB_ID}.resu                                       \n"
    elif (batch_scheduler == "OAR"):
        if ("sobol" in options) or ("sobol_indices" in options):
            contenu += "ssh $FRONTEND \"python "+workdir+"/master.py container\"           \n"
        else:
            contenu += "ssh $FRONTEND \"python "+workdir+"/master.py simu\"                \n"
        contenu += "mkdir stats${OAR_JOB_ID}.resu                                      \n"
        contenu += "cd stats${OAR_JOB_ID}.resu                                         \n"
    contenu += "                                                                   \n"
    contenu += "# run Melissa                                                      \n"
    contenu += "echo  \"### Launch Melissa\"                                       \n"
    contenu += "export OMP_NUM_THREADS="+str(openmp_threads)+"                     \n"
    contenu += "date +\"%d/%m/%y %T\"                                              \n"
    contenu += "mpirun "+mpi_options+" "+server_path+"/server "+options+" &        \n"
    contenu += "                                                                   \n"
    contenu += "wait %1                                                            \n"
    contenu += "date +\"%d/%m/%y %T\"                                              \n"
    contenu += "cd "+workdir+"                                                     \n"
    fichier.write(contenu)
    fichier.close()
    os.system("chmod 744 run_study.sh")

def create_runcase_sobol (workdir, nodes_saturne, proc_per_node_saturne, nb_parameters, openmp_threads, saturne_path, xml_file_name):
    # script to launch simulations
    contenu=""
    fichier=open("run_saturne.sh", "w")
    contenu += "#!/bin/bash                                        \n"
    contenu += "date +\"%d/%m/%y %T\"                              \n"
    contenu += "export OMP_NUM_THREADS="+str(openmp_threads)+"     \n"
    contenu += "# Ensure the correct command is found:             \n"
    contenu += "export PATH="+saturne_path+"/:$PATH                \n"
    if (coupling != 1):
        contenu += "# wait master name                                 \n"
        contenu += "while [ ! -f \"../DATA/master_name.txt\" ]         \n"
        contenu += "  do                                               \n"
        contenu += "  sleep 1s                                         \n"
        contenu += "done                                               \n"
    contenu += "# Run command:                                     \n"
    contenu += "\code_saturne run --param "+xml_file_name+"        \n"
    contenu += "date +\"%d/%m/%y %T\"                              \n"
    contenu += "exit $?                                            \n"
    fichier.write(contenu)
    fichier.close()
    os.system("chmod 744 run_saturne.sh")
    # script to launch master simulation
    contenu=""
    fichier=open("run_saturne_master.sh", "w")
    contenu += "#!/bin/bash                                                \n"
    contenu += "date +\"%d/%m/%y %T\"                                      \n"
    contenu += "cd ..                                                      \n"
    if (coupling != 1):
        contenu += "# generate master name file                                \n"
        contenu += "cd ../rank0/DATA                                           \n"
        contenu += server_path+"/../examples/create_file_master_name           \n"
        contenu += "cd ..                                                      \n"
        for j in range(nb_parameters+1):
            contenu += "cp ./DATA/master_name.txt ../rank"+str(j+1)+"/DATA \n"
    contenu += "cd ./SCRIPTS                                               \n"
    contenu += "export OMP_NUM_THREADS="+str(openmp_threads)+"             \n"
    contenu += "# Ensure the correct command is found:                     \n"
    contenu += "export PATH="+saturne_path+"/:$PATH                        \n"
    contenu += "# Run command:                                             \n"
    contenu += "\code_saturne run --param "+xml_file_name+"                \n"
    contenu += "date +\"%d/%m/%y %T\"                                      \n"
    contenu += "exit $?                                                    \n"
    fichier.write(contenu)
    fichier.close()
    os.system("chmod 744 run_saturne_master.sh")

def create_runcase (workdir, nodes_saturne, proc_per_node_saturne, openmp_threads, saturne_path, walltime_saturne, xml_file_name):
    contenu=""
    fichier=open("run_saturne_master.sh", "w")
    contenu += "#!/bin/bash                                                        \n"
    if (batch_scheduler == "Slurm"):
        contenu += "#SBATCH -N "+str(nodes_saturne)+"                                  \n"
        contenu += "#SBATCH --ntasks="+str(proc_per_node_saturne*nodes_saturne)+"      \n"
        contenu += "#SBATCH --cpus-per-task="+str(openmp_threads)+"                    \n"
        contenu += "#SBATCH --wckey=P11UK:AVIDO                                        \n"
        contenu += "#SBATCH --partition=cn                                             \n"
        contenu += "#SBATCH --time="+walltime_saturne+"                                \n"
        contenu += "#SBATCH -o saturne.%j.log                                          \n"
        contenu += "#SBATCH -e saturne.%j.err                                          \n"
        contenu += "module load openmpi/2.0.1                                          \n"
        contenu += "module load ifort/2017                                             \n"
    elif (batch_scheduler == "OAR"):
        contenu += "#OAR -l nodes="+str(nodes_saturne)+",walltime="+walltime_saturne+ "\n"
        contenu += "#OAR -O saturne.%jobid%.log                                        \n"
        contenu += "#OAR -E saturne.%jobid%.err                                        \n"
        contenu += "module load openmpi/1.8.5_gcc-4.4.6                                \n"
        contenu += "ulimit -s unlimited                                                \n"
    contenu += "export OMP_NUM_THREADS="+str(openmp_threads)+"                     \n"
    contenu += "export PATH="+saturne_path+"/:$PATH                                \n"
    contenu += "date +\"%d/%m/%y %T\"                                              \n"
    contenu += "\code_saturne run --param "+xml_file_name+"                        \n"
    contenu += "date +\"%d/%m/%y %T\"                                              \n"
    contenu += "exit $?                                                            \n"
    fichier.write(contenu)
    fichier.close()
    os.system("chmod 744 run_saturne_master.sh")


#==================================#
#               main               #
#==================================#

if len(sys.argv) > 1:
    job_step = sys.argv[1]
else:
    job_step = "first_step"

if (job_step == "first_step"):
    op_str = ""
    if (batch_scheduler == "Slurm"):
        mpi_options = mpi_Slurm_options
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
        create_case_str = saturne_path+"/code_saturne create --noref -s group"+str(i)+" -c rank0"
        if ("sobol" in operations) or ("sobol_indices" in operations):
            for j in range(nb_parameters+1):
                create_case_str += " -c rank"+str(j+1)
        os.chdir(workdir)
        print create_case_str
        os.system(create_case_str)
        ret[0] = create_case(A[i,:], 0, i, workdir, xml_file_name)
        if ("sobol" in operations) or ("sobol_indices" in operations):
            ret[1] = create_case(B[i,:], 1, i, workdir, xml_file_name)
            for j in range(nb_parameters):
                ret[j+2] = create_case(C[j][i,:], j+2, i, workdir, xml_file_name)
        for k in range(len(ret)):
            if (ret[k] != 0):
                print "error creating simulation "+str(k)+" of group "+str(i)
    if (batch_scheduler == "Slurm"):
        os.system('sbatch "./run_study.sh"')
    elif (batch_scheduler == "OAR"):
        os.system('oarsub -S "./run_study.sh" --project=avido')

if ((job_step == "container") or (job_step == "simu")):
    if (not (("sobol" in operations) or ("sobol_indices" in operations))):
        nb_simu = nb_groups
    else:
        nb_simu = nb_groups*(nb_parameters+2)
    for i in range(nb_groups):
        # scripts to launch coupled simulation groups
        os.chdir(workdir+"/group"+str(i))
        if ("sobol" in operations) or ("sobol_indices" in operations):
            create_coupling_parameters (nb_parameters, "None", nodes_saturne*proc_per_node_saturne, "None")
            if (batch_scheduler == "Slurm"):
                os.system('sbatch "../STATS/run_cas_couple.sh" --job-name=Saturnes'+str(i))
            elif (batch_scheduler == "OAR"):
                os.system('oarsub -S "../STATS/run_cas_couple.sh" -n Saturnes'+str(i)+' --project=avido')
        else:
            os.chdir("./rank0/SCRIPTS")
            if (batch_scheduler == "Slurm"):
                os.system('sbatch "./runcase" --job-name=Saturne'+str(i))
            elif (batch_scheduler == "OAR"):
                os.system('oarsub -S "./runcase" -n Saturne'+str(i)+' --project=avido')

if ((job_step == "container") or (job_step == "simu")):
    converged_sobol = np.zeros(nb_proc_server,int)
    iterations_server = np.zeros(nb_proc_server,int)
    finished_server = np.zeros(nb_proc_server,int)
    context = zmq.Context()
    rep_melissa_socket = context.socket(zmq.REP)
    rep_melissa_socket.bind("tcp://*:5555")
#        pull_simu_socket = context.socket(zmq.PULL)
#        pull_simu_socket.bind("tcp://*:5556")
    poller = zmq.Poller()
    poller.register(rep_melissa_socket, zmq.POLLIN)
#        poller.register(pull_simu_socket, zmq.POLLIN)
    snd_message = "continue"
    while True:
        socks = dict(poller.poll(1000))
        if (rep_melissa_socket in socks.keys() and socks[rep_melissa_socket] == zmq.POLLIN):
#                rcv_message = rep_melissa_socket.recv_string()
            message = dict([rep_melissa_socket.recv_string().split()])
            if (converged in message):
                rep_melissa_socket.send_string(snd_message)
                converged_sobol[int(message[converged])] = 1
            elif (finished in message):
                rep_melissa_socket.send_string(snd_message)
                finished_server[int(message[finished])] = 1
                if (not 0 in finished_server):
                    break
            elif (iteration in message):
                rep_melissa_socket.send_string(snd_message)
                iteration_server[int(message[iteration])] += 1
#            if (pull_simu_socket in socks.keys() and socks[pull_simu_socket] == zmq.POLLIN):
#                rcv_message = pull_simu_socket.recv_string()
#                message = int(rcv_message)
#                if (message >= 0 and message < nb_groups):
        if (not 0 in converged_sobol):
            print "Cancel pending simulation jobs..."
            os.system("oardel "+re.sub('\n',' ',call_bash("oarstat -u --sql \"state = 'Waiting'\" | grep 'Saturne' | grep -o '^[[:digit:]]\+'")))
            running_jobs = call_bash("oarstat -u --sql \"state = 'Running'\" | grep 'Saturne' | grep -o '^[[:digit:]]\+'").split("\n")
            if (range(running_jobs) == 0):
                snd_message = "continue"
            else:
                snd_message = "stop"
        if (not Melissa in call_bash("oarstat -u --sql \"state = 'Running'\"")):
            print "Melissa crashed at iteration %d" % iterations_server[0]




