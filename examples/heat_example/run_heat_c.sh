#!/bin/bash

command_line="-p 1 -s 3 -t 100 -o mean:variance:min:max"
let "t=0"
let "tmax=2"
if [ ! -d "./resu" ];then
mkdir resu
fi
cd resu
mpirun -n 1 ../../../src/server $command_line&
cd ..
while [ $t -le $tmax ]
do
    mpirun -n 1 ./heatc ${t} 0 ${t}
    let "t+=1"
done
