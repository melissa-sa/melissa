
"""
    user defined options
"""

import os
import numpy as np
import batch_scripts

BATCH_SCHEDULER == "CCC"
xml_file_name = "bundle_3x2_f16_param.xml"

def check_job(job):
    state = 0
    if (BATCH_SCHEDULER == "OAR"):
        if (not job.job_id in call_bash("oarstat -u --sql \"state = 'Waiting'\"")['out']):
            state = 1
            if (not job.job_id in call_bash("oarstat -u --sql \"state = 'Running'\"")['out']):
                state = 2
    elif (BATCH_SCHEDULER == "Slurm") or (BATCH_SCHEDULER == "CCC"):
        if (not "PENDING" in call_bash("squeue --job="+job.job_id+" -l")['out']):
            state = 1
            if (not "RUNNING" in call_bash("squeue --job="+job.job_id+" -l")['out']):
                state = 2
    job.job_status = state

def cancel_job(job):
    global output
    output += "cancel job "+str(job.job_id)
    if (batch_scheduler == "Slurm" or batch_scheduler == "CCC"):
        call_bash("scancel "+job.job_id)
    elif (batch_scheduler == "OAR"):
        call_bash("oardel "+job.job_id)
    elif (batch_scheduler == "local"):
        call_bash("kill "+job.job_id)

def create_group(group):
    os.chdir(GLOBAL_OPTIONS['working_directory'])
    if (not os.path.isdir(GLOBAL_OPTIONS['working_directory']+"/group"+str(group.rank))):
        create_case_str = SIMULATIONS_OPTIONS['saturne_path'] + \
                "/code_saturne create --noref -s group" + \
                str(group.rank) + \
                " -c rank0"
        if MELISSA_STATS['sobol_indices']:
            for j in range(STUDY_OPTIONS['nb_parameters'] + 1):
                create_case_str += " -c rank" + str(j+1)
#        create_case_str
        os.system(create_case_str)
    for j in range(STUDY_OPTIONS['nb_parameters'] + 2):
        create_simu(group.simulations[j])


def create_simu(simu):
    work_dir = GLOBAL_OPTIONS['working_directory']
    if (simu.sobol_id > 0):
        parameters = str(simu.sobol_id)+":"+str(simu.rank)
        casedir = workdir+"/group"+str(simu.rank)+"/rank"+str(simu.sobol_id)
    else:
        parameters = "0:"+str(simu.sobol_id)
        casedir = workdir+"/group"+str(simu.sobol_id)+"/rank0"
#    os.system("cp "+workdir+"/case1/DATA/server_name.txt "+casedir+"/DATA")
    if (simu.sobol_id > 0):
        os.system("cp "+workdir+"/case1/SCRIPTS/run_saturne.sh "+casedir+"/SCRIPTS/runcase")
    else:
        os.system("cp "+workdir+"/case1/SCRIPTS/run_saturne_master.sh "+casedir+"/SCRIPTS/runcase")
    # modif xml file
    os.chdir(casedir+"/DATA")
    fichier=open(workdir+"/case1/DATA/"+xml_file_name, "r")
    contenu = ""
    for line in fichier:
        if not("melissa" in line):
            contenu += line
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
    for line in fichier:
        if ("param_intensite_haut=" in line):
            contenu += 'param_intensite_haut='+str(simu.param_set[0])+'\n'
        elif ("param_intensite_bas=" in line):
            contenu += 'param_intensite_bas='+str(simu.param_set[1])+'\n'
        elif ("param_largeur_haut=" in line):
            contenu += 'param_largeur_haut='+str(simu.param_set[2])+'\n'
        elif ("param_largeur_bas=" in line):
            contenu += 'param_largeur_bas='+str(simu.param_set[3])+'\n'
        elif ("param_duree_injection_haut=" in line):
            contenu += 'param_duree_injection_haut='+str(simu.param_set[4])+'\n'
        elif ("param_duree_injection_bas=" in line):
            contenu += 'param_duree_injection_bas='+str(simu.param_set[5])+'\n'
        else:
            contenu += ligne
    fichier.close()
    fichier = open('cs_user_boundary_conditions.f90', 'w')
    fichier.write(contenu)
    fichier.close()
    os.system("cp "+workdir+"/case1/SRC/cs_user_mesh.c "+casedir+"/SRC/cs_user_mesh.c")
    return 0

