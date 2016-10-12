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
 * @param[in] time_step
 * time step of the current simulation
 *
 * @param[in] nb_vect
 * number of input vectors
 *
 * @param[in] **in_vect_tab
 * array of input vectors
 *
 *******************************************************************************/

void compute_stats (stats_data_t  *data,
                    const int      time_step,
                    const int      nb_vect,
                    double       **in_vect_tab)
{
    int     i, j;
    double *in_vect = in_vect_tab[0];
    if (data->is_valid != 1)
    {
        fprintf (stderr, "ERROR: data structure not valid (compute_stats)\n");
        exit (1);
    }

//    if (data->computed[time_step] != 0);
//    {
//        return;
//    }

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
        if (nb_vect != data->options->nb_parameters + 2)
        {
            fprintf (stderr, "ERROR: invalid vector nunber (compute_stats)\n");
            exit (1);
        }
            increment_sobol_martinez (&data->sobol_indices[time_step],
                                      data->options->nb_parameters,
                                      in_vect_tab,
                                      data->vect_size);
    }

//    data->computed[time_step] = 1;
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
                  comm_data_t      *comm_data,
                  int              *local_vect_sizes,
                  char             *field
                  )
{
    long long int        i, t, offset = 0;
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
            sprintf(file_name, "%s_mean_%.*d", field, max_size_time, t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
#else // BUILD_WITH_MPI
            f = fopen(file_name, "w");
#endif // BUILD_WITH_MPI
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (comm_data->rcounts[i] > 0)
                {
#ifdef BUILD_WITH_MPI
                    MPI_File_write_at (f, offset + comm_data->rdispls[i], (*data)[i].means[t].mean, comm_data->rcounts[i], MPI_DOUBLE, &status);
#else // BUILD_WITH_MPI
                    for (j=0; j<comm_data->rcounts[i]; j++)
                    {
                        fprintf(f, "%g\n", (*data)[i].means[t].mean[j]);
                    }
#endif // BUILD_WITH_MPI
                }
            }
#ifdef BUILD_WITH_MPI
            MPI_File_close (&f);
#else // BUILD_WITH_MPI
            fclose(f);
#endif // BUILD_WITH_MPI
        }
    }

    if (options->variance_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "%s_variance_%.*d", field, max_size_time, t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
#else // BUILD_WITH_MPI
            f = fopen(file_name, "w");
#endif // BUILD_WITH_MPI
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (comm_data->rcounts[i] > 0)
                {
#ifdef BUILD_WITH_MPI
                    MPI_File_write_at (f, offset + comm_data->rdispls[i], (*data)[i].variances[t].variance, comm_data->rcounts[i], MPI_DOUBLE, &status);
#else // BUILD_WITH_MPI
                    for (j=0; j<comm_data->rcounts[i]; j++)
                    {
                        fprintf(f, "%g\n", (*data)[i].variances[t].variance[j]);
                    }
#endif // BUILD_WITH_MPI
                }
            }
#ifdef BUILD_WITH_MPI
            MPI_File_close (&f);
#else // BUILD_WITH_MPI
            fclose(f);
