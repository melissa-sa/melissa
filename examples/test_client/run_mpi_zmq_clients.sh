#!/bin/bash

param_sizes=(2 3)
command_line="-p 2:3 -t 3 -o mean:variance:max:threshold -e 10.5"
let "nb_parameters=2"
parameters=(0 0)
let "t=0"
while [ $t -eq 0 ]
do
    mpirun -n 3 ./client ${parameters[0]}:${parameters[1]} 15 3 &
    let "i=0"
    while [ ${parameters[$i]} -g ${param_sizes[$i]} ]
    do
        let "parameters[i] = 0"
        let "i+=1"
        if [ $i -ge $nb_parameters ]
        then
            break;
        fi
    done
    if [ $i -lt $nb_parameters ]
    then
        let "parameters[i]+=1"
    else
        let "t+=1"
    fi
    echo ${parameters[@]}
done
mpirun -n 2 ../../src/server $command_line