def create_study():
    global server
    if (not os.path.isdir(GLOBAL_OPTIONS['working_directory']+"/STATS")):
        os.mkdir(GLOBAL_OPTIONS['working_directory']+"/STATS")
    os.chdir(GLOBAL_OPTIONS['working_directory']+"/STATS")
    options = server.cmd_opt
    create_run_study (GLOBAL_OPTIONS['working_directory'],
                      SERVER_OPTIONS['nb_nodes'],
                      1,
                      SERVER_OPTIONS['path'],
                      SERVER_OPTIONS['walltime'],
                      '',
                      options,
                      BATCH_SCHEDULER)
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
                              SIMULATIONS_OPTIONS['nb_nodes'],
                              SIMULATIONS_OPTIONS['nb_proc'],
                              STUDY_OPTIONS['nb_parameters'],
                              2,
                              SIMULATIONS_OPTIONS['path'],
                              xml_file_name,
                              BATCH_SCHEDULER)
    else:
        create_runcase (GLOBAL_OPTIONS['working_directory'],
                        SIMULATIONS_OPTIONS['nb_nodes'],
                        SIMULATIONS_OPTIONS['nb_proc'],
                        2,
                        SIMULATIONS_OPTIONS['path'],
                        SIMULATIONS_OPTIONS['walltime'],
                        xml_file_name,
                        BATCH_SCHEDULER)

def launch_simulation(simu):
    os.chdir(GLOBAL_OPTIONS['working_directory']+"/group"+str(simu.rank))
    if MELISSA_STATS['sobol_indices']:
        for j in range(STUDY_OPTIONS['nb_parameters']+2):
            casedir = GLOBAL_OPTIONS['working_directory']+"/group"+str(i)+"/rank"+str(j)
            os.system("cp "+GLOBAL_OPTIONS['working_directory']+"/case1/DATA/server_name.txt "+casedir+"/DATA")
        create_coupling_parameters (STUDY_OPTIONS['nb_parameters'],
                                    "None",
                                    SIMULATIONS_OPTIONS['nb_nodes'] * SIMULATIONS_OPTIONS['nb_proc'],
                                    "None")
        if (batch_scheduler == "Slurm"):
            simu.job_id = int(call_bash('sbatch "../STATS/run_cas_couple.sh" --exclusive --job-name=Saturnes'+str(simu.rank))['out'].split()[-1])
        elif (batch_scheduler == "CCC"):
            simu.job_id = int(call_bash('ccc_msub -r Saturne'+str(simu.rank)+' "../STATS/run_cas_couple.sh"')['out'].split()[-1])
        elif (batch_scheduler == "OAR"):
            simu.job_id = int(call_bash('oarsub -S "../STATS/run_cas_couple.sh" -n Saturnes'+str(simu.rank)+' --project=avido')['out'].split("OAR_JOB_ID=")[1])

    else:
        casedir = GLOBAL_OPTIONS['working_directory']+"/group"+str(simu.rank)+"/rank0"
        os.system("cp "+GLOBAL_OPTIONS['working_directory']+"/case1/DATA/server_name.txt "+casedir+"/DATA")
        os.chdir("./rank0/SCRIPTS")
        if (global_options.batch_scheduler == "Slurm"):
            simu.job_id = int(call_bash('sbatch "./runcase" --exclusive --job-name=Saturne'+str(simu.rank))['out'].split()[-1])
        elif (global_options.batch_scheduler == "CCC"):
            simu.job_id = int(call_bash('ccc_msub -r Saturne'+str(simu.rank)+' "./runcase"')['out'].split()[-1])
        elif (global_options.batch_scheduler == "OAR"):
            simu.job_id = int(call_bash('oarsub -S "./runcase" -n Saturne'+str(simu.rank)+' --project=avido')['out'].split("OAR_JOB_ID=")[1])

    job_states[i] = 1 # pending