#endif // BUILD_WITH_MPI
        }

        if (options->mean_op == 1)
        {
            for (t=0; t<options->nb_time_steps; t++)
            {
                sprintf(file_name, "%s_mean_%.*d", field, max_size_time, t+1);
#ifdef BUILD_WITH_MPI
                MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
#else // BUILD_WITH_MPI
                f = fopen(file_name, "w");
#endif // BUILD_WITH_MPI
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if (comm_data->rcounts[i] > 0)
                    {
#ifdef BUILD_WITH_MPI
                        MPI_File_write_at (f, offset + comm_data->rdispls[i], (*data)[i].variances[t].mean_structure.mean, comm_data->rcounts[i], MPI_DOUBLE, &status);
#else // BUILD_WITH_MPI
                        for (j=0; j<comm_data->rcounts[i]; j++)
                        {
                            fprintf(f, "%g\n", (*data)[i].variances[t].mean_structure.mean[j]);
                        }
#endif // BUILD_WITH_MPI
                    }
                }
#ifdef BUILD_WITH_MPI
                MPI_File_close (&f);
#else // BUILD_WITH_MPI
                fclose(f);
#endif // BUILD_WITH_MPI
            }
        }
    }

    if (options->min_and_max_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "%s_min_%.*d", field, max_size_time, t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
#else // BUILD_WITH_MPI
            f = fopen(file_name, "w");
#endif // BUILD_WITH_MPI
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (comm_data->rcounts[i] > 0)
                {
#ifdef BUILD_WITH_MPI
                    MPI_File_write_at (f, offset + comm_data->rdispls[i], (*data)[i].min_max[t].min, comm_data->rcounts[i], MPI_DOUBLE, &status);
#else // BUILD_WITH_MPI
                    for (j=0; j<comm_data->rcounts[i]; j++)
                    {
                        fprintf(f, "%g\n", (*data)[i].min_max[t].min[j]);
                    }
#endif // BUILD_WITH_MPI
                }
            }
#ifdef BUILD_WITH_MPI
            MPI_File_close (&f);
#else // BUILD_WITH_MPI
            fclose(f);
#endif // BUILD_WITH_MPI
        }

        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "%s_max_%.*d", field, max_size_time, t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
#else // BUILD_WITH_MPI
            f = fopen(file_name, "w");
#endif // BUILD_WITH_MPI
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (comm_data->rcounts[i] > 0)
                {
#ifdef BUILD_WITH_MPI
                    MPI_File_write_at (f, offset + comm_data->rdispls[i], (*data)[i].min_max[t].max, comm_data->rcounts[i], MPI_DOUBLE, &status);
#else // BUILD_WITH_MPI
                    for (j=0; j<comm_data->rcounts[i]; j++)
                    {
                        fprintf(f, "%g\n", (*data)[i].min_max[t].max[j]);
                    }
#endif // BUILD_WITH_MPI
                }
            }
#ifdef BUILD_WITH_MPI
            MPI_File_close (&f);
#else // BUILD_WITH_MPI
            fclose(f);
#endif // BUILD_WITH_MPI
        }
    }

    if (options->threshold_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "%s_threshold_exceedance_%.*d", field, max_size_time, t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
#else // BUILD_WITH_MPI
            f = fopen(file_name, "w");
#endif // BUILD_WITH_MPI
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (comm_data->rcounts[i] > 0)
                {
#ifdef BUILD_WITH_MPI
                    MPI_File_write_at (f, offset + comm_data->rdispls[i], (*data)[i].thresholds[t], comm_data->rcounts[i], MPI_INT, &status);
#else // BUILD_WITH_MPI
                    for (j=0; j<comm_data->rcounts[i]; j++)
                    {
                        fprintf(f, "%g\n", (*data)[i].thresholds[t][j]);
                    }
#endif // BUILD_WITH_MPI
                }
            }
#ifdef BUILD_WITH_MPI
            MPI_File_close (&f);
#else // BUILD_WITH_MPI
            fclose(f);
