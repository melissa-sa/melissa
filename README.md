# Table of content
 * [Overview](# Overview)
 * [Getting Started](# Getting Started)
 * [Running Melissa](# Getting Started)
    * [Local Execution](## Local Execution)
    * [Virtual Cluster](## Virtual Cluster)
    * [Real Cluster](## Real Cluster)
 * [How to cite Melissa](# How to cite Melissa)
 * [Pulbications](# Publications)


# Overview 

Melissa is a file avoiding, adaptive, fault tolerant and elastic framework, to run large scale sensitivity analysis  on  supercomputers.  Largest runs so far involved up to 30k core, executed 80000 parallel simulations,  and  generated 288 TB of intermediate data that did not need to be stored on the file system. 

Classical sensitivity analysis consists in running different  instances of the simulation with different set of input parameters, store the results to disk  to later read them back from disk to compute the required statistics. The amount of storage needed can quickly become overwhelming, with the associated long read time that makes statistic computing time consuming. To avoid this pitfall, scientists reduce their study size by running low resolution simulations or down-sampling output data in space and time. 

Melissa bypasses this limitation by avoiding intermediate file storage. Melissa   processes the data  in transit   enabling very large scale sensitivity analysis.  Melissa is built around two key concepts: iterative statistics algorithms and asynchronous client/server model for data transfer. Simulation outputs are never stored on disc. They are sent by the simulations to a parallel server, which aggregate them to the statistic fields in an iterative fashion, and then throw them away. This allows to compute oblivious statistics maps on every mesh element for every timestep on a full scale study. 


Melissa  comes with iterative algorithms for computing the average, variance and co-variance, skewness, kurtosis, max, min, threshold exceedance, quantiles and Sobol' indices, and can easily be extended with new algorithms. 


## Melissa Workflow

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

The examples are built if CMake finds a Fortran compiler and if you enabled the BUILD_EXAMPLES option. The examples are installed in install/share/melissa/examples. We use a heat equation solver example to test the installation.
To compile the solver from the install/share/melissa/examples/heat_example/solver directory, run:

```bash
    cd ../install/share/melissa/examples/heat_example/
    mkdir build
    cd build
    cmake ../solver
    make
    make install
    cd ..
```    
To start  the study (from heat_example dir): 

```bash
    melissa_launcher  -o ./study_local/options.py
```    
The results of this sensitivity analysis are stored in:
```bash
   ./study_local/STATS
```   

# Running Melissa
## Local Execution
## Virtual Cluster
## Real Cluster

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


# Pulbications
   * Melissa: Large Scale In Transit Sensitivity Analysis Avoiding Intermediate Files. Théophile Terraz, Alejandro Ribes, Yvan Fournier, Bertrand Iooss, Bruno Raffin. The International Conference for High Performance Computing, Networking, Storage and Analysis (Supercomputing), Nov 2017, Denver, United States. pp.1 - 14. [PDF](https://hal.inria.fr/hal-01607479/file/main-Sobol-SC-2017-HALVERSION.pdf)
  
   





