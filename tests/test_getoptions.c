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

/**
 *
 * @file test_getoptions.c
 * @brief Basic test to try stats_get_options.
 * @author Terraz Théophile
 * @date 2016-04-03
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>
#include "melissa_options.h"

int main (int argc, char **argv)
{
    melissa_options_t options;

    MPI_Init (&argc, &argv);
    melissa_get_options(argc, argv, &options);
    melissa_print_options (&options);

    MPI_Finalize ();

    return (0);
}
