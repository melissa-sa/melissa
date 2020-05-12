#!/bin/bash
set -e
#install melissa on oardocker
cd /home/docker/melissa
mkdir build && cd build
cmake ../. -DINSTALL_ZMQ=ON
make install
cd /home/docker/melissa/install
source bin/melissa_set_env.sh
cd share/melissa/examples/heat_example/solver
mkdir build && cd build
cmake ../.
make install