#endif // BUILD_WITH_MPI
        }
    }

    if (options->sobol_op == 1)
    {
        int p;
        for (t=0; t<options->nb_time_steps; t++)
        {
            for (p=0; p<options->nb_parameters; p++)
            {
                sprintf(file_name, "%s_sobol_indices_%.*d.%d",field,  max_size_time, t+1, p);
#ifdef BUILD_WITH_MPI
                MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
#else // BUILD_WITH_MPI
                f = fopen(file_name, "w");
#endif // BUILD_WITH_MPI
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if (comm_data->rcounts[i] > 0)
                    {
#ifdef BUILD_WITH_MPI
                        MPI_File_write_at (f, offset + comm_data->rdispls[i], (*data)[i].sobol_indices[t].sobol_martinez[p].first_order_values, comm_data->rcounts[i], MPI_INT, &status);
#else // BUILD_WITH_MPI
                        for (j=0; j<comm_data->rcounts[i]; j++)
                        {
                            fprintf(f, "%g\n", (*data)[i].sobol_indices[t].sobol_martinez[p].values[j]);
                        }
#endif // BUILD_WITH_MPI
                    }
                }
#ifdef BUILD_WITH_MPI
                MPI_File_close (&f);
#else // BUILD_WITH_MPI
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

void write_stats_ensight (stats_data_t    **data,
                          stats_options_t  *options,
                          comm_data_t      *comm_data,
                          int              *local_vect_sizes,
                          char             *field)
{
    long int    i, j, t, offset=0, temp_offset=0;
    FILE*       f;
    char        file_name[256];
    int         max_size_time;
    double      time_value = 0;
    float      *s_buffer;
    char        c_buffer[81], c_buffer2[81];
    MPI_Status  status;
    int32_t     n;

    max_size_time=floor(log10(options->nb_time_steps))+1;

    for (i=0; i<comm_data->rank; i++)
    {
        offset += local_vect_sizes[i];
    }

    s_buffer = malloc (options->global_vect_size * sizeof(float));

    if (options->mean_op == 1 && options->variance_op == 0)
    {
        time_value = 0;
        for (t=0; t<options->nb_time_steps; t++)
        {
            time_value += 0.0012;
            sprintf(file_name, "results.%s_mean.%.*d", field, max_size_time, t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (comm_data->rcounts[i] > 0)
                {
                    for (j=0; j<comm_data->rcounts[i]; j++)
                    {
                        s_buffer[j + offset + comm_data->rdispls[i]] = (float)(*data)[i].means[t].mean[j];
                    }
                }
            }
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                temp_offset = 0;
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j];
                    MPI_Recv (&s_buffer[temp_offset], local_vect_sizes[j], MPI_FLOAT, j, j+121, comm_data->comm, &status);
                }
            }
            else
            {
                MPI_Send(&s_buffer[offset], local_vect_sizes[comm_data->rank], MPI_FLOAT, 0, comm_data->rank+121, comm_data->comm);
            }
#endif // BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                f = fopen(file_name, "w");
                sprintf(c_buffer, "%s_mean (time values: %d, %g)", field, t, time_value);
                strncpy(c_buffer2, c_buffer, 80);
                for (i=strlen(c_buffer); i < 80; i++)
                  c_buffer2[i] = ' ';
                c_buffer2[80] = '\0';
                fwrite (c_buffer2, sizeof(char), 80, f);
                n = 1;
                sprintf(c_buffer, "part");
                strncpy(c_buffer2, c_buffer, 80);
                for (i=strlen(c_buffer); i < 80; i++)
                  c_buffer2[i] = ' ';
                c_buffer2[80] = '\0';
                fwrite (c_buffer2, sizeof(char), 80, f);
                fwrite (&n, sizeof(int32_t), 1, f);
                sprintf(c_buffer, "hexa8");
                strncpy(c_buffer2, c_buffer, 80);
                for (i=strlen(c_buffer); i < 80; i++)
                  c_buffer2[i] = ' ';
                c_buffer2[80] = '\0';
                fwrite (c_buffer2, sizeof(char), 80, f);
                fwrite (s_buffer, sizeof(float), options->global_vect_size, f);

                fclose(f);
            }
        }
    }

    if (options->variance_op == 1)
    {
        time_value = 0;
        for (t=0; t<options->nb_time_steps; t++)
        {
            time_value += 0.0012;
            sprintf(file_name, "results.%s_variance.%.*d", field, max_size_time, t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (comm_data->rcounts[i] > 0)
                {
                    for (j=0; j<comm_data->rcounts[i]; j++)
                    {
                        s_buffer[j + offset + comm_data->rdispls[i]] = (float)(*data)[i].variances[t].variance[j];
                    }
                }
            }
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                temp_offset = 0;
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j];
                    MPI_Recv (&s_buffer[temp_offset], local_vect_sizes[j], MPI_FLOAT, j, j+121, comm_data->comm, &status);
                }
            }
            else
            {
                MPI_Send(&s_buffer[offset], local_vect_sizes[comm_data->rank], MPI_FLOAT, 0, comm_data->rank+121, comm_data->comm);
            }