def launc_server(server):
    os.chdir(GLOBAL_OPTIONS['working_directory']+"/STATS")
    if (batch_scheduler == "Slurm"):
        server.job_id = call_bash('sbatch "./run_study.sh"')['out'].split()[-1]
    elif (batch_scheduler == "CCC"):
        server.job_id = call_bash('ccc_msub -r Melissa "./run_study.sh"')['out'].split()[-1]
    elif (batch_scheduler == "OAR"):
        server.job_id = call_bash('oarsub -S "./run_study.sh" --project=avido')['out'].split("OAR_JOB_ID=")[1]
    os.chdir(GLOBAL_OPTIONS['working_directory'])
    server.first_job_id = server.job_id


def restart_server(server):
    os.chdir(GLOBAL_OPTIONS['working_directory']+"/STATS")
    create_reboot_study(GLOBAL_OPTIONS['working_directory'],
                        SERVER_OPTIONS['nb_nodes'],
                        1,
                        SERVER_OPTIONS['path'],
                        SERVER_OPTIONS['walltime'],
                        server.mpi_options,
                        server.cmd_opt,
                        batch_scheduler,
                        server.first_job_id)
    if (batch_scheduler == "Slurm" or batch_scheduler == "CCC"):
        call_bash("scancel "+server.job_id)
    elif (batch_scheduler == "OAR"):
        call_bash("oardel "+server.job_id)
    elif (batch_scheduler == "local"):
        call_bash("kill "+server.job_id)
    if (batch_scheduler == "Slurm"):
        server.job_id = call_bash('sbatch "./reboot_study.sh"')['out'].split()[-1]
    elif (batch_scheduler == "CCC"):
        server.job_id = call_bash('ccc_msub -r Melissa "./reboot_study.sh"')['out'].split()[-1]
    elif (batch_scheduler == "OAR"):
        server.job_id = call_bash('oarsub -S "./reboot_study.sh" --project=avido')['out'].split("OAR_JOB_ID=")[1]
    os.chdir(GLOBAL_OPTIONS['working_directory'])
    server.nb_restart += 1

def restart_simu(simu):
    if (batch_scheduler == "Slurm" or batch_scheduler == "CCC"):
        os.system("scancel "+simu.job_id)
    elif (batch_scheduler == "OAR"):
        os.system("oardel "+simu.job_id)
    elif (batch_scheduler == "OAR"):
        os.system("kill "+simu.job_id)
    os.chdir(GLOBAL_OPTIONS['working_directory']+"/group"+str(simu.rank))
    if ("sobol" in operations) or ("sobol_indices" in operations):
        output = "Reboot simulation group "+str(simu.rank)+"\n"
        if (batch_scheduler == "Slurm"):
            simu.job_id = call_bash('sbatch "../STATS/run_cas_couple.sh" --exclusive --job-name=Saturnes'+str(simu.rank))['out'].split()[-1]
        elif (batch_scheduler == "CCC"):
            simu.job_id = call_bash('ccc_msub -r Saturne'+str(simu.rank)+' "../STATS/run_cas_couple.sh"')['out'].split()[-1]
        elif (batch_scheduler == "OAR"):
            simu.job_id = call_bash('oarsub -S "../STATS/run_cas_couple.sh" -n Saturnes'+str(simu.rank)+' --project=avido')['out'].split("OAR_JOB_ID=")[1]
    else:
        output = "Reboot simulation "+str(simu.rank)+"\n"
        os.chdir("./rank0/SCRIPTS")
        if (batch_scheduler == "Slurm"):
            simu_job_id[simu_id] = call_bash('sbatch "./runcase" --exclusive --job-name=Saturne'+str(simu.rank))['out'].split()[-1]
        elif (batch_scheduler == "CCC"):
            simu_job_id[simu_id] = call_bash('ccc_msub -r Saturne'+str(simu.rank)+' "./runcase"')['out'].split()[-1]
        elif (batch_scheduler == "OAR"):
            simu_job_id[simu_id] = call_bash('oarsub -S "./runcase" -n Saturne'+str(simu.rank)+' --project=avido')['out'].split("OAR_JOB_ID=")[1]
        elif (batch_scheduler == "local"):
            simu_job_id[simu_id] = call_bash('./runcase & echo $!')['out']
    simu.job_state = 1
    simu.nb_restart += 1
    os.chdir(GLOBAL_OPTIONS['working_directory'])

