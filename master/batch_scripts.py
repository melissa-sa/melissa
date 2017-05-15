import os
import time
import sys
import re


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

def create_run_coupling (workdir, nodes_saturne, proc_per_node_saturne, nb_parameters, openmp_threads, saturne_path, walltime_container, batch_scheduler):
    contenu=""
    fichier=open("run_cas_couple.sh", "w")
    contenu += "#!/bin/bash                                                                     \n"
    if (batch_scheduler == "Slurm"):
        contenu += "#SBATCH -N "+str(nodes_saturne*(nb_parameters+2))+"                                    \n"
        contenu += "#SBATCH --ntasks="+str(proc_per_node_saturne*nodes_saturne*(nb_parameters+2))+"        \n"
        contenu += "#SBATCH --cpus-per-task="+str(openmp_threads)+"                                        \n"
        contenu += "#SBATCH --wckey=P11UK:AVIDO                                                            \n"
        contenu += "#SBATCH --partition=cn                                                                 \n"
        contenu += "#SBATCH --mem=0                                                                        \n"
        contenu += "#SBATCH --time="+walltime_container+"                                                  \n"
        contenu += "#SBATCH -o coupling.%j.log                                                             \n"
        contenu += "#SBATCH -e coupling.%j.err                                                             \n"
        contenu += "module load openmpi/2.0.1                                                              \n"
        contenu += "module load icc/2017                                                                   \n"
        contenu += "module load ifort/2017                                                                 \n"
        contenu += "env | grep SLURM                                                                       \n"
    elif (batch_scheduler == "OAR"):
        contenu += "#OAR -l nodes="+str(nodes_saturne*(nb_parameters+2))+",walltime="+walltime_container+ "\n"
        contenu += "#OAR -O coupling.%jobid%.log                                                           \n"
        contenu += "#OAR -E coupling.%jobid%.err                                                           \n"
        contenu += "module load openmpi/1.8.5_gcc-4.4.6                                                    \n"
        contenu += "ulimit -s unlimited                                                                    \n"
    elif (batch_scheduler == "CCC"):
        contenu += "#MSUB -n "+str(proc_per_node_saturne*nodes_saturne*(nb_parameters+2))+"                \n"
        contenu += "#MSUB -c "+str(openmp_threads)+"                                                       \n"
        contenu += "#MSUB -o coupling.%I.log                                                               \n"
        contenu += "#MSUB -e coupling.%I.err                                                               \n"
        contenu += "#MSUB -T "+walltime_container+"                                                        \n"
        contenu += "#MSUB -A gen10064                                                                      \n"
        contenu += "#MSUB -q standard                                                                      \n"
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

def create_run_study (workdir, nodes_melissa, openmp_threads, server_path, walltime_melissa, mpi_options, options, batch_scheduler):
    # signal handler definition
    signal_handler="handler() {                            \n"
    signal_handler+="echo \"### CLEAN-UP TIME !!!\"        \n"
    signal_handler+="STOP=1                                \n"
    signal_handler+="sleep 1                               \n"
    #signal_handler+="killer \n"
    signal_handler+="killall -USR1 mpirun                  \n"
    signal_handler+="wait %1                               \n"
    signal_handler+="}                                     \n"
    contenu = ""
    fichier=open("run_study.sh", "w")
    contenu += "#!/bin/bash                                                        \n"
    if (batch_scheduler == "Slurm"):
        contenu += "#SBATCH -N "+str(nodes_melissa)+"                                  \n"
        contenu += "#SBATCH --ntasks-per-node=14                                        \n"
        contenu += "#SBATCH --wckey=P11UK:AVIDO                                        \n"
        contenu += "#SBATCH --partition=bm                                             \n"
        contenu += "#SBATCH --mem=0                                                    \n"
        contenu += "#SBATCH --time="+walltime_melissa+"                                \n"
        contenu += "#SBATCH -o melissa.%j.log                                          \n"
        contenu += "#SBATCH -e melissa.%j.err                                          \n"
        contenu += "#SBATCH --job-name=Melissa                                         \n"
        contenu += "#SBATCH --signal=B:SIGUSR2@300                                       \n"
        contenu += "module load openmpi/2.0.1                                          \n"
        contenu += "module load ifort/2017                                             \n"
    elif (batch_scheduler == "OAR"):
        contenu += "#OAR -l nodes="+str(nodes_melissa)+",walltime="+walltime_melissa+ "\n"
        contenu += "#OAR -O melissa.%jobid%.log                                        \n"
        contenu += "#OAR -E melissa.%jobid%.err                                        \n"
        contenu += "#OAR -n Melissa                                                    \n"
        contenu += "#OAR --checkpoint 300                                              \n"
        contenu += "#OAR --signal=SIGUSR2                                              \n"
        contenu += "module load openmpi/1.8.5_gcc-4.4.6                                \n"
        contenu += "ulimit -s unlimited                                                \n"
        contenu += "export OMPI_MCA_orte_rsh_agent=oarsh                               \n"
    elif (batch_scheduler == "CCC"):
        contenu += "#MSUB -n "+str(nodes_melissa*(16/openmp_threads))+"                 \n"
        contenu += "#MSUB -c "+str(openmp_threads)+"                                   \n"
        contenu += "#MSUB -o melissa.%I.log                                            \n"
        contenu += "#MSUB -e melissa.%I.err                                            \n"
        contenu += "#MSUB -T "+walltime_melissa+"                                      \n"
        contenu += "#MSUB -A gen10064                                                  \n"
        contenu += "#MSUB -r Melissa                                                   \n"
        contenu += "#MSUB -q standard                                                  \n"
        contenu += "#MSUB --signal=B:SIGUSR2@300                                       \n"
