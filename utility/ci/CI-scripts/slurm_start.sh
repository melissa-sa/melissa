#! /bin/bash
set -e
cd $HOME/CI-scripts
# Start slurm services on a running  oardocker cluster


# Collect container ids
nodes=$(docker ps  | grep -e "oar.*node" | awk '{print $1}')
nb_nodes=$(echo "$nodes" | wc -w)
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


# Update  slurm conf file with actual number of compute nodes and copy to cluster nodes
sed -n -e s/nb_nodes/"$nb_nodes"/ -e p slurm.conf.in  >  slurm.conf
dockercp  "$nodes $server $frontend"  "slurm.conf"  "/etc/slurm-llnl/slurm.conf"

# Start services

dockerexec  "$nodes"  "systemctl start  munge slurmd"
dockerexec  "$frontend"  "systemctl start  munge"
dockerexec  "$server"  "systemctl start  munge slurmctld"

rm slurm.conf
