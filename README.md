# Melissa Sensitivity Analysis

## Table of Contents

* [About](#about)
* [News](#news)
* [Getting Started](#getting-started)
* [Solver Instrumenting and Study Setup](examples/heat-pde/README.md)
* Developer Documentation
* [Server](server/README.md)
* [Launcher](launcher/README.md)
* [Solver API](api/README.md)
* [Common](api/README.md)
* [How to cite Melissa](#how-to-cite-melissa)
* [Publications](#publications)
* [Contacts](#contacts)
* [Licence](#licence)



## About

Melissa is a scientific computing software for global sensitivity analysis. Melissa computes statistics on-the-fly while simulations are running including basics such as the minimum, the maximum, and the (arithmetic) mean as well as higher-order statistics, quantiles, and Sobol' indices. The advantages of Melissa are avoidance of temporary files, fault tolerance, and elastic use of available computational resources.

The goal of a _sensitivity analysis_ (SA) is to determine which input variables of a numerical model affect given output variables and how much changes in the input have an effect on the output (that is, how _sensitive_ the output is to a certain input). An SA helps to understand the relationships between input and output variables, it highlights outputs that may respond strongly to small changes in inputs, and it can help to simplify models by omitting uninfluential variables. For example, in computer-aided engineering an SA could identify product designs that are strongly affected by small manufacturing deviations. SA is related to (but distinct from) uncertainty analyses which attempts to describe the impact of input changes of a model on the outputs.

Sensitivity analysis is based on a statistical approach and works the better the more data is available. The classical approach for SA in scientific computing is to derive possibly interesting ranges for each input parameter, to run numerical simulations for each possible combination of input parameters (grid search), and to evaluate the collected model outputs with statistical software.

This approach has several problems:
* It is necessary to guess in advance which input parameters may be influential but this is exactly what the SA is supposed to find out.
* If the grid search is run with to few simulations, the analysis may miss inputs where numerical models responds strongly to small input changes.
* Finally, large-scale numerical simulations may fully exploit the resources provided by the world's most powerful computer cluster. Storing thousands of model outputs for statistical simulation may not be (physically) possible.

Melissa solves the problems with its iterative statistics computations.

Melissa consists of three components: Simulations, servers, and the launcher. The simulations are started by the launcher and run numerical models with input value provided by the launcher. The simulations send model outputs to the server which iteratively computes model outputs. The server makes requests to the launcher to start more simulations. The launcher manages all started simulations and servers. Upon request, it starts new simulations and generates input parameters for them.

To run a sensitivity analysis with Melissa, a user needs to
* update the simulation with three function calls to the Melissa API (initialization, sending data, deinitialization),
* create a configuration file for Melissa, and
* start the Melissa launcher.


## News
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


## Getting Started

### Install MELISSA


#### Code Download

Download Melissa sources [here](https://github.com/melissa-sa/melissa).

#### Dependencies

* CMake 3.7.2 or newer
* GNU Make
* A C99 compiler
* A Fortran90 compiler
* MPI
* ZeroMQ 4.1.5 or newer
* Python 3.5.3 or newer
* NumPy (for Python3)

CMake can download and install ZeroMQ if the flag `-DINSTALL_ZMQ=ON` is passed to CMake.

If you are unsure if all dependencies are installed, simply run CMake because it will find all required software packages automatically and check their version numbers or print error messages otherwise.


#### Compilation and Installation

Create a build directory and change directories:
```sh
mkdir build
cd build
```
Call CMake and customize the build by passing build options on its command line (see the table below). The build here has a compiler optimizations enabled:
```sh
cmake -DCMAKE_BUILD_TYPE=Release -- ../
make
make test
make install
```
Update environment variables to ensure the Melissa launcher and server can be found by the shell:
```sh
source ../install/bin/melissa-setup-env.sh
```
This command needs to be executed whenever you start a new shell.


#### Build Options

| CMake option | Default value | Description |
| -- | -- | -- |
| `-DCMAKE_BUILD_TYPE` | `Debug` | Build type (try `Debug` or `Release`) |
| `-DCMAKE_INSTALL_PREFIX` | `../install` | Melissa installation directory |
| `-DINSTALL_ZMQ` | `OFF` | Download, build, and install ZeroMQ |
| `-DBUILD_DOCUMENTATION` | `OFF` | Build the documentation (requires Doxygen) |
| `-DBUILD_TESTING` | `ON` | Build tests; run with `make test` in build directory |


### Run a first example


Go to  the heat example [README.md](examples/heat-pde/README.md) to run your first sensitivity analysis with melissa


## Reporting Issues


## License

Melissa is open source under the [BSD 3-Clause License](LICENSE).


## How to cite Melissa

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


## Publications
   * Melissa: Large Scale In Transit Sensitivity Analysis Avoiding Intermediate Files. Théophile Terraz, Alejandro Ribes, Yvan Fournier, Bertrand Iooss, Bruno Raffin. The International Conference for High Performance Computing, Networking, Storage and Analysis (Supercomputing), Nov 2017, Denver, United States. pp.1 - 14. [PDF](https://hal.inria.fr/hal-01607479/file/main-Sobol-SC-2017-HALVERSION.pdf)


## Dependencies

Melissa would not exist without high-quality C compilers, Fortran compilers, Python interpreters, standard language libraries, build systems, development tools, text editors, command line tools, and Linux distributions. The Melissa developers want to thank all developers, maintainers, forum moderators and everybody else who helped to improve these pieces of software.

Melissa links against [ØMQ](https://zeromq.org/) (ZeroMQ) and because Melissa may be _distributing in binary form_ (when static linking is enabled), we are obliged to mention that ZeroMQ is available under the terms of the [GNU Lesser General Public License version 3 with static linking exception](http://wiki.zeromq.org/area:licensing).

Copies of the licenses can be found in the folder [`licenses`](licenses).





## Development Hints

### C and C++

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


### MPI

MPI code may lead to false positives when checking for leaks with Valgrind or the address sanitizer. The address sanitizer can be instructed not to check for memory leaks on exit (update the environment variable `ASAN_OPTIONS='leak_check_on_exit=0'`) and the Valgrind manual contains instructions for MPI applications (see [§4.9 _Debugging MPI Parallel Programs with Valgrind_](https://www.valgrind.org/docs/manual/mc-manual.html#mc-manual.mpiwrap).

Open MPI is known to leak (usually) small amounts of statically allocated memory. For this reason recent Open MPI releases ship with a Valgrind suppression file, see the Open MPI FAQ [13. _Is Open MPI 'Valgrind-clean' or how can I identify real errors?_](https://www-lb.open-mpi.org/faq/?category=debugging#valgrind_clean)


### ZeroMQ

Building ZeroMQ causes linker errors when the GNU ld options `-z defs` is used.
