#!/bin/bash

param_sizes=(2 3)
command_line="-p 2:3 -t 3 -o mean:variance:max:threshold -e 10.5"
let "nb_parameters=2"
let "nb_simu=3"
let "t=0"
while [ $t -lt 3 ]
do
    mpirun -n 3 ./client 2:$t 15 3 &
    let "t+=1"
done
mpirun -n 2 ../../src/server $command_line
