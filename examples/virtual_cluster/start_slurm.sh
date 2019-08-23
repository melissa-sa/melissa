#! /bin/bash

# Start slurm services on a running  oardocker cluster

# On nodes
for i in $(docker ps  | grep -e "oar.*node" | awk '{print $1}') ; do
    cmd="docker exec $i sudo systemctl start munge slurmd"
    echo $cmd
    $cmd
done
# On server
cmd="docker exec $(docker ps  | grep -e "oar.*server" | awk '{print $1}')  sudo systemctl start munge slurmctld"
echo $cmd
$cmd
# On frontend
cmd="docker exec $(docker ps  | grep -e "oar.*frontend" | awk '{print $1}') sudo systemctl start munge"
echo $cmd
$cmd
