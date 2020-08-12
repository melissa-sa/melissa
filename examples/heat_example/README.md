# Overview

  Here we take you through the different steps to instrument a solver and run a Melissa sensitivity analysis with it. 
  It's a 4  step process: 
1. [Prepare the Study](#prepare-the-study)
2. [Instrument the solver](#instrument-the-solver)
3. [Configure Melissa launcher](#configure-melissa-launcher)
4. [Run the Study](#run-the-study)
 
 
# Prepare the Study

The options file is located in every example and it is mandatory to run Melissa. The file contains python code with 3 python dictionaries:

* `STUDY_OPTIONS` - sets the parameters of the sensitivity study
* `MELISSA_STATS` - boolean values to activate certain statistics in Melissa server
* `USER_FUNCTIONS` - pointers to user defined functions, which will be used to control the study

There are a few mandatory functions that need to be defined, the rest is optional:

* `USER_FUNCTIONS['draw_parameter_set']` - this function is used by Melissa launcher to draw the parameter sets of the simulations. It must return a Numpy array of floats of the size `STUDY_OPTIONS['nb_parameters']`
* `USER_FUNCTIONS['launch_server']` - function that will launch Melissa server. Takes `Server` object as an argument and needs to set server job ID in `server.job_id`. On cluster, job ID is given by batch scheduler and on local machine it can be a PID of the process. The server command line options are in `server.cmd_opt` and the path to the executable `melissa_server` is in `server.path`, you must use it and not modify it.
* `USER_FUNCTIONS['launch_group']` - this function is quite complex and is described below
* `USER_FUNCTIONS['check_server_job']` - function checks the server status. Takes `Job` object as an argument and needs to set `job.job_status` to 1 if the server is running or 2 if otherwise. 
* `USER_FUNCTIONS['check_group_job']` - function checks the simulation status. Takes `Job` object as an argument and needs to set `job.job_status` to 1 if the simulation is running or 2 if otherwise.
* `USER_FUNCTIONS['cancel_job']` - function that kills the job. It takes `Job` object for argument. The job ID is defined in `job.job_id`

Function `USER_FUNCTIONS['launch_group']` is quite complex because it depends on the type of study you want to do. It takes `Group` object for argument. It provides 4 important attributes:

* `rank`: the rank of the group in the study
* `simu_id`: the IDs of the simulations of the group in the study
* `param_set`: the parameter sets of the simulations of the group
* `job_id`: the ID of the group, used for fault-tolerance
  
There are 3 kinds of group but the most common is single simulation. Each simulation runs in its own job. `simu_id` and `rank` are equivalents and `param_set` is a numpy array of size `STUDY_OPTIONS['nb_parameters']`

The function needs to set the group job ID in the `job_id` attribute. On a cluster the job ID is given by the batch scheduler. In your local machine, you can use the PID. You have to copy the file called `server_name.txt` from the directory `STUDY_OPTIONS['working_directory']` to the location where the simulations will run. The simulations will read it to retrieve the server node name in order to contact it.

This is bare minimum you need to know about the options file to run the sensitivity study. Look at the options file in other example to get an overall understanding how to create such a file. To learn about 2 other groups or optional functions visit [user documentation on github](https://github.com/melissa-sa/melissa/wiki/4-User-Documentation)

# Instrument the Solver

## Introduction

The principle of a global sensitivity analysis is to run a large number of simulations with varying input parameter sets, and measure the influence of the parameters on the solver output. The key to enable large scale sensitivity analysis is to process the data in-situ and iteratively.
On a cluster, all the simulations from a ensemble study run on their own independent jobs. These simulations can run in any order, asynchronously: they are all independents. In an in-situ sensitivity analysis, the main difference is during the I/O phase. the simulations must send their data to an external server instead of writing them on the filesystem. This server will then use the data to update a set of ubiquitous statistics in an iterative fashion, and discard the data. This server must run during the entire simulation campaign, on its own job. To be able to handle such a study, one need a tool to manage the whole process of generating the study, running the simulations, and controlling the progress of the campaign.

Melissa propose a three tier architecture, and is based on this client/server model.

* **Melissa Server**: an independent parallel executable. It receives data from the simulations, updates iterative  statistics as soon as possible, then discard the processed data.
* **Melissa API**: a shared library to be linked within the numerical simulation solver. It forwards simulation data to Melissa Server.
* **Melissa Launcher**: A manager in charge of generating and overseeing the whole global sensitivity study.

Melissa can be used within a C or Fortran code, with or without MPI. In this tutorial, we will focus on C with MPI. Basic knowledge of C, Python and MPI is expected, as well as the usage of a cluster with a batch scheduler.

We will learn below how to use Melissa, from the instrumentation of the numerical solver to the management of a sensitivity analysis.
To illustrate this documentation, we provide a simple example: a global sensitivity analysis with Melissa and a simple heat equation solver. To follow this example step-by-step alongside the documentation, unfold the example bloc after each paragraph.

<details>
<summary><em> Click here to fold/unfold example </em></summary>

***
In these foldable paragraphs, we will go through the prosessus of instrumenting a heat equation solver to run a global sensitivity analysis. This solver is available in the "examples" folder of Melissa installation in `share/melissa/examples/heat_example/solver`. The solver comes in four flavours: c+MPI, Fortran+MPI, C without MPI and Fortran without MPI, respectively in `solver/src/heat.c`, `solver/src/heat.f90`, `solver/src/heat_no_mpi.c` and `solver/src/heat_no_mpi.f90`. In this tutorial, we will focus on the c+mpi example, but they are all equivalent. If you did the "testing" part of the Getting Started, you already ran it on your machine.
***

</details>

## Melissa API

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

### melissa_init

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

### `melissa_send`

This function must be called at each time step that needs to be sent to Melissa Server, for each field. It can replace the I/O phase of the code. The field name is used by Melissa to identify the field, and must be declared in the launcher option file. If a field name not declared in the launcher is passed, Melissa will ignore the field. Melissa guaranties to keep the order of the cells (the array of double) and the order of the calls (in the form of timestamps), and it is up to the user to map them to the mesh and to the timesteps afterwards.

Prototype:

```c
void melissa_send(const char *field_name,
                  double     *send_vect);
```
        

variables:

* `field_name`: the name of the field sent
* `send_vect`: the vector to send to Melissa Server

### `melissa_finalize`

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
        

In general, if all the simulations in a Sobol' group can easily be launched in a single MPMD MPI call, we can use the "MELISSA_COUPLING_MPI" coupling mechanism (see Launcher section). Links between simulations will be MPI communications in that case. Otherwise, if the simulation relies on MPI_COMM_WORLD for MPI routines or is not MPI at all, simulations have to be connected via ZeroMQ. This is the default coupling mechanism, called "MELISSA_COUPLING_ZMQ".
A third coupling mechanism is also available if you have the FlowVR software installed in your environment. It is called "MELISSA_COUPLING_FLOWVR", and we will not use it in this example.
All these coupling mechanisms are transparent to the user.
In the heat solver, we can easily split MPI communicator, so we will use "MELISSA_COUPLING_MPI". We split MPI_COMM_WORLD by simulation in the simulation group:


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

# Configure Melissa Launcher

Once the solver  is instrumented, it can be managed by Melissa.
Melissa Launcher can supervise the whole sensitivity analysis, as long as it knows how to interact with the machine and the solver. These informations are very specific to each environment.
That's why each user have to provide Melissa the tools to launch the simulations, to check their behavior, and other useful operations, described in more details later in this page.
This is done by giving the launcher some functions and variables through a python file. The file is usually called options.py. An empty template of this file is provided in `share/melissa/launcher/options.py`. This is not only a configuration file: it contains functions that will be loaded and executed by the launcher. The options supported by Melissa are predefined in this file. You must copy this file and modify it to meet your needs. There are three sets of variables to define, as dictionaries.

* STUDY_OPTIONS: sets the parameters of the sensitivity study. They will be used by the launcher to generate its internal structures for the study management.
* MELISSA_STATS: contains booleans used to activate (or not) the iterative statistics.
* USER_FUNCTIONS: contains pointers to user defined functions, used by the launcher. Some of them are optional, others are mandatory. We will describe these functions in this section.

Note: you can add fields to the option dictionaries, and they can be used in the user defined functions.

Melissa Launcher needs the mandatory functions to be defined. Otherwise, it will return without running the study. The optional functions are ignored if they are not defined by the user.

Important note: Sobol' indices iterative computation requires a special way of managing the simulations. For more details about the algorithm, refer to this [article](https://hal.archives-ouvertes.fr/hal-01607479). Basically, simulations have to run in groups of size nb_parameters+2. This does not impact the computation of the other statistics. From now, keep in mind that the computation of Sobol' indices implies some specific customizations, and we will now distinguish this case from the classical case.

The launcher sees simulations as "groups". In the case of classical statistics (i.e. not Sobol' indices), one group corresponds to a batch of simulations. When Sobol' indices are requested, simulations have to be launched by groups of nb_parameters+2 coupled simulations, in the same job allocation.

<details>
<summary><em> Example </em></summary>

***
To folow this section, open the `options.py` file available in `install/share/melissa/examples/heat_example/sludy_local`.
This file contains all the material needed by the launcher to handle the study on your local machine. We will see later an example for a cluster.

Start by the end of the file. Here, we can see tree different dictionaries. The first, `STUDY_OPTIONS`, contains mandatory informations about the study. The second, `MELISSA_STATS`, defines the operations to be performed by Melissa. The third, `USER_FUNCTIONS`, contains pointers to user defined functions. In this one, we have to define at least the mandatory user functions, as described below. In the example file, all the functions are defined above, but you can also define them in a separate file and include it in your `option.py`.
***

</details>

## Functions

All the user defined functions must be callable from the `option.py` file. The function pointers are stored in the `USER_FUNCTIONS` dictionary.

```python
USER_FUNCTIONS['create_study'] # (optional)
```

This function is called only once, before starting the study. It is called from the directory set in STUDY_OPTIONS['working_directory']

You can use this function to create a folder tree, copy files, compile code, etc.

```python
USER_FUNCTIONS['draw_parameter_set'] # (mandatory):
```

This function is used by Melissa launcher to draw the parameter sets of the simulations. It must return a Numpy array of floats of the size STUDY_OPTIONS['nb_parameters']

```python
USER_FUNCTIONS['create_group'] # (optional):
```

This function is called once for each group in the study, before launching the study. Its behavior is the same as `USER_FUNCTIONS['create_study']`.

```python
USER_FUNCTIONS['launch_server'] # (mandatory):
```

When every simulation, group and parameter set is defined, Melissa Launcher uses this function to launch Melissa Server. The function takes a Server object for argument, and needs to set the server job ID in `server.job_id`. On a cluster the job ID is given by the batch scheduler. In your local machine, you can use the process ID. The server command line options are in `server.cmd_opt` and the path to the executable `melissa_server` is in `server.path`, you must use it and not modify it.

<details>
<summary><em> Example </em></summary>

***
An example for a Slurm cluster:


```python
import subprocess

def launch_server(server):
      os.chdir(STUDY_OPTIONS['working_directory'])
      
      # create a job script
      file=open("run_server.sh", "w")
      content  = "#!/bin/bash \n"
      content += "# Example with Slurm \n"
      content += "#SBATCH -N 4 \n"
      content += "#SBATCH --job-name=Melissa_server \n"
      content += "mkdir stats${SLURM_JOB_ID}.resu \n"
      content += "cd stats${SLURM_JOB_ID}.resu \n"
      content += "mpirun "+server.path+"/server "+server.cmd_opt+" & \n"
      content += "wait %1 \n"
      content += "cd "+STUDY_OPTIONS['working_directory']+" \n"
      file.write(content)
      file.close()
          
      # run the job script
      proc = subprocess.Popen('sbatch "./run_server.sh"',
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE,
                              shell=True,
                              universal_newlines=True)
                                  
      # get the job ID
      (out, err) = proc.communicate()
      server.job_id = out.split()[-1]

USER_FUNCTIONS['launch_server'] = launch_server
```
            
You can also have a ready-to-use job script. Simply parse it to add the server command line, and submit it in the function.

In our `option.py` file, we see that we can use the PID of the main process of the server as an identifier. It will be useful later for the fault tolerance mechanism.

***

</details>
<br />

```python
USER_FUNCTIONS['launch_group'] # (mandatory):
```

This function is used to launch simulations. The function takes a Group object for argument.
The group object provides four important attributes:

* `rank`: the rank of the group in the study
* `simu_id`: the list of IDs of the simulations of the group in the study
* `param_set`: the list parameter sets of the simulations of the group
* `job_id`: the ID of the group, used for fault-tolerance
* `server_node_name`: the name of the main server node, provided by the launcher

We distinguish two kinds of groups:

* Batch of simulations:  Run a batch of simulations in one job. In this case, `simu_id` is a list of size `STUDY_OPTIONS['batch_size']` and `rank` is the rank of the group. `param_set` is a list of size `STUDY_OPTIONS['batch_size']` of numpy arrays of size `STUDY_OPTIONS['nb_parameters']`.
The parameter `STUDY_OPTIONS['sampling_size']` must be a multiple of `STUDY_OPTIONS['batch_size']`. The `rank` attribute is the rank of the batch, and the `simu_id` attribute is a list of the simulation IDs of size `STUDY_OPTIONS['batch_size']`.

* Sobol group: In the case of Sobol' indices computation, all the simulations of a Sobol' group run in the same job. `simu_id` is a list of the simulation IDs inside the Sobol' group (the ones you will pass to `melissa_init`), and `rank` is the ID of the group. `param_set` is a list of size `STUDY_OPTIONS['nb_parameters'] + 2` of numpy arrays of size `STUDY_OPTIONS['nb_parameters']`, corresponding to the sets of n parameters of the n+2 simulations in the Sobol' group.

In each cases, all simulations from the same group must run in the same job. The simulation ID must be passed to the simulations using the MELISSA_SIMU_ID environment variable. The server node nam must be passed to the simulations using the MELISSA_SERVER_NODE_NAME environment variable.
The function needs to set the group job ID in the job_id attribute.
On a cluster the job ID is given by the batch scheduler. In your local machine, you can use the process ID.

```python
USER_FUNCTIONS['check_server_job'] # (mandatory):
```

This function is called while the server is running. It checks the job's status in the scope of the fault tolerance mechanism. It takes a Job object for argument, and needs to set job.job_status to 1 if the job is still running, an to 2 otherwise. A Server object inherits from Job, and the job ID is in job.job_id.

```python
USER_FUNCTIONS['check_group_job'] # (mandatory):
```

This function is called while a simulation is running. It checks the job's status in the scope of the fault tolerance mechanism. It takes a Job object for argument, and needs to set job.job_status to 1 if the job is still running, an to 2 otherwise. A Simulation object inherits from Job, and the job ID is in job.job_id.

```python
USER_FUNCTIONS['cancel_job'] # (mandatory):
```

This function is called by the launcher when a job have to be canceled. It takes a Job object for argument, and kills it. The job ID is defined in job.job_id.

<details>
<summary><em> Example </em></summary>

***
An example with a Slurm job:

```python
import os
      
def cancel_job(job):
    os.system('scancel '+job.job_id)
          
USER_FUNCTIONS['cancel_job'] = cancel_job
```
***
</details>   
<br />

```python
USER_FUNCTIONS['restart_server'] # (optional):
```

This function have the same behavior as `USER_FUNCTIONS['launch_server']`. It is used to reboot the server in the case of a fault. If it is not defined, Melissa Launcher uses the launch_server function by default.

```python
USER_FUNCTIONS['restart_group'] # (optional):
```

This function have the same behavior as `USER_FUNCTIONS['launch_group']`. It is used to reboot a simulation group in the case of a fault. If it is not defined, Melissa Launcher uses the `USER_FUNCTIONS['launch_group']` function by default.

```python
USER_FUNCTIONS['check_scheduler_load'] # (optional):
```

Melissa Launcher calls this function right before launching each simulation job. If it returns False, the launcher will not launch the simulation. Instead, il will wait one second, check the fault tolerance mechanism, and then retry until this function returns True.

```python
USER_FUNCTIONS['postprocessing'] # (optional):
```

This function is called once at the end of the study. It does nothing by default.

```python
USER_FUNCTIONS['finalize'] # (optional):
```

The same as `USER_FUNCTIONS['postprocessing']`.



# Run the study

To run a study, call the launcher with the path to your `options.py` file:

```bash
melissa_launcher -o <path/to/options.py>
```

If you only give a path to a directory, Melissa Launcher will search for a file named explicitly `options.py` in this directory. If you give the path to a file, Melissa Launcher will try load this file.
Melissa Launcher produces a log file named `melissa_launcher.log` in the location of the call to `melissa_launcher`.

<details>
<summary><em> Example </em></summary>

***
In our case, the file is names "options.py", so we can either do, from the `study_local` directory:

```bash
melissa_launcher -o ./options.py
 ```
or:
```bash
melissa_launcher -o ./
```

The results will be in the directory `STATS` because we defined it in `STUDY_OPTIONS['working_directory']`. The results files are of the form: `<field_name>_<stat>.<time_stamp>`
For example, the variance of the field "heat" at the first timestep is stored in `heat_variance.001`.

***

</details>
<br />

You can use the  [virtual cluster](../../utility/virtual_cluster/README.md) to test your code at small scale with a batch scheduler on a local machine before to move to the target supercomputer. That's a good way to solve issues progressively.