#endif // BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                f = fopen(file_name, "w");
                sprintf(c_buffer, "%s_variance (time values: %d, %g)", field, t, time_value);
                strncpy(c_buffer2, c_buffer, 80);
                for (i=strlen(c_buffer); i < 80; i++)
                  c_buffer2[i] = ' ';
                c_buffer2[80] = '\0';
                fwrite (c_buffer2, sizeof(char), 80, f);
                n = 1;
                sprintf(c_buffer, "part");
                strncpy(c_buffer2, c_buffer, 80);
                for (i=strlen(c_buffer); i < 80; i++)
                  c_buffer2[i] = ' ';
                c_buffer2[80] = '\0';
                fwrite (c_buffer2, sizeof(char), 80, f);
                fwrite (&n, sizeof(int32_t), 1, f);
                sprintf(c_buffer, "hexa8");
                strncpy(c_buffer2, c_buffer, 80);
                for (i=strlen(c_buffer); i < 80; i++)
                  c_buffer2[i] = ' ';
                c_buffer2[80] = '\0';
                fwrite (c_buffer2, sizeof(char), 80, f);
                fwrite (s_buffer, sizeof(float), options->global_vect_size, f);

                fclose(f);
            }
        }

        if (options->mean_op == 1)
        {
            time_value = 0;
            for (t=0; t<options->nb_time_steps; t++)
            {
                time_value += 0.0012;
                sprintf(file_name, "results.%s_mean.%.*d", field, max_size_time, t+1);
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if (comm_data->rcounts[i] > 0)
                    {
                        for (j=0; j<comm_data->rcounts[i]; j++)
                        {
                            s_buffer[j + offset + comm_data->rdispls[i]] = (float)(*data)[i].variances[t].mean_structure.mean[j];
                        }
                    }
                }
    #ifdef BUILD_WITH_MPI
                if (comm_data->rank == 0)
                {
                    temp_offset = 0;
                    for (j=1; j<comm_data->comm_size; j++)
                    {
                        temp_offset += local_vect_sizes[j];
                        MPI_Recv (&s_buffer[temp_offset], local_vect_sizes[j], MPI_FLOAT, j, j+121, comm_data->comm, &status);
                    }
                }
                else
                {
                    MPI_Send(&s_buffer[offset], local_vect_sizes[comm_data->rank], MPI_FLOAT, 0, comm_data->rank+121, comm_data->comm);
                }
    #endif // BUILD_WITH_MPI
                if (comm_data->rank == 0)
                {
                    f = fopen(file_name, "w");
                    sprintf(c_buffer, "%s_mean (time values: %d, %g)", field, t, time_value);
                    strncpy(c_buffer2, c_buffer, 80);
                    for (i=strlen(c_buffer); i < 80; i++)
                      c_buffer2[i] = ' ';
                    c_buffer2[80] = '\0';
                    fwrite (c_buffer2, sizeof(char), 80, f);
                    n = 1;
                    sprintf(c_buffer, "part");
                    strncpy(c_buffer2, c_buffer, 80);
                    for (i=strlen(c_buffer); i < 80; i++)
                      c_buffer2[i] = ' ';
                    c_buffer2[80] = '\0';
                    fwrite (c_buffer2, sizeof(char), 80, f);
                    fwrite (&n, sizeof(int32_t), 1, f);
                    sprintf(c_buffer, "hexa8");
                    strncpy(c_buffer2, c_buffer, 80);
                    for (i=strlen(c_buffer); i < 80; i++)
                      c_buffer2[i] = ' ';
                    c_buffer2[80] = '\0';
                    fwrite (c_buffer2, sizeof(char), 80, f);
                    fwrite (s_buffer, sizeof(float), options->global_vect_size, f);

                    fclose(f);
                }
            }
        }
    }

    if (options->min_and_max_op == 1)
    {
        time_value = 0;
        for (t=0; t<options->nb_time_steps; t++)
        {
            time_value += 0.0012;
            sprintf(file_name, "results.%s_min.%.*d", field, max_size_time, t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (comm_data->rcounts[i] > 0)
                {
                    for (j=0; j<comm_data->rcounts[i]; j++)
                    {
                        s_buffer[j + offset + comm_data->rdispls[i]] = (float)(*data)[i].min_max[t].min[j];
                    }
                }
            }
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                temp_offset = 0;
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j];
                    MPI_Recv (&s_buffer[temp_offset], local_vect_sizes[j], MPI_FLOAT, j, j+121, comm_data->comm, &status);
                }
            }
            else
            {
                MPI_Send(&s_buffer[offset], local_vect_sizes[comm_data->rank], MPI_FLOAT, 0, comm_data->rank+121, comm_data->comm);
            }
