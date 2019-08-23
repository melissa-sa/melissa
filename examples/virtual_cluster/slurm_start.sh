#! /bin/bash

# Start slurm services on a running  oardocker cluster


# Collect container ids
nodes=$(docker ps  | grep -e "oar.*node" | awk '{print $1}')
server=$(docker ps  | grep -e "oar.*server" | awk '{print $1}')
frontend=$(docker ps  | grep -e "oar.*frontend" | awk '{print $1}')


# Call "docker exec ...." with 2 args : dockerexec targetnodes  cmd   
function dockerexec {
    
    for i in $1; do
	cmd="docker exec $i sudo $2"
	echo $cmd 
	$cmd
    done
}
# Call "docker cp localfile  tocontainerfile" with 2 args : dockerexec targetnodes  srcfile targetfile
function dockercp {
    
    for i in $1; do
	cmd="docker cp  $2 $i:$3"
	echo $cmd 
	$cmd
    done
}


# CP slurm conf file 
dockercp  "$nodes $server"  "slurm.conf"  "/etc/slurm-llnl/slurm.conf"


# Start services

dockerexec  "$nodes"  "systemctl start  munge slurmd"
dockerexec  "$frontend"  "systemctl start  munge"
dockerexec  "$server"  "systemctl start  munge slurmctld"

# Resume nodes (in some case start in drain state)
dockerexec  "$frontend"  "scontrol update NodeName=node[1-3] State=RESUME"
