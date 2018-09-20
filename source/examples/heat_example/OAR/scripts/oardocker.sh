#! /bin/sh
# # Set Python virtual env:
# virtualenv -p python3.6 env3.6
# source env3.6/bin/activate
# # get and install oar-docker
# git clone https://github.com/oar-team/oar-docker.git
# cd oar-docker
# git checkout dev
# pip install -e .
# cd ..
# # init images vith debian stretch
oardocker init -e stretch
# copy the Dockerfile to install Melissa dependencies
cp Dockerfile .oardocker/images/base/Dockerfile
# build the images. must be done eauch time Dockerfile is modified
oardocker build
# install OAR on the cluster
oardocker install http://oar-ftp.imag.fr/oar/2.5/sources/testing/oar-2.5.8+rc5.tar.gz
# start the cluster, with -n <number of nodes>
oardocker start -n 3
# we can also share a directory:
# oardocker start -n 3 -v $HOME/mon_super_projet:/data
# get Melissa from github
oardocker exec frontend git clone https://github.com/melissa-sa/melissa.git
# configure, build and test melissa
oardocker exec frontend mkdir melissa/build
oardocker exec frontend cd melissa/build && cmake ../source -DBUILD_DOCUMENTATION=OFF -DINSTALL_ZMQ=ON && make install && source ../install/melissa_set_env.sh && ctest
# # stop the cluster
# docker stop $(docker ps -a -q)
# # delete the images
# docker rm $(docker ps -a -q)
# docker rmi $(docker images -q)
