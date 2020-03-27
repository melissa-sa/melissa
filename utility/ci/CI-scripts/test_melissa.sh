#!/bin/bash
set -e
source $CI_PROJECT_DIR/install/bin/melissa_set_env.sh
cd $CI_PROJECT_DIR/build && ctest