#endif // BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                f = fopen(file_name, "w");
                sprintf(c_buffer, "%s_min (time values: %d, %g)", field, t, time_value);
                strncpy(c_buffer2, c_buffer, 80);
                for (i=strlen(c_buffer); i < 80; i++)
                  c_buffer2[i] = ' ';
                c_buffer2[80] = '\0';
                fwrite (c_buffer2, sizeof(char), 80, f);
                n = 1;
                sprintf(c_buffer, "part");
                strncpy(c_buffer2, c_buffer, 80);
                for (i=strlen(c_buffer); i < 80; i++)
                  c_buffer2[i] = ' ';
                c_buffer2[80] = '\0';
                fwrite (c_buffer2, sizeof(char), 80, f);
                fwrite (&n, sizeof(int32_t), 1, f);
                sprintf(c_buffer, "hexa8");
                strncpy(c_buffer2, c_buffer, 80);
                for (i=strlen(c_buffer); i < 80; i++)
                  c_buffer2[i] = ' ';
                c_buffer2[80] = '\0';
                fwrite (c_buffer2, sizeof(char), 80, f);
                fwrite (s_buffer, sizeof(float), options->global_vect_size, f);

                fclose(f);
            }
        }

        time_value = 0;
        for (t=0; t<options->nb_time_steps; t++)
        {
            time_value += 0.0012;
            sprintf(file_name, "results.%s_max.%.*d", field, max_size_time, t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (comm_data->rcounts[i] > 0)
                {
                    for (j=0; j<comm_data->rcounts[i]; j++)
                    {
                        s_buffer[j + offset + comm_data->rdispls[i]] = (float)(*data)[i].min_max[t].max[j];
                    }
                }
            }
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                temp_offset = 0;
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j];
                    MPI_Recv (&s_buffer[temp_offset], local_vect_sizes[j], MPI_FLOAT, j, j+121, comm_data->comm, &status);
                }
            }
            else
            {
                MPI_Send(&s_buffer[offset], local_vect_sizes[comm_data->rank], MPI_FLOAT, 0, comm_data->rank+121, comm_data->comm);
            }
