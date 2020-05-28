# Table of content
 * [Overview](#overview)
 * [Getting Started](#getting-started)
 * [Running Melissa](#getting-started)
    * [Local Execution](#local-execution)
    * [Virtual Cluster](#virtual-cluster)
    * [Real Cluster](#real-cluster)
 * [How to cite Melissa](#how-to-cite-melissa)
 * [Pulbications](#publications)


# Overview 

Melissa is a file avoiding, adaptive, fault tolerant and elastic framework, to run large scale sensitivity analysis  on  supercomputers.  Largest runs so far involved up to 30k core, executed 80000 parallel simulations,  and  generated 288 TB of intermediate data that did not need to be stored on the file system. 

Classical sensitivity analysis consists in running different  instances of the simulation with different set of input parameters, store the results to disk  to later read them back from disk to compute the required statistics. The amount of storage needed can quickly become overwhelming, with the associated long read time that makes statistic computing time consuming. To avoid this pitfall, scientists reduce their study size by running low resolution simulations or down-sampling output data in space and time. 

Melissa bypasses this limitation by avoiding intermediate file storage. Melissa   processes the data  in transit   enabling very large scale sensitivity analysis.  Melissa is built around two key concepts: iterative statistics algorithms and asynchronous client/server model for data transfer. Simulation outputs are never stored on disc. They are sent by the simulations to a parallel server, which aggregate them to the statistic fields in an iterative fashion, and then throw them away. This allows to compute oblivious statistics maps on every mesh element for every timestep on a full scale study. 


Melissa  comes with iterative algorithms for computing the average, variance and co-variance, skewness, kurtosis, max, min, threshold exceedance, quantiles and Sobol' indices, and can easily be extended with new algorithms. 

Melissa relies on a client/server architecture, composed of three main modules:
* Melissa Server: an independent parallel executable. It receives data from the simulations, updates iterative statistics as soon as possible, then throw data away.
* Melissa API: a shared library to be linked within the simulation code. It forwards simulation data to Melissa Server. The simulations of the sensitivity analysis become the clients of Melissa Server.
* Melissa Launcher: A Python script in charge of generating and driving the whole global sensitivity analysis.



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

 * [News](#news)
 * [Getting Started](#getting-started)
 * [Running Melissa](#running-melissa)
    * [Local Execution](#local-execution)
    * [Virtual Cluster](#virtual-cluster)
    * [Supercomputer](#supercomputer)
       * [Configuring](#configuring)
       * [Schedulers](#schedulers)
       * [Options](#options)
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
 * **May 2020: melissa 0.6 release**
    * Changes in the API
    * New examples
    * new lightweight python server
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
* Python >= 3.5.3
* Python libraries: NumPy, JupyterHub, Traitlets, async_generator, Jinja, asyncio, Tornado

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
    cmake .. -DINSTALL_ZMQ=OFF
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
All instructions in the [utility/virtual_cluster/README.md](utility/virtual_cluster/README.md).

## Supercomputer

Mellisa will run on any cluster with one of the following job schedulers:
* Torque
* Moab
* PBS
* Slurm
* Grid Engine
* Condor
* LSF
* OAR is to be implemented soon

`job_scheduler_config.py` is a fully customisable module for managing your scheduler of choice. It is an interface to how Melissa interacts with job schedulers. It does not require programming skills and requires only basic understaing of job scheduler features.

### Configuring

Start by defining a job scheduler in the `spawner` field. Next, assign values to job scheduler options: `scheduler.req_...`, `scheduler.server_script`, `scheduler.simu_script`. Only job template scripts `scheduler.server_script` and `scheduler.simu_script` are required. Job template scripts must contain selected `req_...` options without `req_` prefix.

There is an example Slurm configuration in `job_scheduler_config.py`.

### Schedulers

Job scheduler should be defined as one of these:
```python
batchspawner.TorqueSpawner()
batchspawner.MoabSpawner()
batchspawner.PBSSpawner()
batchspawner.SlurmSpawner()
batchspawner.GridengineSpawner()
batchspawner.CondorSpawner()
batchspawner.LsfSpawner()
```

### Options

* `req_queue` - queue name to submit job to resource manager
* `req_host` - host name of batch server to submit job to resource manager
* `req_memory` - memory to request from resource manager
* `req_nodes` - number of nodes allocated to a job
* `req_tasks_per_node` - number of tasks invoked on each node
* `req_nprocs` - number of processors to request from resource manager
* `req_ngpus` - number of GPUs to request from resource manager
* `req_runtime` - length of time for submitted job to run
* `req_partition` - partition name to submit job to resource manager
* `req_account` - account name string to pass to the resource manager
* `req_server_output_log` - server batch script standard output file
* `req_server_error_log` - server batch script standard error file
* `req_simu_output_log` - simulation batch script standard output file
* `req_simu_error_log` - simulation batch script standard error file
* `req_username` - name of a user running a job
* `req_homedir` - home directory of a user running a job
* `req_prologue` - script to run before batch script commands are invoked
* `req_epilogue` - script to run after batch script commands are invoked
* `req_options` - other job scheduler options to include into job submission script


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


