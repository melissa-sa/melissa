#! /bin/bash

# Start slurm services on a running  oardocker cluster

# On nodes 
for i in $(docker ps  | grep -e "oar.*node" | awk '{print $1}') ; do
    echo     docker exec $i sudo systemctl start munge slurmd
    docker exec $i sudo systemctl start munge slurmd
done
# On server
    echo docker exec $(docker ps  | grep -e "oar.*server" | awk '{print $1}')  sudo systemctl start munge slurmctld
    docker exec $(docker ps  | grep -e "oar.*server" | awk '{print $1}')  sudo systemctl start munge slurmctld





