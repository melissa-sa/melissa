# Table of content
 * [News](#news)
 * [Getting Started](#getting-started)
 * [Running Melissa](#running-melissa)
    * [Local Execution](#local-execution)
    * [Virtual Cluster](#virtual-cluster)
    * [Supercomputer](#supercomputer)
 * [Utility](utility/README.md)
 * [Solver Instrumenting and Study Setup](examples/heat_example/README.md)
 * Developer Documentation
    * [Server](server/README.md)
    * [Launcher](launcher/README.md)
    * [Solver API](api/README.md)
    * [Common](api/README.md)
 * [How to cite Melissa](#how-to-cite-melissa)
 * [Publications](#publications)
 * [Contacts](#contacts)
 * [Licence](#licence)



# News
 * **Jan 2020: GitHub continuous update**
    * Sync our work repo with the github repo so all changes are immediatly visible to all
    * Major code restructuration  and documentation update
    * New tools for supporting a virtual cluster mode  and using Jupyter notebook for controlling a Melissa run
 * **Nov 2018: Melissa release 0.5 available on GitHub**
    * Changes in the API: remove arguments from the melissa_send function
    * Add batch mode
    * Improve launcher fault tolerance
    * Improve the examples and the install tree
    * Many fixes
 * **Nov 2017: Melissa release 0.4 available on GitHub**
   * Improve quantiles and threshold exceedances
   * Add iterative skewness and kurtosis
   * Add a restart mechanism over results of previous study
   * Add FlowVR coupling mechanism for Sobol' groups
   * Add Telemac2D example, including FlowVR coupling mechanism
   * Many bug fixes


# Getting Started

## Get the software

Download Melissa sources [here](https://github.com/melissa-sa/melissa).


## Dependencies

* CMake
* C++ and Fortran Compiler
* MPI, OpenMP: for melissa parallel server. A sequential server will be built if not available
* ZeroMQ: version 4.1.5 or above. CMake can download and install it if option turned on (-DINSTALL_ZMQ=ON)
* Python 3
* PythonLibs, Numpy 

## CMake Options

Most usefull CMake options:
```cmake
-DCMAKE_INSTALL_PREFIX (default: '../install')    ->  Melissa install directory.
-DBUILD_WITH_MPI (default: ON)                    ->  Enable MPI.
-DBUILD_WITH_OpenMP (default: OFF)                ->  Enable OpenMP for Melissa Server.
-DINSTALL_ZMQ (default: OFF)                      ->  Allows CMake to install ZeroMQ.
-DBUILD_DOCUMENTATION (default: OFF)              ->  If Doxygen is found, build the Doxygen documentation.
-DBUILD_TESTING (default: ON)                     ->  Build Melissa tests. They can be run with "make test" or "ctest".
```

## Compile and Install

Compilation, installation and environnent variable setting from the Melissa root directory:

```bash
    mkdir build
    cd build
    cmake ../source -DINSTALL_ZMQ=OFF
    make
    make install
    source ../install/bin/melissa_set_env.sh
```

## Testing

The examples are built if CMake finds a Fortran compiler and if you enabled the BUILD_EXAMPLES option. The examples are installed in `install/share/melissa/examples`. We use a heat equation solver example to test the installation.
To compile the solver from the `install/share/melissa/examples/heat_example/solver` directory, run:

```bash
    cd ../install/share/melissa/examples/heat_example/
    mkdir build
    cd build
    cmake ../solver
    make
    make install
    cd ..
```    
To start  the study (from `heat_example` dir): 

```bash
    melissa_launcher  -o ./study_local/options.py
```    
The results of this sensitivity analysis are stored in:
```bash
   ./study_local/STATS
```   

# Running Melissa
## Local Execution

Running Melissa localy means that all processes run on your local machine, executed directly by the launcher without goign through a  batch scheduler. This is a mode usefull for initial testing and debugging. The `heat_example` test run from the [Getting Started](## Testing) section is a local execution. 
    
    
## Virtual Cluster

The virtual cluster enables to run Melissa on a local machine with a batch scheduler managing a virtual cluster build using docker containers.
All instructions in the [utility/virtual_cluster/Readme.md](utility/virtual_cluster/Readme.md).

## Supercomputer 

 Running Melissa on a real supercomputer will  need some adaptations to the specificity of the target machine (batch scheduler, launcher that can
 be executed or not on the front nodes, etc.).

 Here are a few hints. To run Melissa on a cluster you need to adapt the code to your batch scheduling systems and system environment. Before you try to run anything on remote cluster, please familiarize yourself with batch scheduling system of your cluster and options file (short but precise enough description is in the `Creating your own study` chapter).

  1. In your `study_Slurm/scripts` folder, change the shell files so they are valid with your scheduling system. Pay attention to the commands and partition names.
  2. In the same shell files add/load appropriate modules for MPI.
  3. Check options file. Look especially at `launch_server` and `launch_group` and check if commands sending jobs to batch scheduler are valid.
  4. Check the shebang at `melissa_launcher`. This python interpreter have to have Numpy. If you know that some module have python with numpy, you can

```bash
module load *python module with numpy*
which python3
```

and copy the path to the shebang.



# How to cite Melissa

Melissa: Large Scale In Transit Sensitivity Analysis Avoiding Intermediate Files. Théophile Terraz, Alejandro Ribes, Yvan Fournier, Bertrand Iooss, Bruno Raffin. The International Conference for High Performance Computing, Networking, Storage and Analysis (Supercomputing), Nov 2017, Denver, United States. pp.1 - 14.
    

```
inproceedings{terraz:hal-01607479,
  TITLE = {{Melissa: Large Scale In Transit Sensitivity Analysis Avoiding Intermediate Files}},
  AUTHOR = {Terraz, Th{\'e}ophile and Ribes, Alejandro and Fournier, Yvan and Iooss, Bertrand and Raffin, Bruno},
  URL = {https://hal.inria.fr/hal-01607479},
  BOOKTITLE = {{The International Conference for High Performance Computing, Networking, Storage and Analysis (Supercomputing)}},
  ADDRESS = {Denver, United States},
  PAGES = {1 - 14},
  YEAR = {2017},
  MONTH = Nov,
  KEYWORDS = {Sensitivity Analysis ; Multi-run Simulations ; Ensemble Simulation ; Sobol' Index ; In Transit Processing},
  PDF = {https://hal.inria.fr/hal-01607479/file/main-Sobol-SC-2017-HALVERSION.pdf},
  HAL_ID = {hal-01607479},
  HAL_VERSION = {v1},
```


# Publications
   * Melissa: Large Scale In Transit Sensitivity Analysis Avoiding Intermediate Files. Théophile Terraz, Alejandro Ribes, Yvan Fournier, Bertrand Iooss, Bruno Raffin. The International Conference for High Performance Computing, Networking, Storage and Analysis (Supercomputing), Nov 2017, Denver, United States. pp.1 - 14. [PDF](https://hal.inria.fr/hal-01607479/file/main-Sobol-SC-2017-HALVERSION.pdf)
  

# Contacts

 * Theophile TERRAZ - theophile.terraz@inria.fr
 * Bruno RAFFIN - bruno.raffin@inria.fr
 * Alejandro RIBES CORTES - alejandro.ribes@edf.fr
 * Bertrand IOOSS - bertrand.iooss@edf.fr
 * Sebastian FRIEDEMANN - sebastian.friedemann@inria.fr
 

# Licence 
     Melissa is open source under the [BSD 3-Clause License](LICENSE)


