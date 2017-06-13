import os
import time
import sys
import numpy as np
import re

from threading import Thread, RLock
from call_bash import *
from batch_scripts import create_reboot_study

timeout_simu = 300

#=======================================#
#               functions               #
#=======================================#

def to_seconds(date):
    if len(date) == 3:
        if len(date[0].split("-")) == 2:
            time = 24 * 3600 * int(date[0].split("-")[0]) + 3600 * int(date[0].split("-")[1]) + 60 * int(date[1]) + int(date[2])
        else:
            time = 3600 * int(date[0]) + 60 * int(date[1]) + int(date[2])
    if len(date) == 2:
        time = 60 * int(date[0]) + int(date[1])
    if len(date) == 1:
        time = int(date[0])
    return time

def check_job(batch_scheduler, username, job_id):
    state = "pending"
    if (batch_scheduler == "OAR"):
        if (not job_id in call_bash("oarstat -u --sql \"state = 'Waiting'\"")['out']):
            state = "running"
            if (not job_id in call_bash("oarstat -u --sql \"state = 'Running'\"")['out']):
                state = "terminated"
    elif (batch_scheduler == "Slurm") or (batch_scheduler == "CCC"):
        if (not "PENDING" in call_bash("squeue --job="+job_id+" -l")['out']):
            state = "running"
            if (not "RUNNING" in call_bash("squeue --job="+job_id+" -l")['out']):
                state = "terminated"
    return state

def reboot_simu(simu_id,
                simu_job_id,
                job_states,
                batch_scheduler,
                workdir,
                output,
                operations,
                simu_crash):
    if (batch_scheduler == "Slurm" or batch_scheduler == "CCC"):
        call_bash("scancel "+simu_job_id[simu_id])
    elif (batch_scheduler == "OAR"):
        call_bash("oardel "+simu_job_id[simu_id])
    elif (batch_scheduler == "OAR"):
        call_bash("kill "+simu_job_id[simu_id])
    os.chdir(workdir+"/group"+str(simu_id))
    if ("sobol" in operations) or ("sobol_indices" in operations):
        output = "Reboot simulation group "+str(simu_id)+"\n"
        if (batch_scheduler == "Slurm"):
            simu_job_id[simu_id] = call_bash('sbatch "../STATS/run_cas_couple.sh" --exclusive --job-name=Saturnes'+str(simu_id))['out'].split()[-1]
        elif (batch_scheduler == "CCC"):
            simu_job_id[simu_id] = call_bash('ccc_msub -r Saturne'+str(simu_id)+' "../STATS/run_cas_couple.sh"')['out'].split()[-1]
        elif (batch_scheduler == "OAR"):
            simu_job_id[simu_id] = call_bash('oarsub -S "../STATS/run_cas_couple.sh" -n Saturnes'+str(simu_id)+' --project=avido')['out'].split("OAR_JOB_ID=")[1]
        elif (batch_scheduler == "local"):
            simu_job_id[simu_id] = call_bash('../STATS/run_cas_couple.sh & echo $!')['out']
    else:
        output = "Reboot simulation "+str(simu_id)+"\n"
        os.chdir("./rank0/SCRIPTS")
        if (batch_scheduler == "Slurm"):
            simu_job_id[simu_id] = call_bash('sbatch "./runcase" --exclusive --job-name=Saturne'+str(simu_id))['out'].split()[-1]
        elif (batch_scheduler == "CCC"):
            simu_job_id[simu_id] = call_bash('ccc_msub "./runcase"')['out'].split()[-1]
        elif (batch_scheduler == "OAR"):
            simu_job_id[simu_id] = call_bash('oarsub -S "./runcase" -n Saturne'+str(simu_id)+' --project=avido')['out'].split("OAR_JOB_ID=")[1]
        elif (batch_scheduler == "local"):
            simu_job_id[simu_id] = call_bash('./runcase & echo $!')['out']
    job_states[simu_id] = 1
    simu_crash[simu_id] += 1
    os.chdir(workdir)
    return output

def reboot_server(workdir,
                  melissa_first_job_id,
                  melissa_job_id,
                  output,
                  batch_scheduler,
                  nodes_melissa,
                  openmp_threads,
                  server_path,
                  walltime_melissa,
                  mpi_options,
                  options):
    os.chdir(workdir+"/STATS")
    create_reboot_study(workdir,
                        nodes_melissa,
                        openmp_threads,
                        server_path,
                        walltime_melissa,
                        mpi_options,
                        options,
                        batch_scheduler,
                        melissa_first_job_id)
    if (batch_scheduler == "Slurm" or batch_scheduler == "CCC"):
        call_bash("scancel "+melissa_job_id)
    elif (batch_scheduler == "OAR"):
        call_bash("oardel "+melissa_job_id)
    elif (batch_scheduler == "local"):
        call_bash("kill "+melissa_job_id)
    if (batch_scheduler == "Slurm"):
        melissa_job_id = call_bash('sbatch "./reboot_study.sh"')['out'].split()[-1]
    elif (batch_scheduler == "CCC"):
        melissa_job_id = call_bash('ccc_msub -r Melissa "./reboot_study.sh"')['out'].split()[-1]
    elif (batch_scheduler == "OAR"):
        melissa_job_id = call_bash('oarsub -S "./reboot_study.sh" --project=avido')['out'].split("OAR_JOB_ID=")[1]
    elif (batch_scheduler == "local"):
        melissa_job_id = call_bash('./reboot_study.sh '+melissa_first_job_id+' & echo $!')['out']
    os.chdir(workdir)
    return melissa_job_id

def check_timeout(simu_id, simu_job_id, output, batch_scheduler):
    out = False
    if (batch_scheduler == "OAR"):
        elapsed_date = call_bash("oarstat -j "+simu_job_id[simu_id])['out'].split()[13].split(":")
    elif (batch_scheduler == "Slurm") or (batch_scheduler == "CCC"):
        elapsed_date = call_bash("squeue --job="+simu_job_id[simu_id]+" -h")['out'].split()[6].split(":")
    if (to_seconds(elapsed_date) > timeout_simu):
        out = True
        output += "Timeout detected on simulation "+str(simu_id)+"\n"
    return out

