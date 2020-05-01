#!/bin/bash
set -e
#test OAR on oardocker
source /home/docker/melissa/install/bin/melissa_set_env.sh
cd /home/docker/melissa/install/share/melissa/examples/heat_example/study_OAR
melissa_launcher -o options.py
