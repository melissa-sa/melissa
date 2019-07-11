# Melissa installation and usage guide

Aim of this tutorial is to install Melissa and run three sensitivity studies, one on the local machine, one on the local cluster and the last one on the actual cluster. After reading this guide, user will be able to install Melissa and run own sensitivity studies using own solver.

### Requirements

To run Melissa you will need:

* CMake
* C and Fortran Compiler
* MPI, OpenMP: for Melissa parallel server. A sequential server will be built if not available
  ZeroMQ: version 4.1.5 or above. CMake can download and install it if option turned on (-DINSTALL_ZMQ=ON)
* Python 3
* PythonLibs, Numpy

## Installing and running example study

### 1. Get latest Melissa source code.

You can manually download the zip package or use `git clone https://gitlab.inria.fr/melissa/melissa.git`

### 2. Building and installing Melissa

In repository folder:

```bash
mkdir build && cd build
cmake ../. -DINSTALL_ZMQ=ON
make install
```

Installed Melissa is in `../install` folder.

<details>
<summary>Other useful CMake options</summary>

```
-DCMAKE_INSTALL_PREFIX (default: '../install')    ->  Melissa install directory.
-DBUILD_WITH_MPI (default: ON)                    ->  Enable MPI.
-DBUILD_WITH_OpenMP (default: OFF)                ->  Enable OpenMP for Melissa Server.
-DINSTALL_ZMQ (default: OFF)                      ->  Allows CMake to install ZeroMQ.
-DBUILD_DOCUMENTATION (default: OFF)              ->  If Doxygen is found, build the Doxygen documentation.
-DBUILD_TESTING (default: ON)                     ->  Build Melissa tests. They can be run with "make test" or "ctest".
```

</details>

<br/>

We also need to compile the solver. Solver is the simulation program that take some input and send the result to Melissa server, which does the statistics. In this tutorial we will use the `heat_example` as it's easy to run and doesn't require too much computing power. Later in this guide we will write our own solver.

```bash
source ../install/bin/melissa_set_env.sh #required environmental variables to compile solver
cd ../install/share/melissa/examples/heat_example/solver
mkdir build && cd build
cmake ../.
```
Thats it, now you can run `heat_example` simulation.


### 3. Running Melissa

**!! Every time before you run Melissa you need to set environmental variables !!**

```bash
source install/bin/melissa_set_env.sh
```

<details>

<summary>Running localy</summary>

<br/>

### Running the study

Running Melissa localy means that it will run without the batch scheduler. Change folder to `heat_example/study_local` and run `melissa_launcher -o options`. 

### Problems

Any problem occured here can be an indication of bad installation. Be sure to check if you have every dependency and if you follow the instruction propperly

</details>

<details>

<summary>Running on a local cluster</summary>

<br/>

### Installing the OAR-docker

To run the study you will need to setup an OAR batch scheduler to mimic cluster on your local machine. It's a very handy development environment. You will need Docker and virtualenv installed on your machine. 

Go into the directory `install/share/melissa/examples/heat_example/study_OAR` and create virtual environment. OAR-docker requires python 3.5 or higher.

```bash
virtualenv -p python3.5 env3.5
source env3.5/bin/activate
```

Install OAR-docker

```bash
git clone https://github.com/oar-team/oar-docker.git
cd oar-docker
git checkout dev
pip install -e .
cd ..
```

Initialize Docker image with a minimal Debian Stretch linux distribution

```bash
oardocker init -e stretch
```

Replace regular Dockerfile that comes with OAR-docker with one that includes dependencies for Melissa and build the image

```bash
cp ./scripts/Dockerfile .oardocker/images/base/Dockerfile
oardocker build
```

Install OAR on the cluster from the web

```bash
oardocker install http://oar-ftp.imag.fr/oar/2.5/sources/testing/oar-2.5.8+rc5.tar.gz
```

The cluster is installed and ready to run.

### Starting and using a virtual cluster

Start the cluster with 3 nodes. You can share the Melissa directory with the host machine to avoid having to download and install melissa everytime you start the cluster

```bash
oardocker start -n 3  -v ~/path/to/melissa/on/host/machine:/home/docker/melissa
```

Connect to the frontend and if u shared Melissa folder it will be in `/home/docker/melissa`

```bash
oardocker connect frontend
```

Now, just follow standard installation of Melissa and `heat_example` solver. After this you are ready to launch sensitivity study. When you are finished, exit from oardocker and run `oardocker stop`. If you want to restart just type 2 commands at the start of the chapter.

To run the study type in `study_OAR` folder 
```bash
melissa_launcher -o options.py
```

### Problems

* Some networks block public DNS servers to encourage people to use network's own DNS server. Docker containers default to Google's 8.8.8.8 public DNS server. [This solution](https://development.robinwinslow.uk/2016/06/23/fix-docker-networking-dns/) can fix the problem of containers not updating and installing dependencies.
  

</details>

<details>

<summary>Running on a cluster</summary>

<br/>

### Adapting code to the environment

To run Melissa on a cluster you need to adapt the code to your batch scheduling systems and system environment. Before you try to run anything on remote cluster, please familiarize yourself with batch scheduling system of your cluster and options file (short but precise enough description is in the `Creating your own study` chapter).

Firstly, in your `study_Slurm/scripts` folder, change the shell files so they are valid with your scheduling system. Pay attention to the commands and partition names.

Secondly, in the same shell files add/load appropriate modules for MPI.

Thirdly, check options file. Look especially at `launch_server` and `launch_group` and check if commands sending jobs to batch scheduler are valid.

Lastly, check the shebang at `melissa_launcher`. This python interpreter have to have Numpy. If you know that some module have python with numpy, you can

```bash
module load *python module with numpy*
which python3
```

and copy the path to the shebang.

</details>

## Creating your own study

### 1. The options file

The options file is located in every example and it's mandatory to run Melissa. The file contains python code with 3 python dictionaries:

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

### 2. Writing own solver