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
 * @file test_base.c
 * @brief Basic tests to try basic statistic computations.
 * @author Terraz Théophile
 * @date 2016-15-02
 *
 **/

#include <melissa/server/data.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void d_print_vector (double    in_vect[],
                            const int vect_size,
                            const int rank,
                            const int comm_size,
                            MPI_Comm  comm)
{
    double     *temp_vect;
    int         i, j;
    MPI_Status  status;
    static int tag = 10000;

    if (rank == 0)
    {
        temp_vect = malloc (vect_size*sizeof(double));

        printf ("process[0]: {");
        for (i=0; i<vect_size; i++)
            fprintf (stdout, " %g", in_vect[i]);
        printf (" }\n");

        for (i=1; i<comm_size; i++)
        {
            MPI_Recv (temp_vect, vect_size, MPI_DOUBLE, i, tag+i, comm, &status);

            printf ("                 process[%d]: {", i);
            for (j=0; j<vect_size; j++)
                fprintf (stdout, " %g", temp_vect[j]);
            printf (" }\n");
        }

        free (temp_vect);
    }
    else
    {
        MPI_Send (in_vect, vect_size, MPI_DOUBLE, 0, tag+rank, comm);
    }
    tag += comm_size;
    MPI_Barrier(comm);
}

void i_print_vector (int       in_vect[],
                            const int vect_size,
                            const int rank,
                            const int comm_size,
                            MPI_Comm  comm)
{
    int        *temp_vect;
    int         i, j;
    MPI_Status  status;
    static int tag = 0;


    if (rank == 0)
    {
        temp_vect = malloc (vect_size*sizeof(int));

        printf ("process[0]: {");
        for (i=0; i<vect_size; i++)
            fprintf (stdout, " %d", in_vect[i]);
        printf (" }\n");

        for (i=1; i<comm_size; i++)
        {
            MPI_Recv (temp_vect, vect_size, MPI_INT, i, tag+i, comm, &status);

            printf ("                 process[%d]: {", i);
            for (j=0; j<vect_size; j++)
                fprintf (stdout, " %d", temp_vect[j]);
            printf (" }\n");
        }

        free (temp_vect);
    }
    else
    {
        MPI_Send (in_vect, vect_size, MPI_INT, 0, tag+rank, comm);
    }
    tag += comm_size;
    MPI_Barrier(comm);
}

int main (int argc, char **argv)
{
    double      *tableau = NULL;
    mean_t       my_mean;
    variance_t   my_variance;
    min_max_t    my_min_and_max;
    threshold_t  my_threshold_exceedance;
    int         *my_temp_exceedance = NULL;
    double      *temp_variance = NULL;
    int          i, j;
    int          n = 500; // n expériences
    int          vect_size = 50000; // size points de l'espace
    int          rank = 0;

    int comm_size;
    MPI_Init (&argc, &argv);
    MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);

    init_mean (&my_mean, vect_size);
    init_variance (&my_variance, vect_size);
    init_min_max (&my_min_and_max, vect_size);
    init_threshold (&my_threshold_exceedance, vect_size, 500.0);
    tableau                 = calloc (vect_size, sizeof(double));
    temp_variance           = calloc (vect_size, sizeof(double));
    my_temp_exceedance      = calloc (vect_size, sizeof(int));
//    tab_ptr = tableau;
//    for (j=0; j<vect_size; j++, tab_ptr++)
//    {
//        *tab_ptr += rank*n;
//    }
    for (j=0; j<vect_size; j++)
    {
        tableau[j] = rand() / (double)RAND_MAX * (1000 * (rank+1));
    }

    for (i=0; i<n; i++)
    {
        increment_mean (&my_mean, tableau, vect_size);
        increment_variance (&my_variance, tableau, vect_size);
        min_and_max (&my_min_and_max, tableau, i, vect_size);
        update_threshold_exceedance (&my_threshold_exceedance, tableau, vect_size);

        for (j=0; j<vect_size; j++)
        {
            tableau[j] = rand() / (double)RAND_MAX * (1000 * (rank+1));
        }
    }

    update_global_mean(&my_mean, vect_size, rank, comm_size, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    update_global_variance(&my_variance, vect_size, rank, comm_size, MPI_COMM_WORLD);

    MPI_Reduce (my_min_and_max.min, tableau, vect_size, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    if(rank == 0) memcpy (my_min_and_max.min, tableau,  vect_size*sizeof(double));
    MPI_Reduce (my_min_and_max.max, tableau, vect_size, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if(rank == 0) memcpy (my_min_and_max.max, tableau,  vect_size*sizeof(double));
    MPI_Reduce (my_threshold_exceedance.threshold_exceedance, my_temp_exceedance, vect_size, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if(rank == 0) memcpy (my_threshold_exceedance.threshold_exceedance, my_temp_exceedance,  vect_size*sizeof(int));

    free_mean (&my_mean);
    free_variance (&my_variance);
    free_min_max (&my_min_and_max);
    free_threshold(&my_threshold_exceedance);
    free (tableau);
    free (temp_variance);
    free (my_temp_exceedance);

    MPI_Finalize ();

    return 0;
}
