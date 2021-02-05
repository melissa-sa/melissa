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

Melissa is a scientific computing software for global sensitivity analysis.

Abstract : Global sensitivity analysis is an important step for analyzing and validating numerical simulations. One classical approach consists in computing statistics on the outputs from well-chosen multiple simulation runs. Simulation results are stored to disk and statistics are computed postmortem. Even if supercomputers enable to run large studies, scientists are constrained to run low resolution simulations with a limited number of probes to keep the amount of intermediate storage manageable. In this paper we propose a file avoiding, adaptive, fault tolerant and elastic framework that enables high resolution global sensitivity analysis at large scale. Our approach combines iterative statistics and in transit processing to compute Sobol' indices without any intermediate storage. Statistics are updated on-the-fly as soon as the in transit parallel server receives results from one of the running simulations. For one experiment, we computed the Sobol' indices on 10M hexahedra and 100 timesteps, running 8000 parallel simulations executed in 1h27 on up to 28672 cores, avoiding 48TB of file storage.


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
