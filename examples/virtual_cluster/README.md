#Virtual culster 

This directory contains the necessary files and scripts to install and run a virtual cluster
based on the oardocker work from https://oar.imag.fr/wiki:oar-docker

Modifications made include:
* installing melissa dependencies
* installing and enabling slurm


## Virtual cluster install and setup

For installing the virtual cluster rely on the script

```bash
    oardocker.sh
```

For starting a virtual cluster

```bash
   oardocker start -n 3
```

This virtual cluster will have:
* 3 compute nodes: node1, node2, node3  (if more nodes needed rememebr to also change slurm.conf accordingly)
* 1 server
* 1 frontend


For connecting to a  cluster node
```bash
oardocker connect frontend
```
To exec command on the frontend for instance:
```bash
oardocker exec frontend ls -al 
```
To stop the custer
```bash
oardocker stop 
```

To stop the virtual machines:
```bash
docker stop $(docker ps -a -q)
```
To delete the images

```bash
 docker rm $(docker ps -a -q)
 docker rmi $(docker images -q)
```

To copy a file to a virtualmachine (use `docker ps` to retreive the vm name):
```bash
 docker cp  slurm.conf 72ca2488b353:/etc/slurm-llnl/slurm.conf
``` 

## Installing Melissa

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




## Running the Heat Example  with OAR

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


## Running the Heat Example  with SLURM

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









