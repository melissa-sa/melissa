#!/bin/bash

command_line="-p 2:2 -t 100 -o sobol"
let "t=0"
let "tmax=1"
let "g=0"
let "gmax=4"
while [ $t -le $tmax ]
do
  while [ $g -lt $gmax ]
  do
    mpirun -n 1 ./heatc ${t} ${g}&
    let "g+=1"
  done
  let "g=0"
let "t+=1"
done
if [ ! -d "./resu" ];then
mkdir resu
fi
cd resu
mpirun -n 1 ../../../src/server $command_line
#valgrind --leak-check=full mpirun -n 2 ../../../src/server $command_line
cd ..
