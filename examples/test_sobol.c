/**
 *
 * @file test_sobol.c
 * @brief Basic tests to try sobol indices computations.
 * @author Terraz Théophile
 * @date 2016-15-02
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stats.h"

int main (int argc, char **argv)
{
    stats_options_t     options;
    stats_data_t        data;
    double             *tableau = NULL; // "input" values array
    int                *parameters = NULL;
    int                 incr, ret;
    int                 i, j;
    int                 comm_size, rank;

#ifdef BUILD_WITH_MPI
    MPI_Init (&argc, &argv);
    MPI_Comm_size (MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank (MPI_COMM_WORLD, &rank);
#else // BUILD_WITH_MPI
    rank = 0;
    comm_size = 1;
#endif // BUILD_WITH_MPI

    // init options
    options.threshold       = 0.0;
    options.mean_op         = 0;
    options.variance_op     = 1;
    options.min_and_max_op  = 0;
    options.threshold_op    = 0;
    options.sobol_op        = 1;
    options.sobol_order     = 1;

    options.nb_time_steps    = 1; // not used here
    options.nb_parameters    = 3; // number of parameters
    options.size_parameters  = malloc (options.nb_parameters * sizeof(int)); // nb of values taken by each parameter
    for (i=0; i<options.nb_parameters; i++)
        options.size_parameters[i] = i+2;

    // init data
    options.global_vect_size = 5; // size of the global input vector
    data.vect_size        = options.global_vect_size / comm_size;
    if (rank < data.vect_size % comm_size)
        data.vect_size   += 1;
    init_data (&data, &options, data.vect_size);
    print_options (&options);

    // test conditional means //

    tableau = calloc (data.vect_size, sizeof(double));
    parameters = calloc (data.options->nb_parameters, sizeof(int));
    for (i=0; i<data.vect_size; i++)
    {
        tableau[i] = i;
    }

    // incrment means with every possible set of parameters //

    i = 0;
    while (i<data.options->nb_parameters)
    {
        i = 0;
        while (parameters[i] == data.options->size_parameters[i]-1)
        {
            parameters[i] = 0;
            i += 1;
        }
        if (i<data.options->nb_parameters)
            parameters[i] += 1;

        for (j=0; j<data.vect_size; j++)
        {
//            tableau[j] = 10. + j;
            tableau[j] += data.options->nb_parameters;
        }
        increment_conditional_mean (&(data.cond_means[0]), tableau, parameters, &data);
        increment_variance (tableau, &(data.variances[0]), data.vect_size);
    }

    for (i=0; i<data.options->nb_parameters; i++)
    {
        parameters[i] = -1;
    }

    // retrieve and print every computed mean //

    i = 0;
    while (i<data.options->nb_parameters)
    {
        i = 0;
        while (parameters[i] == data.options->size_parameters[i]-1)
        {
            parameters[i] = -1;
            i += 1;
        }
        if (i<data.options->nb_parameters)
            parameters[i] += 1;

        tableau[0] = -10;
        ret = get_conditional_mean (&(data.cond_means[0]), tableau, &incr, parameters, &data);
        if (ret == ERROR_BAD_PARAMETER)
        {
            printf ("problem in get_conditional_mean with parameters {");
            for (j=0; j<data.options->nb_parameters; j++)
                printf (" %d ",  parameters[j]);
            printf ("}\n");
            printf ("on process %d\n",rank);
            continue;
        }
        else if (ret == WARNING_NOTING_RETURNED)
        {
            // It is normal if the number of -1 values in parameters[] is lower than nb_parameters -1
            // or lower than max_order used for init_conditional_means.
            printf ("parameters = {");
            for (j=0; j<data.options->nb_parameters; j++)
                printf (" %d ",  parameters[j]);
            printf ("}\n");
            printf ("  --> no mean returned\n");
        }
        else // if (ret == SUCCESS)
        {
            printf ("parameters = {");
            for (j=0; j<data.options->nb_parameters; j++)
                printf (" %d ",  parameters[j]);
            printf ("}\n");
            printf ("  --> mean = {");
            for (j=0; j<data.vect_size; j++)
                printf (" %g ",  tableau[j]);
            printf ("}\n");
        }
    }

    // fin test conditional means //

    // test sobol indices and conditional variances //
    finalize_stats (&data);

    printf ("\n");
    printf ("Global variances : {");
    for (i=0; i<data.vect_size; i++)
    {
        printf (" %g ",  data.variances[0].variance[i]);
    }
    printf ("}\n");

//    printf ("\n");
//    printf ("Conditional variances (first order) : \n");
//    for (j=0; j<data.options->nb_parameters; j++)
//    {
//        printf ("Var(E(Y|X%d)) = {", j);
//        for (i=0; i<data.vect_size; i++)
//        {
//            printf (" %g ",  data.sobol_indices[0].sobol_indices[j].conditional_variance.variance.variance[i]);
//        }
//        printf ("}\n");
//    }

//    printf ("\n");
//    printf ("Sobol indices (first order) : \n");
//    for (j=0; j<data.options->nb_parameters; j++)
//    {
//        printf ("S%d = {", j);
//        for (i=0; i<data.vect_size; i++)
//        {
//            printf (" %g ",  data.sobol_indices[0].sobol_indices[j].values[i]);
//        }
//        printf ("}\n");
//    }

    {
        int p, t, offset = 0;
    #ifdef BUILD_WITH_MPI
        MPI_File   f;
        MPI_Status status;
    #else // BUILD_WITH_MPI
        FILE* f;
    #endif // BUILD_WITH_MPI
        char       file_name[256];

        for (i=0; i<rank; i++)
        {
            offset += options.global_vect_size / comm_size;
            if (i < data.vect_size % comm_size)
                offset += 1;
        }

        for (t=0; t<options.nb_time_steps; t++)
        {
            for (p=0; p<options.nb_parameters; p++)
            {
                sprintf(file_name, "sobol_indices_%.*d.%d", 1, t+1, p+1);
#ifdef BUILD_WITH_MPI
                MPI_File_open (MPI_COMM_WORLD, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
                MPI_File_write_at (f, offset, data.sobol_indices[t].sobol_martinez[p].first_order_values, data.vect_size, MPI_DOUBLE, &status);
                MPI_File_close (&f);
#else // BUILD_WITH_MPI
                fopen(file_name, "w+");
                for (i=0; i<data.vect_size; i++)
                {
                    fprintf(f, "%f\n", data.sobol_indices[t].sobol_martinez[p].first_order_values[i]);
                }
                fclose(f);
#endif // BUILD_WITH_MPI
            }
        }
    }

    free (tableau);
    free (parameters);
    free_data (&data);
    free (options.size_parameters);

#ifdef BUILD_WITH_MPI
    MPI_Finalize ();
#endif // BUILD_WITH_MPI

    return 0;
}
