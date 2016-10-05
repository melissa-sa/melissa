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
#include "stats.h"

int main (int argc, char **argv)
{
    stats_options_t options;

#ifdef BUILD_WITH_MPI
    MPI_Init (&argc, &argv);
#endif // BUILD_WITH_MPI
    stats_get_options(argc, argv, &options);
    print_options (&options);

#ifdef BUILD_WITH_MPI
    MPI_Finalize ();
#endif // BUILD_WITH_MPI

    return (0);
}
