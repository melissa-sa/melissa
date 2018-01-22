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
    user defined options
"""

import os
import numpy as np
import time
import sys
import re
import subprocess

USER_OPTIONS = {}
RANGE_MIN_PARAM = np.zeros(6, float)
RANGE_MAX_PARAM = np.ones(6, float)
RANGE_MIN_PARAM[0:2] = 0.1
RANGE_MAX_PARAM[0:2] = 0.9
RANGE_MIN_PARAM[2:4] = 0.1
RANGE_MAX_PARAM[2:4] = 0.9
RANGE_MIN_PARAM[4:6] = 0.001
RANGE_MAX_PARAM[4:6] = 0.1
BATCH_SCHEDULER = "Slurm"
XML_FILE_NAME = "bundle_3x2_f16_param.xml"

def call_bash(string):
    """
        Launches subprocess and returns out and err messages
    """
    proc = subprocess.Popen(string,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            shell=True,
                            universal_newlines=True)
    (out, err) = proc.communicate()
    return{'out':remove_end_of_line(out),
           'err':remove_end_of_line(err)}

def remove_end_of_line(string):
    """
        remove "\n" at end of a string
    """
    if len(string) > 0:
        return str(string[:len(string)-int(string[-1] == "\n")])
    else:
        return ""

def create_coupling_parameters (nb_parameters,
                                n_procs_weight,
                                n_procs_min,
                                n_procs_max):
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

def create_run_coupling (workdir,
                         nodes_saturne,
                         proc_per_node_saturne,
                         nb_parameters,
                         openmp_threads,
                         saturne_path,
                         walltime_container,
                         batch_scheduler):
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

def create_run_study (workdir,
                      nodes_melissa,
                      openmp_threads,
                      server_path,
                      walltime_melissa,
                      mpi_options,
                      options,
                      batch_scheduler):
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

def create_runcase_sobol (workdir,
                          openmp_threads,
                          saturne_path):
    # script to launch simulations
    contenu=""
    fichier=open("run_saturne.sh", "w")
    contenu += "#!/bin/bash                                        \n"
    contenu += "date +\"%d/%m/%y %T\"                              \n"
    contenu += "export OMP_NUM_THREADS="+str(openmp_threads)+"     \n"
    contenu += "# Ensure the correct command is found:             \n"
    contenu += "export PATH="+saturne_path+"/:$PATH                \n"
    contenu += "# copy server name:                                \n"
    contenu += "# cp "+workdir+"/server_name.txt .                 \n"
    contenu += "# Run command:                                     \n"
    contenu += "\code_saturne run --param "+XML_FILE_NAME+"        \n"
    contenu += "date +\"%d/%m/%y %T\"                              \n"
    contenu += "exit $?                                            \n"
    fichier.write(contenu)
    fichier.close()
    os.system("chmod 744 run_saturne.sh")

def create_runcase (workdir,
                    nodes_saturne,
                    proc_per_node_saturne,
                    openmp_threads,
                    saturne_path,
                    walltime_saturne,
                    batch_scheduler):
    contenu=""
    fichier=open("run_saturne.sh", "w")
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
        contenu += "#MSUB -T "+walltime_saturne+"                                                       \n"
        contenu += "#MSUB -A gen10064                                                                     \n"
        contenu += "#MSUB -q standard                                                                     \n"
    contenu += "export OMP_NUM_THREADS="+str(openmp_threads)+"                     \n"
    contenu += "export PATH="+saturne_path+"/:$PATH                                \n"
    contenu += "date +\"%d/%m/%y %T\"                                              \n"
    contenu += "\code_saturne run --param "+XML_FILE_NAME+"                        \n"
    contenu += "date +\"%d/%m/%y %T\"                                              \n"
    contenu += "exit $?                                                            \n"
    fichier.write(contenu)
    fichier.close()
    os.system("chmod 744 run_saturne.sh")


def draw_param_set():
    param_set = np.zeros(STUDY_OPTIONS['nb_parameters'])
    for i in range(STUDY_OPTIONS['nb_parameters']):
        param_set[i] = np.random.uniform(RANGE_MIN_PARAM[i],
                                         RANGE_MAX_PARAM[i])
    return param_set

def check_job(job):
    state = 0
    if (BATCH_SCHEDULER == "OAR"):
        if (not str(job.job_id) in call_bash("oarstat -u --sql \"state = 'Waiting'\"")['out']):
            state = 1
            if (not str(job.job_id) in call_bash("oarstat -u --sql \"state = 'Running'\"")['out']):
                state = 2
    elif (BATCH_SCHEDULER == "Slurm") or (BATCH_SCHEDULER == "CCC"):
        if (not "PENDING" in call_bash("squeue --job="+str(job.job_id)+" -l")['out']):
            state = 1
            if (not "RUNNING" in call_bash("squeue --job="+str(job.job_id)+" -l")['out']):
                state = 2
                print "job "+str(job.job_id)+" finished"
    job.job_status = state

def cancel_job(job):
    print "cancel job "+str(job.job_id)
    if (BATCH_SCHEDULER == "Slurm" or BATCH_SCHEDULER == "CCC"):
        call_bash("scancel "+str(job.job_id))
    elif (BATCH_SCHEDULER == "OAR"):
        call_bash("oardel "+str(job.job_id))
    elif (BATCH_SCHEDULER == "local"):
        call_bash("kill "+str(job.job_id))

def create_simu(group):
    workdir = GLOBAL_OPTIONS['working_directory']
    os.chdir(workdir)
    if (not os.path.isdir(GLOBAL_OPTIONS['working_directory']+"/group"+str(group.rank))):
        create_case_str = SIMULATIONS_OPTIONS['path'] + \
                "/code_saturne create --noref -s group" + \
                str(group.rank) + \
                " -c rank0"
        if MELISSA_STATS['sobol_indices']:
            for j in range(STUDY_OPTIONS['nb_parameters'] + 1):
                create_case_str += " -c rank" + str(j+1)
#        create_case_str
        os.system(create_case_str)

    if MELISSA_STATS['sobol_indices']:
        # Only works for Sobol
        for simu in range(len(group.param_set)):
            casedir = workdir+"/group"+str(group.rank)+"/rank"+str(simu)
            os.system("cp "+workdir+"/case1/SCRIPTS/run_saturne.sh "+casedir+"/SCRIPTS/runcase")
            # modif xml file
            os.chdir(casedir+"/DATA")
            fichier=open(workdir+"/case1/DATA/"+XML_FILE_NAME, "r")
            contenu = ""
            for line in fichier:
                if not("melissa" in line):
                    contenu += line
                else:
                    contenu += re.sub('options=".*"','options="'+str(group.simu_id[simu])+'"',line)
            fichier.close()
            fichier = open(XML_FILE_NAME, 'w')
            fichier.write(contenu)
            fichier.close()
            os.chdir(casedir+"/SRC")
            #modif fortran routine
            fichier = open(workdir+'/case1/SRC/cs_user_boundary_conditions.f90', 'r')
            contenu = ""
            for line in fichier:
                if ("param_intensite_haut=" in line):
                    contenu += 'param_intensite_haut='+str(group.param_set[simu][0])+'\n'
                elif ("param_intensite_bas=" in line):
                    contenu += 'param_intensite_bas='+str(group.param_set[simu][1])+'\n'
                elif ("param_largeur_haut=" in line):
                    contenu += 'param_largeur_haut='+str(group.param_set[simu][2])+'\n'
                elif ("param_largeur_bas=" in line):
                    contenu += 'param_largeur_bas='+str(group.param_set[simu][3])+'\n'
                elif ("param_duree_injection_haut=" in line):
                    contenu += 'param_duree_injection_haut='+str(group.param_set[simu][4])+'\n'
                elif ("param_duree_injection_bas=" in line):
                    contenu += 'param_duree_injection_bas='+str(group.param_set[simu][5])+'\n'
                else:
                    contenu += line
            fichier.close()
            fichier = open('cs_user_boundary_conditions.f90', 'w')
            fichier.write(contenu)
            fichier.close()
            os.system("cp "+workdir+"/case1/SRC/cs_user_mesh.c "+casedir+"/SRC/cs_user_mesh.c")

    else:

        casedir = workdir+"/group"+str(group.simu_id)+"/rank0"
        os.system("cp "+workdir+"/case1/SCRIPTS/run_saturne.sh "+casedir+"/SCRIPTS/runcase")
        # modif xml file
        os.chdir(casedir+"/DATA")
        fichier=open(workdir+"/case1/DATA/"+XML_FILE_NAME, "r")
        contenu = ""
        for line in fichier:
            if not("melissa" in line):
                contenu += line
            else:
                contenu += re.sub('options=".*"','options="'+str(group.id)+'"',line)
        fichier.close()
        fichier = open(XML_FILE_NAME, 'w')
        fichier.write(contenu)
        fichier.close()
        os.chdir(casedir+"/SRC")
        #modif fortran routine
        fichier = open(workdir+'/case1/SRC/cs_user_boundary_conditions.f90', 'r')
        contenu = ""
        for line in fichier:
            if ("param_intensite_haut=" in line):
                contenu += 'param_intensite_haut='+str(group.param_set[0])+'\n'
            elif ("param_intensite_bas=" in line):
                contenu += 'param_intensite_bas='+str(group.param_set[1])+'\n'
            elif ("param_largeur_haut=" in line):
                contenu += 'param_largeur_haut='+str(group.param_set[2])+'\n'
            elif ("param_largeur_bas=" in line):
                contenu += 'param_largeur_bas='+str(group.param_set[3])+'\n'
            elif ("param_duree_injection_haut=" in line):
                contenu += 'param_duree_injection_haut='+str(group.param_set[4])+'\n'
            elif ("param_duree_injection_bas=" in line):
                contenu += 'param_duree_injection_bas='+str(group.param_set[5])+'\n'
            else:
                contenu += line
        fichier.close()
        fichier = open('cs_user_boundary_conditions.f90', 'w')
        fichier.write(contenu)
        fichier.close()
        os.system("cp "+workdir+"/case1/SRC/cs_user_mesh.c "+casedir+"/SRC/cs_user_mesh.c")
    return 0

def create_study():
    if not os.path.isdir(GLOBAL_OPTIONS['working_directory']+"/STATS"):
        os.mkdir(GLOBAL_OPTIONS['working_directory']+"/STATS")
    os.chdir(GLOBAL_OPTIONS['working_directory']+"/STATS")
    if MELISSA_STATS['sobol_indices']:
        create_run_coupling (GLOBAL_OPTIONS['working_directory'],
                             SIMULATIONS_OPTIONS['nb_nodes'],
                             SIMULATIONS_OPTIONS['nb_proc'],
                             STUDY_OPTIONS['nb_parameters'],
                             2,
                             SIMULATIONS_OPTIONS['path'],
                             SIMULATIONS_OPTIONS['walltime'],
                             BATCH_SCHEDULER)
    if (not os.path.isdir(GLOBAL_OPTIONS['working_directory']+"/case1/SCRIPTS")):
        os.mkdir(GLOBAL_OPTIONS['working_directory']+"/case1/SCRIPTS")
    os.chdir(GLOBAL_OPTIONS['working_directory']+"/case1/SCRIPTS")
    if MELISSA_STATS['sobol_indices']:
        create_runcase_sobol (GLOBAL_OPTIONS['working_directory'],
                              2,
                              SIMULATIONS_OPTIONS['path'])
    else:
        create_runcase (GLOBAL_OPTIONS['working_directory'],
                        SIMULATIONS_OPTIONS['nb_nodes'],
                        SIMULATIONS_OPTIONS['nb_proc'],
                        2,
                        SIMULATIONS_OPTIONS['path'],
                        SIMULATIONS_OPTIONS['walltime'],
                        BATCH_SCHEDULER)

def launch_simu(simu):
    os.chdir(GLOBAL_OPTIONS['working_directory']+"/group"+str(simu.rank))
    if MELISSA_STATS['sobol_indices']:
        create_coupling_parameters (STUDY_OPTIONS['nb_parameters'],
                                    "None",
                                    SIMULATIONS_OPTIONS['nb_nodes'] * SIMULATIONS_OPTIONS['nb_proc'],
                                    "None")
        if (BATCH_SCHEDULER == "Slurm"):
            simu.job_id = int(call_bash('sbatch "../STATS/run_cas_couple.sh" --exclusive --job-name=Saturnes'+str(simu.rank))['out'].split()[-1])
        elif (BATCH_SCHEDULER == "CCC"):
            simu.job_id = int(call_bash('ccc_msub -r Saturne'+str(simu.rank)+' "../STATS/run_cas_couple.sh"')['out'].split()[-1])
        elif (BATCH_SCHEDULER == "OAR"):
            simu.job_id = int(call_bash('oarsub -S "../STATS/run_cas_couple.sh" -n Saturnes'+str(simu.rank)+' --project=avido')['out'].split("OAR_JOB_ID=")[1])

    else:
        os.chdir("./rank0/SCRIPTS")
        if (BATCH_SCHEDULER == "Slurm"):
            simu.job_id = int(call_bash('sbatch "./runcase" --exclusive --job-name=Saturne'+str(simu.rank))['out'].split()[-1])
        elif (BATCH_SCHEDULER == "CCC"):
            simu.job_id = int(call_bash('ccc_msub "./runcase"')['out'].split()[-1])
        elif (BATCH_SCHEDULER == "OAR"):
            simu.job_id = int(call_bash('oarsub -S "./runcase" -n Saturne'+str(simu.rank)+' --project=avido')['out'].split("OAR_JOB_ID=")[1])


def launch_server(server):
    if (not os.path.isdir(GLOBAL_OPTIONS['working_directory']+"/STATS")):
        os.mkdir(GLOBAL_OPTIONS['working_directory']+"/STATS")
    os.chdir(GLOBAL_OPTIONS['working_directory']+"/STATS")
    options = server.cmd_opt
    create_run_study (GLOBAL_OPTIONS['working_directory'],
                      SERVER_OPTIONS['nb_nodes'],
                      1,
                      server.path,
                      SERVER_OPTIONS['walltime'],
                      '',
                      options,
                      BATCH_SCHEDULER)
    os.chdir(GLOBAL_OPTIONS['working_directory']+"/STATS")
    if (BATCH_SCHEDULER == "Slurm"):
        server.job_id = call_bash('sbatch "./run_study.sh"')['out'].split()[-1]
    elif (BATCH_SCHEDULER == "CCC"):
        server.job_id = call_bash('ccc_msub -r Melissa "./run_study.sh"')['out'].split()[-1]
    elif (BATCH_SCHEDULER == "OAR"):
        server.job_id = call_bash('oarsub -S "./run_study.sh" --project=avido')['out'].split("OAR_JOB_ID=")[1]
    os.chdir(GLOBAL_OPTIONS['working_directory'])


def restart_server(server):
    os.chdir(GLOBAL_OPTIONS['working_directory']+"/STATS")
    create_reboot_study(GLOBAL_OPTIONS['working_directory'],
                        SERVER_OPTIONS['nb_nodes'],
                        1,
                        server.path,
                        SERVER_OPTIONS['walltime'],
                        server.mpi_options,
                        server.cmd_opt,
                        BATCH_SCHEDULER,
                        server.first_job_id)
    if (BATCH_SCHEDULER == "Slurm" or BATCH_SCHEDULER == "CCC"):
        call_bash("scancel "+server.job_id)
    elif (BATCH_SCHEDULER == "OAR"):
        call_bash("oardel "+server.job_id)
    elif (BATCH_SCHEDULER == "local"):
        call_bash("kill "+server.job_id)
    if (BATCH_SCHEDULER == "Slurm"):
        server.job_id = call_bash('sbatch "./reboot_study.sh"')['out'].split()[-1]
    elif (BATCH_SCHEDULER == "CCC"):
        server.job_id = call_bash('ccc_msub -r Melissa "./reboot_study.sh"')['out'].split()[-1]
    elif (BATCH_SCHEDULER == "OAR"):
        server.job_id = call_bash('oarsub -S "./reboot_study.sh" --project=avido')['out'].split("OAR_JOB_ID=")[1]
    os.chdir(GLOBAL_OPTIONS['working_directory'])

def restart_simu(simu):
    os.chdir(GLOBAL_OPTIONS['working_directory']+"/group"+str(simu.rank))
    if MELISSA_STATS['sobol_indices']:
        output = "Reboot simulation group "+str(simu.rank)+"\n"
        if (BATCH_SCHEDULER == "Slurm"):
            simu.job_id = call_bash('sbatch "../STATS/run_cas_couple.sh" --exclusive --job-name=Saturnes'+str(simu.rank))['out'].split()[-1]
        elif (BATCH_SCHEDULER == "CCC"):
            simu.job_id = call_bash('ccc_msub -r Saturne'+str(simu.rank)+' "../STATS/run_cas_couple.sh"')['out'].split()[-1]
        elif (BATCH_SCHEDULER == "OAR"):
            simu.job_id = call_bash('oarsub -S "../STATS/run_cas_couple.sh" -n Saturnes'+str(simu.rank)+' --project=avido')['out'].split("OAR_JOB_ID=")[1]
    else:
        output = "Reboot simulation "+str(simu.rank)+"\n"
        os.chdir("./rank0/SCRIPTS")
        if (BATCH_SCHEDULER == "Slurm"):
            simu.job_id = call_bash('sbatch "./runcase" --exclusive --job-name=Saturne'+str(simu.rank))['out'].split()[-1]
        elif (BATCH_SCHEDULER == "CCC"):
            simu.job_id = call_bash('ccc_msub -r Saturne'+str(simu.rank)+' "./runcase"')['out'].split()[-1]
        elif (BATCH_SCHEDULER == "OAR"):
            simu.job_id = call_bash('oarsub -S "./runcase" -n Saturne'+str(simu.rank)+' --project=avido')['out'].split("OAR_JOB_ID=")[1]
        elif (BATCH_SCHEDULER == "local"):
            simu.job_id = call_bash('./runcase & echo $!')['out']
    os.chdir(GLOBAL_OPTIONS['working_directory'])
    print output

def check_scheduler_load():
    if BATCH_SCHEDULER == "local":
#        try:
#            subprocess.check_output(["pidof",EXECUTABLE])
#            return False
#        except:
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

GLOBAL_OPTIONS = {}
GLOBAL_OPTIONS['user_name'] = "terrazth"
GLOBAL_OPTIONS['working_directory'] = "/ccc/scratch/cont003/gen10064/terrazth/etude_plantage"

STUDY_OPTIONS = {}
STUDY_OPTIONS['nb_parameters'] = 6
STUDY_OPTIONS['sampling_size'] = 1000
STUDY_OPTIONS['nb_time_steps'] = 100
STUDY_OPTIONS['threshold_value'] = 0.4
STUDY_OPTIONS['nb_fields'] = 1
STUDY_OPTIONS['field_names'] = ["scalar1"]
STUDY_OPTIONS['simulation_timeout'] = 300
STUDY_OPTIONS['checkpoint_interval'] = 300

SERVER_OPTIONS = {}
SERVER_OPTIONS['walltime'] = '3600'
SERVER_OPTIONS['nb_nodes'] = 32
SERVER_OPTIONS['nb_proc'] = 16

SIMULATIONS_OPTIONS = {}
SIMULATIONS_OPTIONS['path'] = "/ccc/cont003/home/gen10064/terrazth/code_saturne/4.3.1/code_saturne-4.3.1/arch/Linux_x86_64/bin"
SIMULATIONS_OPTIONS['walltime'] = '600'
SIMULATIONS_OPTIONS['nb_nodes'] = 4
SIMULATIONS_OPTIONS['nb_proc'] = 8

MELISSA_STATS = {}
MELISSA_STATS['mean'] = True
MELISSA_STATS['variance'] = True
MELISSA_STATS['min'] = True
MELISSA_STATS['max'] = True
MELISSA_STATS['threshold_exceedance'] = True
MELISSA_STATS['quantile'] = False
MELISSA_STATS['sobol_indices'] = True

USER_FUNCTIONS = {}
USER_FUNCTIONS['create_study'] = create_study
USER_FUNCTIONS['draw_parameter_set'] = draw_param_set
USER_FUNCTIONS['create_group'] = create_simu
USER_FUNCTIONS['launch_group'] = launch_simu
USER_FUNCTIONS['launch_server'] = launch_server
USER_FUNCTIONS['check_server_job'] = check_job
USER_FUNCTIONS['check_group_job'] = check_job
USER_FUNCTIONS['restart_server'] = restart_server
USER_FUNCTIONS['restart_group'] = restart_simu
USER_FUNCTIONS['check_scheduler_load'] = check_scheduler_load
USER_FUNCTIONS['cancel_job'] = cancel_job
USER_FUNCTIONS['postprocessing'] = None
USER_FUNCTIONS['finalize'] = None
