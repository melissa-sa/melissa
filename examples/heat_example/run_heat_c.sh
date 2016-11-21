#!/bin/bash

command_line="-p 1 -g 3 -t 100 -e 2 -o mean:variance:min:max:threshold"
#command_line="-r"
let "t=0"
let "tmax=2"
while [ $t -le $tmax ]
do
    mpirun -n 3 ./heatc ${t} 0 ${t} &
    let "t+=1"
done
if [ ! -d "./resu" ];then
mkdir resu
fi
cd resu
mpirun -n 2 ../../../src/server $command_line
cd ..
#sleep 2
#killall -USR1 mpirun
