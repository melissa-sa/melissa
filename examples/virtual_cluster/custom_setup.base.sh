#!/usr/bin/env bash

#echo "Melissa Deps"
apt-get update
apt-get install -y pkg-config
apt-get install -y gfortran libopenmpi-dev openmpi-bin
apt-get install -y cmake cmake-curses-gui
# assumption: Python3 module is available in the Virtual Cluster
#apt-get install -y python3
python3 -m pip install numpy


# SLURM
echo "Slurm"
#apt-get update
apt-get install -y slurm-wlm
#apt-get install -y mailutils
#cp /tmp/config/slurm.conf /etc/slurm-llnl/slurm.conf # done in slurm_start.sh
 # Does not work. Use slurm_start.sh  instead 
#systemctl enable munge