#    contenu += signal handler
    contenu += "date +\"%d/%m/%y %T\"                                              \n"
    contenu += "WORK_DIR="+workdir+"/STATS                                         \n"
    contenu += "STOP=0                                                             \n"
    contenu += "# generate server name files                                       \n"
    contenu += "cd "+workdir+"/case1/DATA                                          \n"
    contenu += server_path+"/../utils/create_file_server_name                   \n"
    contenu += "# run Melissa                                                      \n"
    contenu += "echo  \"### Launch Melissa\"                                       \n"
    contenu += "cd $WORK_DIR                                                       \n"
    if (batch_scheduler == "Slurm") or (batch_scheduler == "CCC"):
        contenu += "mkdir stats${SLURM_JOB_ID}.resu                                    \n"
        contenu += "cd stats${SLURM_JOB_ID}.resu                                       \n"
    elif (batch_scheduler == "OAR"):
        contenu += "mkdir stats${OAR_JOB_ID}.resu                                      \n"
        contenu += "cd stats${OAR_JOB_ID}.resu                                         \n"
    contenu += "export OMP_NUM_THREADS="+str(openmp_threads)+"                     \n"
    contenu += "trap handler USR2                                                  \n"
    contenu += "mpirun "+mpi_options+" "+server_path+"/server "+options+" &        \n"
    contenu += "wait %1                                                            \n"
    contenu += "date +\"%d/%m/%y %T\"                                              \n"
    contenu += "cd "+workdir+"                                                     \n"
    fichier.write(contenu)
    fichier.close()
    os.system("chmod 744 run_study.sh")

