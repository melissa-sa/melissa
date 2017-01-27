#!/bin/bash

command_line="-p 1 -s 3 -t 100 -o mean:variance:min:max"
let "t=0"
let "tmax=2"
while [ $t -le $tmax ]
do
    mpirun -n 3 ./heatf ${t} 0 ${t}&
    let "t+=1"
done
if [ ! -d "./resu" ];then
mkdir resu
fi
cd resu
mpirun -n 2 ../../../server/server $command_line
cd ..
