# Overview

  Here we this very simple example (parallelized with MPI) to take you through the different steps to  run a Melissa sensitivity analysis  and instrument the   solver:
1. [Run the Example](#run-the-example)
2. [Instrument the Solver](#instrument-the-solver)

## Run the Example

### Shell Setup

Open a shell, locate your Melissa installation path (let call it `MELISSA_INSTALL_PREFIX`) and execute:

```sh
source MELISSA_INSTALL_PREFIX/install/bin/melissa-setup-env.sh
```
to setup all necessary environement variables


### Example Build

Next, build the example code:

```sh
mkdir workspace.heat-pde
cd workspace.heat-pde
cmake \
    -DCMAKE_PREFIX_PATH="" \
    "MELISSA_INSTALL_PREFIX/share/melissa/examples/heat-pde"
make
```


###  Example Execution

Depending on your local configuration different options are available to execute the example:

1. Direct execution using  MPI (here we explicitly only support `openmpi` as the MPI launcher differ between implementations).  This is the method to use for
a local execution or a cluster execution that does not require you tu go through abatch scheduler reservation. The `oversubscribe`option is to enable running several MPI processes per core. To be removed if you are looking for performance (but make sure you have enought core available)

```sh
melissa-launcher --scheduler-arg='--oversubscribe' openmpi options.py heatc &
```

2. [Slurm](https://slurm.schedmd.com/)  driven  execution. Slurm is batch scheduler you will find on a majority of supercomputers. Add any specific Slurm option using the
`--scheduler-arg=` (for example to define the account to use:  `--scheduler-arg='--account=igf@cpu'`):
```sh
melissa-launcher slurm  options.py heatc &
```


3. [OAR](https://oar.imag.fr/) driven  execution. OAR is lightweight and flexible batch scheduler gaining popularity. Add any specific OAR option using the
`--scheduler-arg=` (for example to define the account to use:  `--scheduler-arg='-t devel --project pr-avid`):
```sh
melissa-launcher  oar  options.py heatc &
```


### Results

Results, log files and the `option.py` files will be located in a timestamped dedicated  directory `YYYY-MM-DD` unless the default behavior is overwritten through some options. To get the different available options:

```sh
melissa-launcher --help
```

Melissa produces one file per time step and computed statistics. For example  the file `results.heat1_mean.001` contains the mean value over all executed simulation of the temperature for each grid cell at time step `1`.

### Execution Customization: `options.py`

The [options.py](options.py) file is the main file for controlling Melissa execution, parameter sweep, computed statistics computed, etc.


## Instrument the Solver

### Introduction

The principle of a global sensitivity analysis is to run a large number of simulations with varying input parameter sets, and measure the influence of the parameters on the solver output. The key to enable large scale sensitivity analysis is to process the data in-situ and iteratively.
On a cluster, all the simulations from a ensemble study run on their own independent jobs. These simulations can run in any order, asynchronously: they are all independents. In an in-situ sensitivity analysis, the main difference is during the I/O phase. the simulations must send their data to an external server instead of writing them on the filesystem. This server will then use the data to update a set of ubiquitous statistics in an iterative fashion, and discard the data. This server must run during the entire simulation campaign, on its own job. To be able to handle such a study, one need a tool to manage the whole process of generating the study, running the simulations, and controlling the progress of the campaign.

Melissa propose a three tier architecture, and is based on this client/server model.

* **Melissa Server**: an independent parallel executable. It receives data from the simulations, updates iterative  statistics as soon as possible, then discard the processed data.
* **Melissa API**: a shared library to be linked within the numerical simulation solver. It forwards simulation data to Melissa Server.
* **Melissa Launcher**: A manager in charge of generating and overseeing the whole global sensitivity study.

Melissa can be used within a C or Fortran code. In this tutorial, we will focus on C with MPI. Basic knowledge of C, Python and MPI is expected, as well as the usage of a cluster with a batch scheduler.

We will learn below how to use Melissa, from the instrumentation of the numerical solver to the management of a sensitivity analysis.
To illustrate this documentation, we provide a simple example: a global sensitivity analysis with Melissa and a simple heat equation solver. To follow this example step-by-step alongside the documentation, unfold the example bloc after each paragraph.

<details>
<summary><em> Click here to fold/unfold example </em></summary>

***
In these foldable paragraphs, we will go through the process of instrumenting a heat equation solver to run a global sensitivity analysis. This solver is available in the "examples" folder of Melissa installation in `share/melissa/examples/heat_example/solver`. The solver comes in two flavors: C+MPI, Fortran+MPI in `solver/src/heat.c`, `solver/src/heat.f90`. In this tutorial, we will focus on the C+MPI example, but they are all equivalent. If you did the "testing" part of the Getting Started, you already ran it on your machine.
***

</details>

### Melissa API

Before anything else, we have to plug Melissa in our solver.
Melissa API is a shared library composed of three functions, to be integrated in the solver. They are the link between the simulations (clients) and the server. The three functions are as simple as:

* `melissa_init`
* `melissa_send`
* `melissa_finalize`

In order to use these functions, you have to link `melissa_api.so` and include `melissa_api.h` in your solver.

```c
#include <melissa_api.h>
```

We will now explain how to use each one of these functions.

#### melissa_init

This function must be called once for every scalar field to send to Melissa Server. The field names have to be declared in the launcher option file, as we will see later. A scalar field is an array of doubles. It is the simulation result at a given timestep without any other metadata (in particular, without mesh).

Prototype:

```c
void melissa_init(const char *field_name,
                  const int  *local_vect_size,
                  MPI_Comm   *comm);
```


variables:

* `field_name`: a unique name to identify the field
* `vect_size`: the size of the local result vector (in number of elements)
* `comm`: the local MPI communicator

#### `melissa_send`

This function must be called at each time step that needs to be sent to Melissa Server, for each field. It can replace the I/O phase of the code. The field name is used by Melissa to identify the field, and must be declared in the launcher option file. If a field name not declared in the launcher is passed, Melissa will ignore the field. Melissa guaranties to keep the order of the cells (the array of double) and the order of the calls (in the form of timestamps), and it is up to the user to map them to the mesh and to the timesteps afterwards.

Prototype:

```c
void melissa_send(const char *field_name,
                  double     *send_vect);
```


variables:

* `field_name`: the name of the field sent
* `send_vect`: the vector to send to Melissa Server

#### `melissa_finalize`

This function terminates the Melissa environment.
It must be called only once, at the end of the solver, before `MPI_Finalize()`.

Prototype:

```c
void melissa_finalize();
```

<details>
<summary><em> Example </em></summary>

***

Open the file `solver/src/heat.c` in your favorite editor.
To use the Melissa functions in the code, we first have to include `melissa_api.h` at the beginning of the code.

```c
    #include <melissa_api.h>
```

The solver can take from one to five input parameters, stored in five doubles. The code to get the input parameters looks like this:


```c
    double param[5];

    if (argc < 2)
    {
        fprintf (stderr, "Missing parameter");
        return -1;
    }

    for (n=0; n<5; n++)
    {
        param[n] = 0;
        if (argc > n+1)
        {
           param[n] = strtod(argv[n+1], NULL);
        }
    }
```


In general, if all the simulations in a Sobol' group can easily be launched in a single MPMD MPI call, we can use the `MELISSA_COUPLING_MPI` coupling mechanism (see Launcher section). Links between simulations will be MPI communications in that case. Otherwise, if the simulation relies on `MPI_COMM_WORL`D for MPI routines or is not MPI at all, simulations have to be connected via ZeroMQ. This is the default coupling mechanism, called `MELISSA_COUPLING_ZMQ`. The coupling mechanisms are transparent to the user.
In the heat solver, we can easily split MPI communicator, so we will use `MELISSA_COUPLING_MPI`. We split `MPI_COMM_WORLD` by simulation in the simulation group:


```c
    int *appnum, info;
    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_APPNUM, &appnum, &info);
    MPI_Comm_split(MPI_COMM_WORLD, *appnum, me, &comm);
```

We also need to give a "name" to the computed field. It must be the same name you will put in `STUDY_OPTIONS['field_names']`.


```c
    char *field_name = "heat1";
```

The first Melissa function can be called before the main "for" loop. It has to be called exactly once by process and by field. It takes 3 arguments:

* `field_name`: the unique name of the field to send
* `vect_size`: the size of the local result vector
* `comm`: the local MPI communicator


```c
    melissa_init(field_name, vect_size, comm);
```

Inside the main loop, the result vector is updated by the conjgrad function. This is at this moment that we want to send the updated vector to Melissa Server. We call `melissa_send` right after the `conjgrad` function:

```c
    conjgrad (&a[0], &f[0], &u[0], &nx, &ny, &epsilon, &i1, &in, &np,
              &me, &next, &previous, &fcomm);
    melissa_send (field_name, u);
```

This function takes two arguments:

* `field_name`: the name of the field  to send to Melissa Server
* `u`: the vector to send to Melissa Server

After the main loop, we call `melissa_finalize` to free Melissa structures and disconnect the simulations from the server. This function does not take any argument.

```c
    melissa_finalize();
```

We are done with the solver instrumentation! In the solver folder, you will find a `CMakeFile.txt` to compile the solver. Simply do:

```bash
    cmake .
    make install
```

This `CMakeFile.txt` can be a basis for your own application.
***

</details>

## Melissa `options.py`

Once the solver  is instrumented, it can be managed by Melissa.
Melissa Launcher can supervise the whole sensitivity analysis, as long as it knows how to interact with the machine and the solver. These informations are very specific to each environment and can be customized through the `options.py`file.


Important note: Sobol' indices iterative computation requires a special way of managing the simulations. For more details about the algorithm, refer to this [article](https://hal.archives-ouvertes.fr/hal-01607479). Basically, simulations have to run in groups of size nb_parameters+2. This does not impact the computation of the other statistics. From now, keep in mind that the computation of Sobol' indices implies some specific customizations, and we will now distinguish this case from the classical case.

The launcher sees simulations as "groups". In the case of classical statistics (i.e. not Sobol' indices), one group corresponds to a batch of simulations. When Sobol' indices are requested, simulations have to be launched by groups of nb_parameters+2 coupled simulations, in the same job allocation.

