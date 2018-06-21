 
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
    User defined options module
"""
import shutil
import os
from time import sleep
import numpy as np
import subprocess
import getpass
from matplotlib import pyplot as plt
from matplotlib import cm
from string import Template

NODES_SERVER = 1
NODES_GROUP = 1

def create_flowvr_group(executable, args, group_id, nb_proc_simu, nb_parameters):
    content = ""
    file=open(STUDY_OPTIONS['working_directory']+"/create_group.py", "r")
    content = Template(file.read()).substitute(args=str(args),
                                     group_id=str(group_id),
                                     np_simu=str(nb_proc_simu),
                                     nb_param=str(nb_parameters),
                                     executable=str(executable))
    file.close()
    file=open("create_group"+str(group_id)+".py", "w")
    file.write(content)
    file.close()

def draw_param_set():
    param_set = np.zeros(STUDY_OPTIONS['nb_parameters'])
    param_set[0] = np.random.uniform(0, 1)
    param_set[1] = np.random.uniform(5.05, 15.05)
    param_set[2] = np.random.uniform(5.05, 15.05)
    return param_set

def launch_server(server):
    if (not os.path.isdir(STUDY_OPTIONS['working_directory'])):
        os.mkdir(STUDY_OPTIONS['working_directory'])
    os.chdir(STUDY_OPTIONS['working_directory'])
    server.job_id = subprocess.Popen(('mpirun ' +
                                      ' -n '+str(NODES_SERVER) +
                                      ' ' + server.path +
                                      '/melissa_server ' +
                                      server.cmd_opt +
                                      ' &').split()).pid
    os.chdir(STUDY_OPTIONS['working_directory'])
    
    
def create_simu(group):
    workdir = STUDY_OPTIONS['working_directory']
    os.chdir(workdir)
    contenu = ""
    fichier = open(workdir + "/t2d_gouttedo.cas", 'r')
    for line in fichier:
        if ("BOUNDARY CONDITIONS FILE" in line):
            contenu += 'BOUNDARY CONDITIONS FILE        : \'../../geo_gouttedo.cli\'\n'
        elif ("GEOMETRY FILE" in line):
            contenu += 'GEOMETRY FILE                   : \'../../geo_gouttedo.slf\'\n'
        else:
            contenu += line
    fichier.close()
            
    if (not os.path.isdir(workdir+"/group" + str(group.rank))):
        shutil.copytree("./user_fortran", "group"+str(group.rank) + "/rank0/user_fortran")
        fichier = open("group"+str(group.rank) + "/rank0/t2d_gouttedo.cas", 'w')
        fichier.write(contenu)
        fichier.close()
        
        if MELISSA_STATS['sobol_indices']:
            for j in range(STUDY_OPTIONS['nb_parameters'] + 1):
                shutil.copytree("./user_fortran", "group"+str(group.rank) + "/rank" + str(j+1) + "/user_fortran")
                fichier = open("group"+str(group.rank) + "/rank" + str(j+1) + "/t2d_gouttedo.cas", 'w')
                fichier.write(contenu)
                fichier.close()

    if MELISSA_STATS['sobol_indices']:
        # Only works for Sobol
        for simu in range(len(group.param_set)):
            casedir = workdir+"/group"+str(group.rank)+"/rank"+str(simu)
            #modif fortran routine
            fichier = open(casedir + "/user_fortran/condin.f", 'r')
            contenu = ""
            for line in fichier:
                if ("H%R(IPOIN)" in line):
                    contenu += '        H%R(IPOIN)=2.4D0*(1.D0+EXP(-EIKON))+'+str(group.param_set[simu][0])+'D0\n'
                elif ("EIKON=(" in line):
                    contenu += '        EIKON=( (X(IPOIN)-'+str(round(group.param_set[simu][1],4))+'D0)**2+(Y(IPOIN)-'+str(round(group.param_set[simu][2],4))+'D0)**2)/4.D0\n'
                elif ("MELISSA =" in line):
                    contenu += '      MELISSA = .TRUE.\n'
                elif ("SIMU_ID" in line):
                    contenu += '      SIMU_ID = '+ str(group.simu_id[simu]) +'\n'
                else:
                    contenu += line
            fichier.close()
            fichier = open(casedir + "/user_fortran/condin.f", 'w')
            fichier.write(contenu)
            fichier.close()

    else:
        contenu = ""
        casedir = workdir+"/group"+str(group.rank)+"/rank0"
        #modif fortran routine
        fichier = open(casedir + "/user_fortran/condin.f", 'r')
        for line in fichier:
            if ("H%R(IPOIN)" in line):
                contenu += '        H%R(IPOIN)=2.4D0*(1.D0+EXP(-EIKON))+'+str(group.param_set[0])+'D0\n'
            elif ("EIKON=(" in line):
                contenu += '        EIKON=( (X(IPOIN)-'+str(round(group.param_set[1],4))+'D0)**2+(Y(IPOIN)-'+str(round(group.param_set[2],4))+'D0)**2)/4.D0\n'
            elif ("MELISSA =" in line):
                contenu += '      MELISSA = .TRUE.\n'
            elif ("SIMU_ID" in line):
                contenu += '      SIMU_ID = '+ str(group.simu_id) +'\n'
            else:
                contenu += line
        fichier.close()
        fichier = open(casedir + "/user_fortran/condin.f", 'w')
        fichier.write(contenu)
        fichier.close()
    return 0


def launch_simu(simulation):
    if (not os.path.isdir(STUDY_OPTIONS['working_directory']+"/group"+str(simulation.rank))):
        os.mkdir(STUDY_OPTIONS['working_directory']+"/group"+str(simulation.rank))
    os.chdir(STUDY_OPTIONS['working_directory']+"/group"+str(simulation.rank))
    shutil.copyfile(STUDY_OPTIONS['working_directory']+'/server_name.txt' , './server_name.txt')
    if MELISSA_STATS['sobol_indices']:
       if STUDY_OPTIONS['coupling'] == "MELISSA_COUPLING_FLOWVR":
           args = ['./rank'+str(i)+'/t2d_gouttedo.cas' for i in range(STUDY_OPTIONS['nb_parameters'] + 2)]
           create_flowvr_group('telemac2d.py',
                               args,
                               simulation.rank,
                               int(NODES_GROUP),
                               STUDY_OPTIONS['nb_parameters'])
           os.system('python create_group'+str(simulation.rank)+'.py')
           os.system('flowvrd &')
           simulation.job_id = subprocess.Popen('flowvr group'+str(simulation.rank), shell=True).pid
       else:
            for i in range(STUDY_OPTIONS['nb_parameters'] + 2):
                os.chdir(STUDY_OPTIONS['working_directory']+"/group"+str(simulation.rank) + "/rank" + str(i))
                shutil.copyfile(STUDY_OPTIONS['working_directory']+'/server_name.txt' , './server_name.txt')
                command = 'telemac2d.py t2d_gouttedo.cas'
                print command
                simulation.job_id = subprocess.Popen(command.split()).pid

    else:
        os.chdir(STUDY_OPTIONS['working_directory']+"/group"+str(simulation.rank) + "/rank0")
        shutil.copyfile(STUDY_OPTIONS['working_directory']+'/server_name.txt' , './server_name.txt')
        command = 'telemac2d.py t2d_gouttedo.cas'
        print command
        simulation.job_id = subprocess.Popen(command.split()).pid

    os.chdir(STUDY_OPTIONS['working_directory'])

def check_job(job):
    state = 0
    try:
        subprocess.check_output(["ps",str(job.job_id)])
        state = 1
    except:
        state = 2
    job.job_status = state

def check_load():
    try:
        sleep(10)
        subprocess.check_output(["pidof","out_user_fortran"])
        return False
    except:
        os.system('flowvr-kill')
        os.system('killall flowvrd')
        return True

def kill_job(job):
    print 'killing job ...'
    os.system('kill '+str(job.job_id))
    
def convert_to_serafin():
    os.system('./melissa_to_serafin')
    
def postrocessing():
    os.chdir(STUDY_OPTIONS['working_directory'])
    os.system('flowvr-kill')
    os.system('killall flowvrd')
    os.system('./melissa_to_serafin')
    shutil.rmtree('./group*')
    
        
STUDY_OPTIONS = {}
STUDY_OPTIONS['user_name'] = getpass.getuser()
STUDY_OPTIONS['working_directory'] = '@CMAKE_INSTALL_PREFIX@/share/examples/droplet_example'
STUDY_OPTIONS['nb_parameters'] = 3          # number of varying parameters of the study
STUDY_OPTIONS['sampling_size'] = 30         # initial number of parameter sets
STUDY_OPTIONS['nb_time_steps'] = 20         # number of timesteps, from Melissa point of view
STUDY_OPTIONS['threshold_value'] = 0.7
STUDY_OPTIONS['field_names'] = ["WATER_DEPTH_____",
                                "VELOCITY_U______",
                                "VELOCITY_V______"]     # list of field names
STUDY_OPTIONS['simulation_timeout'] = 40    # simulations are restarted if no life sign for 40 seconds
STUDY_OPTIONS['checkpoint_interval'] = 30   # server checkpoints every 30 seconds
STUDY_OPTIONS['coupling'] = "MELISSA_COUPLING_FLOWVR"

MELISSA_STATS = {}
MELISSA_STATS['mean'] = True
MELISSA_STATS['variance'] = True
MELISSA_STATS['min'] = True
MELISSA_STATS['max'] = True
MELISSA_STATS['threshold_exceedance'] = False
MELISSA_STATS['quantile'] = False
MELISSA_STATS['sobol_indices'] = False

USER_FUNCTIONS = {}
USER_FUNCTIONS['create_study'] = None
USER_FUNCTIONS['draw_parameter_set'] = draw_param_set
USER_FUNCTIONS['create_group'] = create_simu
#if MELISSA_STATS['sobol_indices']:
#    USER_FUNCTIONS['launch_group'] = launch_group
#else:
USER_FUNCTIONS['launch_group'] = launch_simu
USER_FUNCTIONS['launch_server'] = launch_server
USER_FUNCTIONS['check_server_job'] = check_job
USER_FUNCTIONS['check_group_job'] = check_job
USER_FUNCTIONS['restart_server'] = launch_server
USER_FUNCTIONS['restart_group'] = None
#USER_FUNCTIONS['check_scheduler_load'] = None
USER_FUNCTIONS['check_scheduler_load'] = check_load
USER_FUNCTIONS['cancel_job'] = kill_job
USER_FUNCTIONS['postprocessing'] = postrocessing
USER_FUNCTIONS['finalize'] = None
