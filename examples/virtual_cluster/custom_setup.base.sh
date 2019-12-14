#!/usr/bin/env bash

#echo "Melissa Deps"
apt-get update
apt-get install -y pkg-config
apt-get install -y gfortran libopenmpi-dev openmpi-bin
apt-get install -y cmake cmake-curses-gui
# assumption: Python3 module is available in the Virtual Cluster
#apt-get install -y python3
python3 -m pip install numpy
## TODO: review at the end:
python3 -m pip install async_generator
python3 -m pip install asyncio
python3 -m pip install xml
python3 -m pip install jinja2
python3 -m pip install tornado
python3 -m pip install subprocess
python3 -m pip install jupyterhub
python3 -m pip install traitlets


# Jupyter Notebook
echo "Jupyter Notebook"
python3 -m pip install jupyter matplotlib ipywidgets plotly
# https://stackoverflow.com/questions/51676835/ipython-cannot-import-name-create-prompt-application-from-prompt-toolkit-s
python3 -m pip install 'prompt-toolkit<2.0.0,>=1.0.15' --force-reinstall
jupyter nbextension enable --py widgetsnbextension
jupyter nbextension enable --py plotlywidget


# SLURM
echo "Slurm"
#apt-get update
apt-get install -y slurm-wlm
#apt-get install -y mailutils
#cp /tmp/config/slurm.conf /etc/slurm-llnl/slurm.conf # done in slurm_start.sh
 # Does not work. Use slurm_start.sh  instead
#systemctl enable munge


