#! /bin/sh

# Script to install a oardocker virtual cluster with melissa dependencies installed, oar and slurm batch scehduler.

# # Set Python virtual env:
 virtualenv -p python3.5 env3.5
 source env3.5/bin/activate
# # get and install oar-docker
 git clone https://github.com/oar-team/oar-docker.git
 cd oar-docker
 git checkout dev
 pip install -e .
 cd ..
 # # init images vith debian stretch
export LC_ALL=C.UTF-8
export LANG=C.UTF-8
oardocker init -e stretch
# Add to  the Dockerfile commands to install   Melissa dependencies and Slurm and activate required services:
#cp Dockerfile .oardocker/images/base/Dockerfile
#cat Dockerfile.base >> .oardocker/images/base/Dockerfile
#sed '/ADD . \/tmp/  r custom.base' < .oardocker/images/base/custom_setup.sh > .oardocker/images/base/custom_setup.sh
#cat custom.base >>  .oardocker/images/base/custom_setup.sh
#cat custom.node >> .oardocker/images/node/custom_setup.sh
#cat custom.server >> .oardocker/images/server/custom_setup.sh
cat Dockerfile.base >>  .oardocker/images/base/Dockerfile
cat Dockerfile.node >> .oardocker/images/node/Dockerfile
cat Dockerfile.server >> .oardocker/images/server/Dockerfile
# copy the slurm config file
cp slurm.conf .oardocker/images/base/config/slurm.conf
# build the images. must be done each time Dockerfile is modified
oardocker build
# install OAR on the cluster
oardocker install http://oar-ftp.imag.fr/oar/2.5/sources/testing/oar-2.5.8.tar.gz
# start the cluster, with -n <number of nodes>
oardocker start -n 3
# we can also share a directory:
# oardocker start -n 3 -v $HOME/mon_super_projet:/data
# get Melissa from github
#oardocker exec frontend git clone https://github.com/melissa-sa/melissa.git
# configure, build and test melissa
#oardocker exec frontend mkdir melissa/build
#oardocker exec frontend cd melissa/build && cmake ../source -DBUILD_DOCUMENTATION=OFF -DINSTALL_ZMQ=ON && make install && source ../install/melissa_set_env.sh && ctest
# # stop the cluster
# docker stop $(docker ps -a -q)
# # delete the images
# docker rm $(docker ps -a -q)
# docker rmi $(docker images -q)
