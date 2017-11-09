/******************************************************************
*                            Melissa                              *
*-----------------------------------------------------------------*
*   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    *
*                                                                 *
* This source is covered by the BSD 3-Clause License.             *
* Refer to the  LICENCE file for further information.             *
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
#ifdef BUILD_WITH_MPI
#include <mpi.h>
#endif // BUILD_WITH_MPI
#include "melissa_options.h"

int main (int argc, char **argv)
{
    melissa_options_t options;

#ifdef BUILD_WITH_MPI
    MPI_Init (&argc, &argv);
#endif // BUILD_WITH_MPI
    melissa_get_options(argc, argv, &options);
    melissa_print_options (&options);

#ifdef BUILD_WITH_MPI
    MPI_Finalize ();
#endif // BUILD_WITH_MPI

    return (0);
}
