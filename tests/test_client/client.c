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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef BUILD_WITH_MPI
#include <mpi.h>
#include "melissa_api.h"
#endif // BUILD_WITH_MPI
#include "melissa_api_no_mpi.h"

static inline void error(const int ret)
{
#ifdef BUILD_WITH_MPI
    MPI_Abort(MPI_COMM_WORLD, ret);
#else // BUILD_WITH_MPI
    exit(ret);
#endif // BUILD_WITH_MPI
}

static inline void gen_data (double       *out_vect,
                             int          *time_step,
                             int           vect_size,
                             int           rank)
{
    int i = 0;
     ++ *time_step;

    for (i=0; i<vect_size; i++)
    {
        out_vect[i] = rank;
    }
}

static inline void get_size_param (char *name,
                                   int  *nb_parameters,
                                   int  *nb_simu)
{
    const char  s[2] = ":";
    char       *temp_char;

    temp_char = strtok (name, s);
    *nb_parameters = atoi (temp_char);
    temp_char = strtok (NULL, s);
    *nb_simu = atoi (temp_char);
}

int main(int argc, char **argv)
{
    int     time_step = 0, nb_time_steps;
//    int    *parameters = NULL;
    double *out_vect;
    int     nb_parameters;
    int     nb_simu;
    int     coupling = 0;
    int     vect_size, my_vect_size;
    int     comm_size, rank;
    const int sobol_tab[2] = {0,0};

#ifdef BUILD_WITH_MPI
    MPI_Init (&argc, &argv);
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_size (comm, &comm_size);
    MPI_Comm_rank (comm, &rank);
#else // BUILD_WITH_MPI
    rank = 0;
    comm_size = 1;
#endif // BUILD_WITH_MPI

    if (argc < 4 || argv[1] == NULL || strcmp (argv[1], "-") == 0 || strcmp (argv[1], ":") == 0)
    {
       fprintf(stderr,"usage: %s param1:param2:...:paramN vect_size nb_time_steps\n", argv[0]);
       error(0);
    }

    vect_size      = atoi (argv[2]);
    nb_time_steps  = atoi (argv[3]);

    get_size_param (argv[1], &nb_parameters, &nb_simu);

    my_vect_size = vect_size / comm_size;
    if (rank < vect_size % comm_size)
        my_vect_size += 1;

    out_vect = calloc (my_vect_size, sizeof(double));

#ifdef BUILD_WITH_MPI
    melissa_init (&my_vect_size,
                  &comm_size,
                  &rank,
                  &sobol_tab[0],
                  &sobol_tab[1],
                  &comm,
                  &coupling);
#else // BUILD_WITH_MPI
    melissa_init_no_mpi (&my_vect_size,
                         &sobol_tab[0],
                         &sobol_tab[1]);
#endif // BUILD_WITH_MPI

    while (time_step < nb_time_steps)
    {
        gen_data(out_vect, &time_step, my_vect_size, rank);
        sleep (1);

#ifdef BUILD_WITH_MPI
        melissa_send (&time_step,
                      "heat",
                      out_vect,
                      &rank,
                      &sobol_tab[0],
                      &sobol_tab[1] );
#else // BUILD_WITH_MPI
        melissa_send_no_mpi (&time_step,
                             "heat",
                             out_vect,
                             &sobol_tab[0],
                             &sobol_tab[1] );
#endif // BUILD_WITH_MPI
    }

    melissa_finalize ();

    free(out_vect);

#ifdef BUILD_WITH_MPI
    MPI_Finalize ();
#endif // BUILD_WITH_MPI

    return 0;
}
