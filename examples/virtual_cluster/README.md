# Virtual Cluster 

## Table of contents
* [Introduction](#intro)
* [Docker Installation](#docker)
* [Virtual Cluster Installation](#cluster)
* [Installing Melissa](#melissa)
* [Running the Heat Example  with OAR](#heatoar)
* [Running the Heat Example  with Slurm](#heatslurm)



## Introduction <a name="intro"></a>
We show here how to setup a virtual cluster on your local machine, and how to run
the Melissa heat example. The  virtual cluster enables you to run various nodes, through Docker containers,
managed by the [OAR batch scheduler](http://oar.imag.fr)  on  your local machine.

This is a very handy intermediate step to validate your developments without having the burden to
confront directly with the supercomputer environment.


This directory contains the necessary files and scripts to install and run a virtual cluster with 2 batch schedulers: OAR and Slurm.

We modified the original [OAR-docker](https://oar.imag.fr/wiki:oar-docker) to:

* installing melissa dependencies
* installing and enabling slurm




## Docker Installation <a name="docker"></a>

```bash
sudo apt-get install docker.io
```

Add your user to the docker group:

```bash
sudo usermod -aG docker $USER
```

logout/login for  the changes to be activated.

To check the groups your user id is associated to:

```bash
id
```

## Virtual Cluster Installation <a name="cluster"></a>

For installing the virtual cluster rely on the script:

```bash
oardocker.sh
```

For starting a virtual cluster with OAR batch scheduler :

```bash
oardocker start -n 3
```

For starting SLURM on the virtual cluster :

```bash
./start_slurm.sh
```

This virtual cluster will have:
* 3 compute nodes: node1, node2, node3  (if more nodes needed remember to also change slurm.conf accordingly on all nodes and the server)
* 1 server
* 1 frontend
where the  frontend and nodes share the `/home` directory.


For connecting to a  cluster node
```bash
oardocker connect frontend
```
To exec command on the frontend for instance:
```bash
oardocker exec frontend ls -al 
```
To stop the cluster
```bash
oardocker stop 
```

To stop the virtual machines:
```bash
docker stop $(docker ps -a -q)
```
To delete the running containers and the images

```bash
docker rm $(docker ps -a -q)
docker rmi $(docker images -q)
```

To copy a file to a virtual machine (use `docker ps` to retreive the vm name):
```bash
docker cp  slurm.conf 72ca2488b353:/etc/slurm-llnl/slurm.conf
``` 

## Installing Melissa <a name="melissa"></a>

Start a virtual cluster. You can share the Melissa directory with the host machine to avoid having to download and install melissa everytime you start the cluster:

```bash
oardocker start -n 3  -v ~/path/to/melissa/on/host/machine:/home/docker/melissa
```

Connect to the frontend:

```bash
oardocker connect frontend
```

You are now on the cluster frontend. If you shared the Melissa folder like shown above, you should find it in

```bash
/home/docker/melissa
```

You can also just clone it from the repo:
```bash
git clone https://github.com/melissa-sa/melissa.git
```

### Compile Melissa 

Go to melissa directory and recompile in specific build and install directories, to avoid  mixing  it with your local host install (if you shared the directory):

```bash
cd ~/melissa/Melissa
mkdir build-oardocker
cd build-oardocker
cmake ../source -DBUILD_DOCUMENTATION=OFF -DINSTALL_ZMQ=ON -DCMAKE_INSTALL_PREFIX=/home/docker/melissa/Melissa/install-oardocker
make install
source ../install-oardocker/bin/melissa_set_env.sh
```

### Compile the Study 

Compile the heat example solver:

```bash
cd /home/docker/melissa/Melissa/install-oardocker/share/melissa/examples/heat_example/
mkdir build
cd build
cmake ../solver
make
make install
cd ..
```




## Running the Heat Example  with OAR <a name="heatoar"></a>

Go to the installed heat example directory in the OAR specific directory:

```bash
cd /home/docker/melissa/Melissa/install-oardocker/share/melissa/examples/heat_example/study_OAR
```
and run:

```bash
melissa_launcher -o options.py &
```

To monitor job execution:

```bash
oarstat
```

At the end of the study, the results are in

```
/home/docker/melissa/Melissa/install-oardocker/share/melissa/examples/heat_example/OAR/STATS
```

### OAR Monitoring tools

You can see the status of your virtual cluster in your favorite web browser using the Monica web service:

```
http://localhost:40080/monika/
```
And the Gantt chart of the jobs using the Drawgantt tool:
```
http://localhost:40080/drawgantt-svg/
```


## Running the Heat Example  with SLURM <a name="heatslurm"></a>

Go to the installed heat example directory in the SLURM specific directory:

```bash
cd /home/docker/melissa/Melissa/install-oardocker/share/melissa/examples/heat_example/study_Slurm
```
and run:

```bash
melissa_launcher -o options.py &
```
To monitor job execution:

```bash
sinfo -i 5
```

At the end of the study, the results are in

```
/home/docker/melissa/Melissa/install-oardocker/share/melissa/examples/heat_example/Slurm/STATS
```









