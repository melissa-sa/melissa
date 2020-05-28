#!/bin/bash
set -e
cd $CI_PROJECT_DIR
mkdir build && cd build
cmake ../. -DINSTALL_ZMQ=ON
make install
cd $CI_PROJECT_DIR/install
source bin/melissa_set_env.sh
cd share/melissa/examples/heat_example/solver
mkdir build && cd build
cmake ../.
make install
