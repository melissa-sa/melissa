
from study import *


#=====================================#
#               options               #
#=====================================#

batch_scheduler       = "Slurm"
nb_parameters         = 6
nb_simu               = 8000
nb_groups             = 1000
nb_time_steps         = 100
operations            = ["mean","variance","min","max","threshold","sobol"]
threshold             = 0.7
mpi_OAR_options       = "--mca orte_rsh_agent \"oarsh\" --mca btl openib,sm,self --mca pml ^cm --machinefile $OAR_NODE_FILE"
mpi_Slurm_options     = ""
mpi_CCC_options       = ""
mpi_options           = mpi_Slurm_options
home_path             = "/scratch/G95757"
server_path           = home_path+"/Melissa/build/server"
sys.path.append(home_path+"/../master")
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
pyzmq                 = 1


#==================================#
#               main               #
#==================================#

if __name__ == '__main__':
    launch_study(
    batch_scheduler,
    nb_parameters,
    nb_simu,
    nb_groups,
    nb_time_steps,
    operations,
    threshold,
    mpi_options,
    home_path,
    server_path,
    workdir,
    saturne_path,
    range_min,
    range_max,
    nodes_saturne,
    proc_per_node_saturne,
    openmp_threads,
    nodes_melissa,
    walltime_saturne,
    walltime_container,
    walltime_melissa,
    frontend,
    coupling,
    xml_file_name,
    username,
    pyzmq
    )


