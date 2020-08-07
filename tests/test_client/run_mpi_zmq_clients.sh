/******************************************************************
*                            Melissa                              *
*-----------------------------------------------------------------*
*   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    *
*                                                                 *
* This source is covered by the BSD 3-Clause License.             *
* Refer to the  LICENSE file for further information.             *
*                                                                 *
*-----------------------------------------------------------------*
*  Original Contributors:                                         *
*    Theophile Terraz,                                            *
*    Bruno Raffin,                                                *
*    Alejandro Ribes,                                             *
*    Bertrand Iooss,                                              *
******************************************************************/

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
