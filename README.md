# Table of Contents
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
 * **Jan 2020: GitHub continuous update**
    * Sync our work repo with the github repo so all changes are immediatly visible to all
    * Major code restructuring  and documentation update
    * New tools for supporting a virtual cluster mode  and using Jupyter notebook for controlling a Melissa run
 * **Nov 2018: Melissa release 0.5 available on GitHub**
    * Changes in the API: remove arguments from the `melissa_send` function
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

* CMake 3.2 or newer
* C, C++, and Fortran90 compilers
* MPI
* ZeroMQ 4.1.5 or newer
* OpenMP (for a parallel Melissa server, optional)
* Python 3.5.3 or newer
* Python libraries: NumPy, JupyterHub, Traitlets, async\_generator, Jinja, asyncio, Tornado

CMake can download and install ZeroMQ if the flag `-DINSTALL_ZMQ=ON` is passed to CMake.


## CMake Options

Most useful CMake options:
```cmake
-DCMAKE_INSTALL_PREFIX (default: '../install')    ->  Melissa install directory.
-DBUILD_WITH_OpenMP (default: OFF)                ->  Enable OpenMP for Melissa Server.
-DINSTALL_ZMQ (default: OFF)                      ->  Allows CMake to download, build, and install ZeroMQ.
-DBUILD_DOCUMENTATION (default: OFF)              ->  If Doxygen is found, build the Doxygen documentation.
-DBUILD_TESTING (default: ON)                     ->  Build Melissa tests. They can be run with "make test" or "ctest".
```

## Compile and Install

Compilation, installation and environnent variable setting from the Melissa root directory:

```bash
    mkdir build
    cd build
    cmake ..
    make
    make install
    source ../install/bin/melissa_set_env.sh
```

## Testing

The examples are built if CMake finds a Fortran compiler and if you enabled the `BUILD_EXAMPLES` option. The examples are installed in `install/share/melissa/examples`. We use a heat equation solver example to test the installation.
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

Running Melissa locally means that all processes run on your local machine, executed directly by the launcher without going through a  batch scheduler. This is a mode useful for initial testing and debugging. The `heat_example` test run from the [Getting Started](## Testing) section is a local execution.


## Virtual Cluster

The virtual cluster enables to run Melissa on a local machine with a batch scheduler managing a virtual cluster build using docker containers.
All instructions in the [utility/virtual_cluster/README.md](utility/virtual_cluster/README.md).


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
 * Christoph CONRADS - christoph.conrads@inria.fr


# Licence
     Melissa is open source under the [BSD 3-Clause License](LICENSE)


# Dependencies

Melissa would not exist without high-quality C compilers, Fortran compilers, Python interpreters, standard language libraries, build systems, development tools, text editors, command line tools, and Linux distributions. The Melissa developers want to thank all developers, maintainers, forum moderators and everybody else who helped to improve these pieces of software.

Melissa links against [ØMQ](https://zeromq.org/) (ZeroMQ) and because Melissa may be _distributing in binary form_ (when static linking is enabled), we are obliged to mention that ZeroMQ is available under the terms of the [GNU Lesser General Public License version 3 with static linking exception](http://wiki.zeromq.org/area:licensing).

Copies of the licenses can be found in the folder [`licenses`](licenses).





# Development Hints

## C and C++

C and C++ are easily susceptible to memory bugs and undefined behavior. For example, the following C99 code shows undefined behavior because the literal `1` is taken to be a _signed_ integer by compiler:
```c
#include <stdint.h>
uint32_t u = 1 << 31;
```
This is the corrected code:
```c
#include <stdint.h>
uint32_t u = UINT32_C(1) << 31;
```

Many of these errors can be detected at compile-time if warnings are enabled and at run-time with the aid of the [_address sanitizer_](https://github.com/google/sanitizers/wiki/AddressSanitizer) (ASAN) and the [_undefined behavior sanitizer_](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html) (UBSAN). Both sanitizers are supported by GCC and Clang.

Most warnings are already enabled in `CMakeLists.txt`.

To enable the sanitiers, pass `-fsanitize=address` for ASAN and `-fsanitize=undefined` for UBSAN on the compiler command line. When using CMake, export the following environment flags before calling CMake:
```sh
export CFLAGS='-fsanitize=address -fsanitize=undefined'
export CXXFLAGS='-fsanitize=address -fsanitize=undefined'
```
Afterwards build the code as usual with `make` and `make install`. If an error is detected, the sanitizers print file and line data if debugging information is present in the executable files. This can be done either by adding the compiler flag `-g` to the compiler options or by settings `-DCMAKE_BUILD_TYPE=Debug` on the CMake command line.

As of August 2020, some Melissa tests seems to be leaking memory. To ignore memory leaks (and have ASAN only check for memory corruption), set the following environment flag before running the tests:
```sh
env ASAN_OPTIONS='leak_check_at_exit=0' ctest
```
A list of ASAN and UBSAN options is available at the linked websites.

Additionally, the standard memory allocator on Linux can be instructed to perform extra consistency checks by setting some environment flags:
```sh
export MALLOC_CHECK_=3
export MALLOC_PERTURB_=1
```
This approach works with _any_ application and without _any_ code modification.


## MPI

MPI code may lead to false positives when checking for leaks with Valgrind or the address sanitizer. The address sanitizer can be instructed not to check for memory leaks on exit (update the environment variable `ASAN_OPTIONS='leak_check_on_exit=0'`) and the Valgrind manual contains instructions for MPI applications (see [§4.9 _Debugging MPI Parallel Programs with Valgrind_](https://www.valgrind.org/docs/manual/mc-manual.html#mc-manual.mpiwrap).

Open MPI is known to leak (usually) small amounts of statically allocated memory. For this reason recent Open MPI releases ship with a Valgrind suppression file, see the Open MPI FAQ [13. _Is Open MPI 'Valgrind-clean' or how can I identify real errors?_](https://www-lb.open-mpi.org/faq/?category=debugging#valgrind_clean)


## ZeroMQ

Building ZeroMQ causes linker errors when the GNU ld options `-z defs` is used.
