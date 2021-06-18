# Melissa Heat PDE Example

The heat equation is a partial differential equation (PDE) often taught in introductory courses on differential equations. This document demonstrates a Melissa sensitivity analysis involving a parallel MPI simulation using the example of a heat equation solver.

* [Run the Example](#run-the-example)
* [Instrument the Solver](#instrument-the-solver)

Throughout this document, we assume that Melissa was already installed in `MELISSA_INSTALL_PREFIX`.


## Run the Example

Update the environment variables of your shell to ensure all Melissa executables are found:
```sh
source MELISSA_INSTALL_PREFIX/bin/melissa-setup-env.sh
```
Next, build the example code:
```sh
mkdir build.heat-pde
cd build.heat-pde
cmake "$MELISSA_INSTALL_PREFIX/share/melissa/examples/heat-pde"
make
```
If the build is successful, there should be multiple new files in the current working directory including an executable called `heatc`.


The options file `options.py` can be used to configure Melissa execution, parameter sweep, the computed statistics, and so on. Feel free to modify the copy in the current working directory.

The example can be started with one of several batch schedulers supported by Melissa: OpenMPI, [Slurm](https://slurm.schedmd.com/), or [OAR](https://oar.imag.fr/). It may be necessary to pass additional arguments directly to the batch scheduler for a successful example run. For example, starting with version 3, OpenMPI refuses to oversubscribe by default (in layman's terms, to start more processes than there are CPUs cores on a system). The heat PDE example is not a computational challenging problem but simply due to the number of simulation processes, the system may end up being oversubscribed.
```sh
melissa-launcher --scheduler-arg='--oversubscribe' openmpi options.py ./heatc
```
On clusters with Slurm or OAR as the job manager, an account or project, respectively, must be specified.
```sh
melissa-launcher --scheduler-arg='--account=abc@cpu' slurm options.py ./heatc
```
melissa-launcher --scheduler-arg='--project=pr-abc' oar options.py ./heatc
```
Pass only the long version of the argument (i.e., `--account=abc@cpu` instead of `-A abc@cpu`); the command line parser gets confused by the the short variant.

All results, log files, and a copy of the options file will be stored in a
dedicated directory called `melissa-YYYYMMDDTHHMMSS` by default, where
`YYYYMMDD` and `THHMMSS` are the current date and local time, respectively, in
ISO 8601 basic format. For each time step, for each field, and for each statistic, the Melissa server will generate one file containing the statistic value for every grid point. For example, the file `melissa-21091207T123456/results.temperature_mean.001` contains the mean values of the temperature for the first time step for the Melissa analysis started at December 7, 2109, 12:34:56pm local time.

The statistics can be turned into a small movie with the aid of the script `plot-results.py`. For example, the command below will create a movie from the variance of the temperature over all time steps:
```python
python3 "$MELISSA_INSTALL_PREFIX/share/melissa/examples/heat-pde/plot-results.py" melissa-21091207T123456/ temperature variance
```


## Instrumenting the Solver

To avoid intermediate file storage and the problems associated with it, the simulations must send their data directly to the Melissa server.

**Attention**: A _time step_ refers to the time steps of the sensitivity analysis; the Melissa time steps should not be mistaken with simulation time steps; they may not be identical if the simulation does not send data after every simulation step.

The Melissa client API provides the link between the simulations (clients) and the Melissa server. The API can be found in the header file `$MELISSA_INSTALL_PREFIX/include/melissa/api.h`, where `MELISSA_INSTALL_PREFIX` is the path to the root directory of the Melissa installation.
```c
#include <melissa/api.h>
```
The header file allows you to check at compile time the Melissa version and it contains declarations for all relevant functions.

Before calling any Melissa API functions, you need to decide on a set of fields or quantities that you want to have analyzed by Melissa; this information must be passed to the Melissa launcher in the options file in the dictionary entry `STUDY_OPTIONS["field_names"]` and it must match the API calls in the simulation code. In the heat example, there is only one field called `temperature`.

Next, MPI must be initialized and an MPI communicator for each individual simulation must be created:
```c
    MPI_Init(NULL, NULL);
    int* appnum = NULL;
    int info = -1;
    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_APPNUM, &appnum, &info);
    MPI_Comm comm_app = MPI_COMM_NULL;
    MPI_Comm_split(MPI_COMM_WORLD, *appnum, me, &comm_app);
```
The simulation can begin the communication with the Melissa server. The first step is to instruct the Melissa server of the field name and the number of degrees of freedom (the number of floating-point value):
```c
    const char field_name[] = "temperature";
    melissa_init(field_name, num_dofs, comm_app);
```
At this point, the simulation can begin sending data to the server with `melissa_send`: The first argument is the field name, the second argument is a reference to an array of values:
```
    double* u = calloc(sizeof(double), num_dofs);
    // ...
    melissa_send(field_name, u);
```
After sending the data for all fields, `melissa_finalize()` must be called to properly disconnect from the Melissa server and releases all resources.
```c
    melissa_finalize();
    MPI_Finalize();
```
This function must be called _before_ `MPI_Finalize()`.