def check_scheduler_load():

    if (batch_scheduler == "Slurm") or (batch_scheduler == "CCC"):
        while (int(call_bash("squeue -u "+GLOBAL_OPTIONS['user_name']+" | wc -l")['out']) >= 250):
            time.sleep(20)


GLOBAL_OPTIONS = {}
GLOBAL_OPTIONS['home_path'] = "/ccc/cont003/home/gen10064/terrazth"
GLOBAL_OPTIONS['user_name'] = "terrazth"
GLOBAL_OPTIONS['working_directory'] = "/ccc/scratch/cont003/gen10064/terrazth/etude_plantage"

STUDY_OPTIONS = {}
STUDY_OPTIONS['nb_parameters'] = 6
STUDY_OPTIONS['range_min_param'] = np.zeros(STUDY_OPTIONS['nb_parameters'],
                                            float)
STUDY_OPTIONS['range_max_param'] = np.ones(STUDY_OPTIONS['nb_parameters'],
                                           float)
STUDY_OPTIONS['range_max_param'][0:2] = 0.1
STUDY_OPTIONS['range_max_param'][0:2] = 0.9
STUDY_OPTIONS['range_max_param'][2:4] = 0.1
STUDY_OPTIONS['range_max_param'][2:4] = 0.9
STUDY_OPTIONS['range_max_param'][4:6] = 0.001
STUDY_OPTIONS['range_max_param'][4:6] = 0.1
STUDY_OPTIONS['sampling_size'] = 100
STUDY_OPTIONS['max_additional_samples'] = 20
STUDY_OPTIONS['nb_time_steps'] = 100
STUDY_OPTIONS['threshold_value'] = 0.4

SERVER_OPTIONS = {}
SERVER_OPTIONS['walltime'] = '3600'
SERVER_OPTIONS['nb_nodes'] = 32
SERVER_OPTIONS['nb_proc'] = 16
SERVER_OPTIONS['path'] = "/home/user/Melissa/build/server"
SERVER_OPTIONS['mpi_options'] = ""
SERVER_OPTIONS['timeout'] = 1000

SIMULATIONS_OPTIONS = {}
SIMULATIONS_OPTIONS['path'] = "/ccc/cont003/home/gen10064/terrazth/code_saturne/4.3.1/code_saturne-4.3.1/arch/Linux_x86_64/bin"
SIMULATIONS_OPTIONS['executable'] = "code_saturne"
SIMULATIONS_OPTIONS['walltime'] = '600'
SIMULATIONS_OPTIONS['nb_nodes'] = 4
SIMULATIONS_OPTIONS['nb_proc'] = 8
SIMULATIONS_OPTIONS['coupling'] = True
SIMULATIONS_OPTIONS['mpi_options'] = ""
SIMULATIONS_OPTIONS['timeout'] = 300

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
USER_FUNCTIONS['draw_parameter'] = np.random.uniform
USER_FUNCTIONS['create_simulation'] = create_simu
USER_FUNCTIONS['create_group'] = create_group
USER_FUNCTIONS['launch_simulation'] = launch_simulation
USER_FUNCTIONS['launch_group'] = launch_simulation
USER_FUNCTIONS['launch_server'] = launch_server
USER_FUNCTIONS['check_server_job'] = check_job
USER_FUNCTIONS['check_simulation_job'] = check_job
USER_FUNCTIONS['restart_server'] = restart_server
USER_FUNCTIONS['restart_simulation'] = restart_simu
USER_FUNCTIONS['restart_group'] = restart_simu
USER_FUNCTIONS['check_scheduler_load'] = check_scheduler_load
USER_FUNCTIONS['cancel_job'] = None
USER_FUNCTIONS['postprocessing'] = None
USER_FUNCTIONS['finalize'] = None
