import numpy as np

class options():
    def __init__(self):
        self.batch_scheduler       = "Slurm"
        self.nb_parameters         = 6
        self.sampling_size         = 1000
        self.nb_time_steps         = 100
        self.operations            = ["mean","variance","min","max","threshold","sobol"]
        self.threshold             = 0.7
        self.mpi_OAR_options       = "--mca orte_rsh_agent \"oarsh\" --mca btl openib,sm,self --mca pml ^cm --machinefile $OAR_NODE_FILE"
        self.mpi_Slurm_options     = ""
        self.mpi_CCC_options       = ""
        self.mpi_options           = self.mpi_Slurm_options
        self.home_path             = "/scratch/G95757"
        self.server_path           = self.home_path+"/Melissa/build/server"
        self.workdir               = "/scratch/G95757/etude_eole"
        self.saturne_path          = self.home_path+"/Code_Saturne/4.3/arch/eole/ompi/bin"
        self.range_min             = np.zeros(self.nb_parameters,float)
        self.range_max             = np.ones(self.nb_parameters,float)
        self.range_min[0:2]        = 0.1
        self.range_max[0:2]        = 0.9
        self.range_min[2:4]        = 0.1
        self.range_max[2:4]        = 0.9
        self.range_min[4:6]        = 0.88
        self.range_max[4:6]        = 1.2
        self.nodes_saturne         = 4
        self.proc_per_node_saturne = 14
        self.openmp_threads        = 2
        self.nodes_melissa         = 3
        self.walltime_saturne      = "1:20:0"
        self.walltime_container    = "1:20:0"
        self.walltime_melissa      = "240:00:0"
        self.frontend              = "eofront2"
        self.coupling              = 1
        self.xml_file_name         = "bundle_3x2_f16_param.xml"
        self.username              = "terrazth"
        self.pyzmq                 = 1
