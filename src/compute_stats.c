/**
 *
 * @file compute_stats.c
 * @brief Functions called by the user.
 * @author Terraz Théophile
 * @date 2016-15-03
 *
 * @defgroup intern_API internal API
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stats.h"

/**
 *******************************************************************************
 *
 * @ingroup intern_API
 *
 * This function updates the statistics stored in the data structure
 *
 *******************************************************************************
 *
 * @param[in] *data
 * pointer to the structure containing global parameters
 *
 * @param[in] *parameters
 * parameter array for the current simulation
 *
 * @param[in] *in_vect
 * input vector
 *
 * @param[in] time_step
 * time step of the current simulation
 *
 *******************************************************************************/

void compute_stats (stats_data_t *data,
                    const int    *parameters,
                    double       *in_vect,
                    const int     time_step)
{
    if (data->is_valid != 1)
    {
        fprintf (stderr, "ERROR: data structure not valid (compute_stats)\n");
        exit (1);
    }

    if (data->options->mean_op == 1 && data->options->variance_op == 0)
    {
        increment_mean (in_vect, &(data->means[time_step]), data->vect_size);
    }

    if (data->options->variance_op == 1)
    {
        increment_mean_and_variance (in_vect, &(data->variances[time_step]), data->vect_size);
    }

    if (data->options->min_and_max_op == 1)
    {
        min_and_max (in_vect, &(data->min_max[time_step]), data->vect_size);
    }

    if (data->options->threshold_op == 1)
    {
        update_threshold_exceedance (in_vect, data->thresholds[time_step], data->options->threshold, data->vect_size);
    }

    if (data->options->sobol_op == 1)
    {
        increment_conditional_mean (&(data->cond_means[time_step]), in_vect, parameters, data);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup intern_API
 *
 * This function writes the computed statistics on files
 *
 *******************************************************************************
 *
 * @param[in] **data
 * pointer to the array of structures containing statistics data
 *
 * @param[in] *options
 * option structure
 *
 * @param[in] *in_vect
 * buffer of size local_vect_sizes[comm_data->rank]
 *
 * @param[in] comm_data
 * structure containing communications parameters
 *
 * @param[in] *local_vect_sizes
 * all local vector sises
 *
 * @param[in] *field
 * name of the field on which are computed the statistics
 *
 *******************************************************************************/

void write_stats (stats_data_t    **data,
                  stats_options_t  *options,
                  double           *in_vect,
                  comm_data_t      *comm_data,
                  int              *local_vect_sizes,
                  char             *field
                  )
{
    int        i, t, offset = 0;
#ifdef BUILD_WITH_MPI
    MPI_File   f;
    MPI_Status status;
#else // BUILD_WITH_MPI
    FILE* f;
#endif // BUILD_WITH_MPI
    char       file_name[256];
    int        max_size_time;

    max_size_time=floor(log10(options->nb_time_steps))+1;

    for (i=0; i<comm_data->rank; i++)
    {
        offset += local_vect_sizes[i];
    }

    if (options->mean_op == 1 && options->variance_op == 0)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (comm_data->rcounts[i] > 0)
                {
                    memcpy(&in_vect[comm_data->rdispls[i]], (*data)[i].means[t].mean, comm_data->rcounts[i] * sizeof(double));
                }
            }
            sprintf(file_name, "%s_mean_%.*d", field, max_size_time, t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
            MPI_File_write_at (f, offset, in_vect, local_vect_sizes[comm_data->rank], MPI_DOUBLE, &status);
            MPI_File_close (&f);
#else // BUILD_WITH_MPI
            f = fopen(file_name, "w");
            for (i=0; i<local_vect_sizes[0]; i++)
            {
                fprintf(f, "%g\n", in_vect[i]);
            }
            fclose(f);
#endif // BUILD_WITH_MPI
        }
    }

    if (options->variance_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (comm_data->rcounts[i] > 0)
                {
                    memcpy(&in_vect[comm_data->rdispls[i]], (*data)[i].variances[t].variance, comm_data->rcounts[i] * sizeof(double));
                }
            }
            sprintf(file_name, "%s_variance_%.*d", field, max_size_time, t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
            MPI_File_write_at (f, offset, in_vect, local_vect_sizes[comm_data->rank], MPI_DOUBLE, &status);
            MPI_File_close (&f);
#else // BUILD_WITH_MPI
            f = fopen(file_name, "w");
            for (i=0; i<local_vect_sizes[0]; i++)
            {
                fprintf(f, "%g\n", in_vect[i]);
            }
            fclose(f);
#endif // BUILD_WITH_MPI
        }

        if (options->mean_op == 1)
        {
            for (t=0; t<options->nb_time_steps; t++)
            {
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if (comm_data->rcounts[i] > 0)
                    {
                        memcpy(&in_vect[comm_data->rdispls[i]], (*data)[i].variances[t].mean_structure.mean, comm_data->rcounts[i] * sizeof(double));
                    }
                }
                sprintf(file_name, "%s_mean_%.*d", field, max_size_time, t+1);
#ifdef BUILD_WITH_MPI
                MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
                MPI_File_write_at (f, offset, in_vect, local_vect_sizes[comm_data->rank], MPI_DOUBLE, &status);
                MPI_File_close (&f);
#else // BUILD_WITH_MPI
                f = fopen(file_name, "w");
                for (i=0; i<local_vect_sizes[0]; i++)
                {
                    fprintf(f, "%g\n", in_vect[i]);
                }
                fclose(f);
#endif // BUILD_WITH_MPI
            }
        }
    }

    if (options->min_and_max_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (comm_data->rcounts[i] > 0)
                {
                    memcpy(&in_vect[comm_data->rdispls[i]], (*data)[i].min_max[t].min, comm_data->rcounts[i] * sizeof(double));
                }
            }
            sprintf(file_name, "%s_min_%.*d", field, max_size_time, t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
            MPI_File_write_at (f, offset, in_vect, local_vect_sizes[comm_data->rank], MPI_DOUBLE, &status);
            MPI_File_close (&f);
#else // BUILD_WITH_MPI
            f = fopen(file_name, "w");
            for (i=0; i<local_vect_sizes[0]; i++)
            {
                fprintf(f, "%g\n", in_vect[i]);
            }
            fclose(f);
#endif // BUILD_WITH_MPI
        }

        for (t=0; t<options->nb_time_steps; t++)
        {
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (comm_data->rcounts[i] > 0)
                {
                    memcpy(&in_vect[comm_data->rdispls[i]], (*data)[i].min_max[t].max, comm_data->rcounts[i] * sizeof(double));
                }
            }
            sprintf(file_name, "%s_max_%.*d", field, max_size_time, t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
            MPI_File_write_at (f, offset, in_vect, local_vect_sizes[comm_data->rank], MPI_DOUBLE, &status);
            MPI_File_close (&f);
#else // BUILD_WITH_MPI
            f = fopen(file_name, "w");
            for (i=0; i<local_vect_sizes[0]; i++)
            {
                fprintf(f, "%g\n", in_vect[i]);
            }
            fclose(f);
#endif // BUILD_WITH_MPI
        }
    }

    if (options->threshold_op == 1)
    {
        int *int_vect = malloc (local_vect_sizes[comm_data->rank] * sizeof(int));
        for (t=0; t<options->nb_time_steps; t++)
        {
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (comm_data->rcounts[i] > 0)
                {
                    memcpy(&int_vect[comm_data->rdispls[i]], (*data)[i].thresholds[t], comm_data->rcounts[i] * sizeof(int));
                }
            }
            sprintf(file_name, "%s_threshold_exceedance_%.*d", field, max_size_time, t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
            MPI_File_write_at (f, offset, int_vect, local_vect_sizes[comm_data->rank], MPI_INT, &status);
            MPI_File_close (&f);
#else // BUILD_WITH_MPI
            f = fopen(file_name, "w");
            for (i=0; i<local_vect_sizes[0]; i++)
            {
                fprintf(f, "%d\n", int_vect[i]);
            }
            fclose(f);
#endif // BUILD_WITH_MPI
        }
        free (int_vect);
    }

    if (options->sobol_op == 1)
    {
        int p;
        for (t=0; t<options->nb_time_steps; t++)
        {
            for (p=0; p<options->nb_parameters; p++)
            {
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if (comm_data->rcounts[i] > 0)
                    {
                        memcpy(&in_vect[comm_data->rdispls[i]], (*data)[i].sobol_indices[t].sobol_indices[p].values, comm_data->rcounts[i] * sizeof(double));
                    }
                }
                sprintf(file_name, "%s_sobol_indices_%.*d.%d",field,  max_size_time, t+1, p);
#ifdef BUILD_WITH_MPI
                MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
                MPI_File_write_at (f, offset, in_vect, local_vect_sizes[comm_data->rank], MPI_DOUBLE, &status);
                MPI_File_close (&f);
#else // BUILD_WITH_MPI
                f = fopen(file_name, "w");
                for (i=0; i<local_vect_sizes[0]; i++)
                {
                    fprintf(f, "%g\n", in_vect[i]);
                }
                fclose(f);
#endif // BUILD_WITH_MPI
            }
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup intern_API
 *
 * This function finalize the statistics stored in the data structure
 *
 *******************************************************************************
 *
 * @param[in] *data
 * pointer to the structure containing global parameters
 *
 *******************************************************************************/

void finalize_stats (stats_data_t *data)
{
    int i, j;

    if (data->is_valid != 1)
    {
        fprintf (stderr, "WARNING: data structure not valid (finalize_stats)\n");
        return;
    }

    if (data->options->sobol_op == 1)
    {
        int *indices_to_fix;

        if (data->options->sobol_order > 1)
            fprintf (stdout, "WARNING: sobol indices of order greater than one are not implemented (yet)\n");

        data->sobol_indices = malloc (data->options->nb_time_steps * sizeof(sobol_array_t));
        indices_to_fix = malloc (data->options->nb_parameters * sizeof(int));

#pragma omp parallel for
        for (i=0; i<data->options->nb_time_steps; i++)
        {
            data->sobol_indices[i].sobol_indices = malloc (data->options->nb_parameters * sizeof(sobol_index_t));
        }
        // initialization
        for (j=0; j<data->options->nb_parameters; j++)
        {
            for (i=0; i<data->options->nb_parameters; i++)
                indices_to_fix[i] = 0;
            indices_to_fix[j] = 1;
#pragma omp parallel for
            for (i=0; i<data->options->nb_time_steps; i++)
                init_sobol_index (&(data->sobol_indices[i].sobol_indices[j]), indices_to_fix, data);
        }

        // computation
        for (i=0; i<data->options->nb_time_steps; i++)
        {
            for (j=0; j<data->options->nb_parameters; j++)
            {
                compute_sobol_index (&(data->sobol_indices[i].sobol_indices[j]), data, i);
            }
        }

        free (indices_to_fix);
    }
}