def create_reboot_study (workdir,
                         nodes_melissa,
                         openmp_threads,
                         server_path,
                         walltime_melissa,
                         mpi_options,
                         options,
                         batch_scheduler,
                         melissa_first_job_id):
    # signal handler definition
    signal_handler="handler() {                      \n"
    signal_handler+="echo \"### CLEAN-UP TIME !!!\"  \n"
    signal_handler+="STOP=1                          \n"
    signal_handler+="sleep 1                         \n"
    #signal_handler+="killer \n"
    signal_handler+="killall -USR1 mpirun            \n"
    signal_handler+="wait %1                         \n"
    signal_handler+="}                               \n"
    contenu = ""
    fichier=open("reboot_study.sh", "w")
    contenu += "#!/bin/bash                                                        \n"
    if (batch_scheduler == "Slurm"):
        contenu += "#SBATCH -N "+str(nodes_melissa)+"                                  \n"
        contenu += "#SBATCH --ntasks-per-node=14                                        \n"
        contenu += "#SBATCH --wckey=P11UK:AVIDO                                        \n"
        contenu += "#SBATCH --partition=bm                                             \n"
        contenu += "#SBATCH --mem=0                                                    \n"
        contenu += "#SBATCH --time="+walltime_melissa+"                                \n"
        contenu += "#SBATCH -o melissa.%j.log                                          \n"
        contenu += "#SBATCH -e melissa.%j.err                                          \n"
        contenu += "#SBATCH --job-name=Melissa                                         \n"
        contenu += "#SBATCH --signal=B:SIGUSR2@30                                        \n"
        contenu += "module load openmpi/2.0.1                                          \n"
        contenu += "module load ifort/2017                                             \n"
    elif (batch_scheduler == "OAR"):
        contenu += "#OAR -l nodes="+str(nodes_melissa)+",walltime="+walltime_melissa+ "\n"
        contenu += "#OAR -O melissa.%jobid%.log                                        \n"
        contenu += "#OAR -E melissa.%jobid%.err                                        \n"
        contenu += "#OAR -n Melissa                                                    \n"
        contenu += "#OAR --checkpoint 300                                              \n"
        contenu += "#OAR --signal=SIGUSR2                                              \n"
        contenu += "module load openmpi/1.8.5_gcc-4.4.6                                \n"
        contenu += "ulimit -s unlimited                                                \n"
        contenu += "export OMPI_MCA_orte_rsh_agent=oarsh                               \n"
    elif (batch_scheduler == "CCC"):
        contenu += "#MSUB -n "+str(nodes_melissa*(16/openmp_threads))+"                 \n"
        contenu += "#MSUB -c "+str(openmp_threads)+"                                   \n"
        contenu += "#MSUB -o melissa.%I.log                                            \n"
        contenu += "#MSUB -e melissa.%I.err                                            \n"
        contenu += "#MSUB -T "+walltime_melissa+"                                      \n"
        contenu += "#MSUB -A gen10064                                                  \n"
        contenu += "#MSUB -r Melissa                                                   \n"
        contenu += "#MSUB -q standard                                                  \n"
        contenu += "#MSUB --signal=B:SIGUSR2@30                                        \n"
    contenu += signal_handler
    contenu += "date +\"%d/%m/%y %T\"                                              \n"
    contenu += "WORK_DIR="+workdir+"/STATS                                         \n"
    contenu += "STOP=0                                                             \n"
    contenu += "# generate server name files                                       \n"
    contenu += "cd "+workdir+"/case1/DATA                                          \n"
    contenu += server_path+"/../utils/create_file_server_name                   \n"
    contenu += "# run Melissa                                                      \n"
    contenu += "echo  \"### Launch Melissa\"                                       \n"
    contenu += "cd $WORK_DIR                                                       \n"
    contenu += "cd stats"+melissa_first_job_id+".resu                              \n"
    contenu += "export OMP_NUM_THREADS="+str(openmp_threads)+"                     \n"
    contenu += "trap handler USR2                                                  \n"
    contenu += "date +\"%d/%m/%y %T\"                                              \n"
    contenu += "mpirun "+mpi_options+" "+server_path+"/server "+options+" -r . &   \n"
    contenu += "wait %1                                                            \n"
    contenu += "date +\"%d/%m/%y %T\"                                              \n"
    contenu += "cd "+workdir+"                                                     \n"
    fichier.write(contenu)
    fichier.close()
    os.system("chmod 744 reboot_study.sh")

def create_runcase_sobol (workdir, nodes_saturne, proc_per_node_saturne, nb_parameters, openmp_threads, saturne_path, xml_file_name, batch_scheduler, coupling = 1):
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
        contenu += server_path+"/../utils/create_file_master_name           \n"
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

def create_runcase (workdir, nodes_saturne, proc_per_node_saturne, openmp_threads, saturne_path, walltime_saturne, xml_file_name, batch_scheduler):
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
    elif (batch_scheduler == "CCC"):
        contenu += "#MSUB -n "+str(proc_per_node_saturne*nodes_saturne)+"                \n"
        contenu += "#MSUB -c "+str(openmp_threads)+"                                                      \n"
        contenu += "#MSUB -o saturne.%I.log                                                               \n"
        contenu += "#MSUB -e saturne.%I.err                                                               \n"
        contenu += "#MSUB -T "+walltime_container+"                                                       \n"
        contenu += "#MSUB -A gen10064                                                                     \n"
        contenu += "#MSUB -q standard                                                                     \n"
    contenu += "export OMP_NUM_THREADS="+str(openmp_threads)+"                     \n"
    contenu += "export PATH="+saturne_path+"/:$PATH                                \n"
    contenu += "date +\"%d/%m/%y %T\"                                              \n"
    contenu += "\code_saturne run --param "+xml_file_name+"                        \n"
    contenu += "date +\"%d/%m/%y %T\"                                              \n"
    contenu += "exit $?                                                            \n"
    fichier.write(contenu)
    fichier.close()
    os.system("chmod 744 run_saturne_master.sh")
