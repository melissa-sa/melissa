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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mpi.h>
#include "melissa_api.h"

static inline void error(const int ret)
{
    MPI_Abort(MPI_COMM_WORLD, ret);
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

    MPI_Init (&argc, &argv);
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_size (comm, &comm_size);
    MPI_Comm_rank (comm, &rank);

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

    melissa_init (&my_vect_size,
                  &comm_size,
                  &rank,
                  &sobol_tab[0],
                  &sobol_tab[1],
                  &comm,
                  &coupling);

    while (time_step < nb_time_steps)
    {
        gen_data(out_vect, &time_step, my_vect_size, rank);
        sleep (1);

        melissa_send (&time_step,
                      "heat",
                      out_vect,
                      &rank,
                      &sobol_tab[0],
                      &sobol_tab[1] );
    }

    melissa_finalize ();

    free(out_vect);

    MPI_Finalize ();

    return 0;
}