#endif // BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                f = fopen(file_name, "w");
                sprintf(c_buffer, "%s_max (time values: %d, %g)", field, t, time_value);
                strncpy(c_buffer2, c_buffer, 80);
                for (i=strlen(c_buffer); i < 80; i++)
                  c_buffer2[i] = ' ';
                c_buffer2[80] = '\0';
                fwrite (c_buffer2, sizeof(char), 80, f);
                n = 1;
                sprintf(c_buffer, "part");
                strncpy(c_buffer2, c_buffer, 80);
                for (i=strlen(c_buffer); i < 80; i++)
                  c_buffer2[i] = ' ';
                c_buffer2[80] = '\0';
                fwrite (c_buffer2, sizeof(char), 80, f);
                fwrite (&n, sizeof(int32_t), 1, f);
                sprintf(c_buffer, "hexa8");
                strncpy(c_buffer2, c_buffer, 80);
                for (i=strlen(c_buffer); i < 80; i++)
                  c_buffer2[i] = ' ';
                c_buffer2[80] = '\0';
                fwrite (c_buffer2, sizeof(char), 80, f);
                fwrite (s_buffer, sizeof(float), options->global_vect_size, f);

                fclose(f);
            }
        }
    }

    if (options->threshold_op == 1)
    {
        time_value = 0;
        for (t=0; t<options->nb_time_steps; t++)
        {
            time_value += 0.0012;
            sprintf(file_name, "results.%s_min.%.*d", field, max_size_time, t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if (comm_data->rcounts[i] > 0)
                {
                    for (j=0; j<comm_data->rcounts[i]; j++)
                    {
                        s_buffer[j + offset + comm_data->rdispls[i]] = (float)(*data)[i].thresholds[t][j];
                    }
                }
            }
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                temp_offset = 0;
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j];
                    MPI_Recv (&s_buffer[temp_offset], local_vect_sizes[j], MPI_FLOAT, j, j+121, comm_data->comm, &status);
                }
            }
            else
            {
                MPI_Send(&s_buffer[offset], local_vect_sizes[comm_data->rank], MPI_FLOAT, 0, comm_data->rank+121, comm_data->comm);
            }
#endif // BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                f = fopen(file_name, "w");
                sprintf(c_buffer, "%s_min (time values: %d, %g)", field, t, time_value);
                strncpy(c_buffer2, c_buffer, 80);
                for (i=strlen(c_buffer); i < 80; i++)
                  c_buffer2[i] = ' ';
                c_buffer2[80] = '\0';
                fwrite (c_buffer2, sizeof(char), 80, f);
                n = 1;
                sprintf(c_buffer, "part");
                strncpy(c_buffer2, c_buffer, 80);
                for (i=strlen(c_buffer); i < 80; i++)
                  c_buffer2[i] = ' ';
                c_buffer2[80] = '\0';
                fwrite (c_buffer2, sizeof(char), 80, f);
                fwrite (&n, sizeof(int32_t), 1, f);
                sprintf(c_buffer, "hexa8");
                strncpy(c_buffer2, c_buffer, 80);
                for (i=strlen(c_buffer); i < 80; i++)
                  c_buffer2[i] = ' ';
                c_buffer2[80] = '\0';
                fwrite (c_buffer2, sizeof(char), 80, f);
                fwrite (s_buffer, sizeof(float), options->global_vect_size, f);

                fclose(f);
            }
        }
    }

//    if (options->sobol_op == 1)
//    {
//        int p;
//        for (t=0; t<options->nb_time_steps; t++)
//        {
//            for (p=0; p<options->nb_parameters; p++)
//            {
//                sprintf(file_name, "%s_sobol_indices_%.*d.%d",field,  max_size_time, t+1, p);
//#ifdef BUILD_WITH_MPI
//                MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
//#else // BUILD_WITH_MPI
//                f = fopen(file_name, "w");
//#endif // BUILD_WITH_MPI
//                for (i=0; i<comm_data->client_comm_size; i++)
//                {
//                    if (comm_data->rcounts[i] > 0)
//                    {
//#ifdef BUILD_WITH_MPI
//                        MPI_File_write_at (f, offset + comm_data->rdispls[i], (*data)[i].sobol_indices[t].sobol_martinez[p].first_order_values, comm_data->rcounts[i], MPI_INT, &status);
//#else // BUILD_WITH_MPI
//                        for (j=0; j<comm_data->rcounts[i]; j++)
//                        {
//                            fprintf(f, "%g\n", (*data)[i].sobol_indices[t].sobol_martinez[p].values[j]);
//                        }
//#endif // BUILD_WITH_MPI
//                    }
//                }
//#ifdef BUILD_WITH_MPI
//                MPI_File_close (&f);
//#else // BUILD_WITH_MPI
//                fclose(f);
//#endif // BUILD_WITH_MPI
//            }
//        }
//    }
    free (s_buffer);
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

}
