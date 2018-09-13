
import os
import time
import subprocess
import numpy as np
import xml.etree.ElementTree as ET

#global variables
param1 = []
param2 = []

def draw_param_set():
    param_set = np.zeros(STUDY_OPTIONS['nb_parameters'])
    RANGE_MIN_PARAM = np.zeros(STUDY_OPTIONS['nb_parameters'], float)
    RANGE_MAX_PARAM = np.ones(STUDY_OPTIONS['nb_parameters'], float)
    RANGE_MIN_PARAM[0] = 200
    RANGE_MAX_PARAM[0] = 500
    RANGE_MIN_PARAM[1] = 1
    RANGE_MAX_PARAM[1] = 2
    for i in range(STUDY_OPTIONS['nb_parameters']):
        param_set[i] = np.random.uniform(RANGE_MIN_PARAM[i],
                                         RANGE_MAX_PARAM[i])
    param_set[1]=param_set[1]*10**(-1*np.random.randint(0,5))
    param1.append([param_set[0]])
    param2.append([param_set[1]])
    return param_set

def call_bash(string):
    proc = subprocess.Popen(string,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            shell=True,
                            universal_newlines=True)
    (out, err) = proc.communicate()
    return{'out':remove_end_of_line(out),
           'err':remove_end_of_line(err)}

def remove_end_of_line(string):
    if len(string) > 0:
        return str(string[:len(string)-int(string[-1] == "\n")])
    else:
        return ""

def create_runcase():
    os.chdir(STUDY_OPTIONS['working_directory']+"/case1/SCRIPTS")
    ntasks = (int(STUDY_OPTIONS['proc_per_node']) / int(STUDY_OPTIONS['openmp_threads']))*int(STUDY_OPTIONS['nodes_simu'])
    contenu=""
    fichier=open("run_saturne.sh", "w")
    contenu += "#!/bin/bash                                                        \n"
    contenu += "#SBATCH -N "+str(STUDY_OPTIONS['nodes_simu'])+"                    \n"
    contenu += "#SBATCH --ntasks="+str(ntasks)+"                                   \n"
    contenu += "#SBATCH --cpus-per-task="+str(STUDY_OPTIONS['openmp_threads'])+"   \n"
    contenu += "#SBATCH --wckey="+STUDY_OPTIONS['wckey']+"                         \n"
    contenu += "#SBATCH --partition=cn                                             \n"
    contenu += "#SBATCH --time="+str(STUDY_OPTIONS['walltime'])+"                  \n"
    contenu += "#SBATCH -o saturne.%j.log                                          \n"
    contenu += "#SBATCH -e saturne.%j.err                                          \n"
    contenu += "module load openmpi/2.0.1                                          \n"
    contenu += "module load ifort/2017                                             \n"
    contenu += "export OMP_NUM_THREADS="+str(STUDY_OPTIONS['openmp_threads'])+"    \n"
    contenu += "export PATH="+STUDY_OPTIONS['saturne_path']+"/:$PATH               \n"
    contenu += "date +\"%d/%m/%y %T\"                                              \n"
    contenu += "\code_saturne run --param "+STUDY_OPTIONS['xml_name']+"            \n"
    contenu += "date +\"%d/%m/%y %T\"                                              \n"
    contenu += "exit $?                                                            \n"
    fichier.write(contenu)
    fichier.close()
    os.system("chmod 744 run_saturne.sh")
             
def check_scheduler_load():
    ret = call_bash("squeue -u "+STUDY_OPTIONS['user_name']+" | wc -l")
    running_jobs = int(ret["out"])
    return running_jobs < 250

class Simu(object):
    def __init__(self, rank, param_set):
        self.rank = rank
        self.param_set = param_set
        self.create()
    
    def create(self):
        workdir = STUDY_OPTIONS['working_directory']
        os.chdir(workdir)
        if (not os.path.isdir(STUDY_OPTIONS['working_directory']+"/group"+str(self.rank))):
            create_case_str = STUDY_OPTIONS['saturne_path'] + \
                    "/code_saturne create --noref -s group" + \
                    str(self.rank) + \
                    " -c rank0"
            os.system(create_case_str)
    
        casedir = workdir+"/group"+str(self.rank)+"/rank0"
        os.system("cp "+workdir+"/case1/SCRIPTS/run_saturne.sh "+casedir+"/SCRIPTS/runcase")
        os.chdir(casedir+"/SRC")

        #modif xml file
        tree = ET.parse(workdir+'/case1/DATA/case1.xml')
        root = tree.getroot()
        # modif parameters
        param0 = 'temperature = '+str(self.param_set[0])+';'
        root.find('thermophysical_models').find('thermal_scalar').find('variable').find('formula').text = param0 
        root.find('physical_properties').find('fluid_properties').find("property/[@name='molecular_viscosity']").find('initial_value').text = str(self.param_set[1])
        # modif path to mesh file directory
        meshlist = root.find('solution_domain').find('meshes_list')
        if meshlist.find('meshdir') is None:
            meshdir=ET.SubElement(meshlist, 'meshdir')
            meshdir.set('name', '../../MESH')
        else:
            meshlist.find('meshdir').set('name', '../../MESH')

        tree.write(casedir+'/DATA/case1.xml')
        return 0
    
    def launch(self):
        os.chdir(STUDY_OPTIONS['working_directory']+"/group"+str(self.rank)+"/rank0/SCRIPTS")
        call_bash('sbatch "./runcase" --exclusive --job-name=Saturne'+str(self.rank))
        
def main():
    simulations=[]
    create_runcase()
    for i in range(STUDY_OPTIONS['sampling_size']):
        simulations.append(Simu(i, draw_param_set()))
    #save parameters used for each simulation
    
    X1 = np.array(param1)
    X2 = np.array(param2)
    
    print(X2)
    np.savetxt(STUDY_OPTIONS['working_directory']+'/param1.out',X1)
    np.savetxt(STUDY_OPTIONS['working_directory']+'/param2.out',X2)
    # you can save the parameter sets here
    for simu in simulations:
        simu.launch()
        while check_scheduler_load() is False:
            time.sleep(10)

STUDY_OPTIONS = {}
# your usernam on the supercomputer
STUDY_OPTIONS['user_name'] = "f77878"
# the main working directory, that must contain a pre-configured case named "case1 and directory "MESH" which should contain all mesh files used"
STUDY_OPTIONS['working_directory'] = "/scratch/f77878/saturn/full_domain_simu/keras_with_time"
# Your wckey on the supercomuter folowed by the project name
STUDY_OPTIONS['wckey'] = "P11UK:AVIDO"
# the name of the xml case file in the reference case named "case1"
STUDY_OPTIONS['xml_name'] = "case1.xml"
# path to Code_Saturne binary dir
STUDY_OPTIONS['saturne_path'] = "/projets/saturne/Code_Saturne/5.2/arch/eole/bin"
# simulations walltime
STUDY_OPTIONS['walltime'] = "00:30:00"
# nb of cores on one node
STUDY_OPTIONS['proc_per_node'] = 2
# number of nodes for one simulation
STUDY_OPTIONS['nodes_simu'] = 1
# number of OpenMP threads for each MPI processus
STUDY_OPTIONS['openmp_threads'] = 2
# number of varying parameters of your parametric study
STUDY_OPTIONS['nb_parameters'] = 2
# number of simulations of your study
STUDY_OPTIONS['sampling_size'] = 150000


if __name__ == '__main__':
    main()
             
             
