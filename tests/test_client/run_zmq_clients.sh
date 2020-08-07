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

param_sizes=(2 3 5)
command_line="-p 2:3:5 -t 3 -o variance:max:threshold -e 10.5"
let "nb_parameters=3"
parameters=(0 0 0)
let "t=0"
while [ $t -eq 0 ]
do
    ./client ${parameters[0]}:${parameters[1]}:${parameters[2]} 15 3 &
    let "i=0"
    while [ ${parameters[$i]} -ge ${param_sizes[$i]} ]
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
../../src/server $command_line
