#!/bin/bash
set -e
cd $HOME/CI-scripts
# copy install and test scripts
cp install_melissa_oardocker.sh test_OAR_oardocker.sh $CI_PROJECT_DIR
cd .. && oardocker start -n 3
# get frontend ID
frontend_ID=$(docker ps | grep frontend | cut -d ' ' -f 1)
# copy project dir to container and change ownership
docker cp $CI_PROJECT_DIR $frontend_ID:/home/docker/melissa
oardocker exec -w /home/docker --no-tty frontend sudo chown -R docker:docker melissa
# run install and tests
docker exec -w /home/docker/melissa -u docker $(docker ps | grep frontend | awk '{print $1}') bash install_melissa_oardocker.sh
docker exec -w /home/docker/melissa -u docker $(docker ps | grep frontend | awk '{print $1}') bash test_OAR_oardocker.sh
