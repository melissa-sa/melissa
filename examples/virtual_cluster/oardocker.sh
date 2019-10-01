#! /bin/bash -e

# Script to install a oardocker virtual cluster with melissa dependencies installed, oar and slurm batch scehduler.

if [ "$1" != "start" ]
then
    # # get and install oar-docker
    git clone https://github.com/oar-team/oar-docker.git
    cd oar-docker
    git checkout dev
    python -m pip install -e .
    cd ..

    # # init images vith debian stretch
    export LC_ALL=C.UTF-8
    export LANG=C.UTF-8
    oardocker init -e stretch

    # Add to  the Dockerfile commands to install   Melissa dependencies and Slurm:
    cp custom_setup.base.sh  .oardocker/images/base/custom_setup.sh
    cp custom_setup.node.sh  .oardocker/images/node/custom_setup.sh
    cp custom_setup.server.sh  .oardocker/images/server/custom_setup.sh

    # copy the slurm config file (done in slurm_start.sh)
    # cp slurm.conf .oardocker/images/base/config/slurm.conf

    # build the images. must be done each time Dockerfile is modified
    oardocker build
    # install OAR on the cluster
    oardocker install http://oar-ftp.imag.fr/oar/2.5/sources/testing/oar-2.5.8.tar.gz
fi

if [ "$1" != "install" ]
then
    # start the cluster, with -n <number of nodes>
    oardocker start -n 3 -v $PWD/../../../:/home/docker/melissa
    # we can also share a directory:
    # oardocker start -n 4 -v $HOME/mon_super_projet:/data


    # Start slurm on the virtual cluster
    ./slurm_start.sh
fi

# Get Melissa from github
#oardocker exec frontend git clone https://github.com/melissa-sa/melissa.git

# Configure, build, install and test  melissa
#oardocker exec frontend mkdir melissa/build
#oardocker exec frontend cd melissa/build && cmake ../source -DBUILD_DOCUMENTATION=OFF -DINSTALL_ZMQ=ON && make install && source ../install/melissa_set_env.sh && ctest

# # Stop the cluster 
# oardocker stop 
# # Stop and remove all images (except the base lunix container) 
# docker destroy 

# Similar but from docker 
# # Stop the cluster
# docker stop $(docker ps -a -q)
# # delete the containers and images
# docker rm $(docker ps -a -q)
# docker rmi $(docker images -q)
