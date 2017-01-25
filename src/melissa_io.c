/**
 *
 * @file melissa_io.c
 * @brief Inputs, outputs and checkpoints.
 * @author Terraz Théophile
 * @date 2017-19-01
 *
 * @defgroup melissa_io input, output and checkpoint functions
 *
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "melissa_data.h"

/**
 *******************************************************************************
 *
 * @ingroup melissa_io
 *
 * This function saves some client data on disc
 *
 *******************************************************************************
 *
 * @param[in] *client_comm_size
 * client MPI communicator size
 *
 * @param[in] *client_vect_sizes
 * client vector sizes
 *
 *******************************************************************************/

void write_client_data (int *client_comm_size,
                        int *client_vect_sizes)
{
    FILE* f;
    int i;

    f = fopen("client_data.save", "wb+");

    fwrite(client_comm_size, sizeof(int), 1, f);
    fwrite(client_vect_sizes, sizeof(int), *client_comm_size, f);

    fclose(f);
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_io
 *
 * This function reads a saved option structure on disc
 *
 *******************************************************************************
 *
 * @param[out] *client_comm_size
 * client MPI communicator size
 *
 * @param[out] **client_vect_sizes
 * client vector sizes
 *
 *******************************************************************************/

int read_client_data (int  *client_comm_size,
                      int **client_vect_sizes)
{
    FILE* f = NULL;
    int ret = 1;

    f = fopen("client_data.save", "rb");

    if (f != NULL)
    {
        if (1 == fread(client_comm_size, sizeof(int), 1, f));
        {
            ret = 0;
        }
        *client_vect_sizes = melissa_malloc (*client_comm_size * sizeof(int));
        if (*client_comm_size == fread(*client_vect_sizes, sizeof(int), *client_comm_size, f));
        {
            ret = 0;
        }
    }

    fclose(f);
    return ret;
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_io
 *
 * This function saves stats on disc
 *
 *******************************************************************************
 *
 * @param[in] *data
 * data structure to save
 *
 * @param[in] *comm_data
 * communication structure
 *
 * @param[in] *field_name
 * name of the field to write
 *
 *******************************************************************************/

void save_stats (melissa_data_t *data,
                 comm_data_t    *comm_data,
                 char           *field_name)
{
    char       file_name[256];
    int        i;
    FILE*      f;

    for (i=0; i<comm_data->client_comm_size; i++)
    {
        if (comm_data->rcounts[i] > 0)
        {
            sprintf(file_name, "%s%d_%d.data", field_name, comm_data->rank, i);
            f = fopen(file_name, "wb+");
            fwrite(&data[i].vect_size, sizeof(int), 1, f);
            if (data[i].options->mean_op != 0 && data[i].options->variance_op == 0)
            {
                write_mean(data[i].means, data[i].vect_size, data[i].options->nb_time_steps, f);
            }
            if (data[i].options->variance_op != 0)
            {
                write_variance(data[i].variances, data[i].vect_size, data[i].options->nb_time_steps, f);
            }
            if (data[i].options->min_and_max_op != 0)
            {
                write_min_max(data[i].min_max, data[i].vect_size, data[i].options->nb_time_steps, f);
            }
            if (data[i].options->threshold_op != 0)
            {
                write_threshold(data[i].thresholds, data[i].vect_size, data[i].options->nb_time_steps, f);
            }
            if (data[i].options->sobol_op != 0)
            {
//                TODO
//                write_sobol(data->thresholds, data->vect_size, data->options->nb_time_steps, f);
            }
            fwrite(data[i].computed, sizeof(int), 1, f);
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_io
 *
 * This function reads stats saved on disc
 *
 *******************************************************************************
 *
 * @param[in] *data
 * data structure to read
 *
 * @param[in] *comm_data
 * communication structure
 *
 * @param[in] *field_name
 * name of the field to read
 *
 * @param[in] client_rank
 * mpi rank of sending client process
 *
 *******************************************************************************/

void read_saved_stats (melissa_data_t *data,
                       comm_data_t    *comm_data,
                       char           *field_name,
                       int             client_rank)
{
    char       file_name[256];
    FILE*      f = NULL;

//    for (i=0; i<comm_data->client_comm_size; i++)
//    {
        if (comm_data->rcounts[client_rank] > 0)
        {
            sprintf(file_name, "%s%d_%d.data", field_name, comm_data->rank, client_rank);
            f = fopen(file_name, "rb");
            if (f == NULL)
            {
              fprintf(stdout,"WARNING: can not open %s%d_%d.data\n", field_name, comm_data->rank, client_rank);
              return;
            }
            fread(&data[client_rank].vect_size, sizeof(int), 1, f);
            if (data[client_rank].options->mean_op != 0 && data[client_rank].options->variance_op == 0)
            {
                read_mean(data[client_rank].means, data[client_rank].vect_size, data[client_rank].options->nb_time_steps, f);
            }
            if (data[client_rank].options->variance_op != 0)
            {
                read_variance(data[client_rank].variances, data[client_rank].vect_size, data[client_rank].options->nb_time_steps, f);
            }
            if (data[client_rank].options->min_and_max_op != 0)
            {
                read_min_max(data[client_rank].min_max, data[client_rank].vect_size, data[client_rank].options->nb_time_steps, f);
            }
            if (data[client_rank].options->threshold_op != 0)
            {
                read_threshold(data[client_rank].thresholds, data[client_rank].vect_size, data[client_rank].options->nb_time_steps, f);
            }
            if (data[client_rank].options->sobol_op != 0)
            {
//                TODO
//                read_sobol(data->thresholds, data->vect_size, data->options->nb_time_steps, f);
            }
//            fread(&data[client_rank].computed, sizeof(int), 1, f);
        }
//    }
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_io
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

void write_stats (melissa_data_t    **data,
                  melissa_options_t  *options,
                  comm_data_t        *comm_data,
                  int                *local_vect_sizes,
                  char               *field
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
 * @ingroup melissa_io
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
 * @param[in] *comm_data
 * structure containing communications parameters
 *
 * @param[in] *local_vect_sizes
 * all local vector sises
 *
 * @param[in] *field
 * name of the field on which are computed the statistics
 *
 *******************************************************************************/

void write_stats_ensight (melissa_data_t    **data,
                          melissa_options_t  *options,
                          comm_data_t        *comm_data,
                          int                *local_vect_sizes,
                          char               *field)
{
    long int    i, j, t, param, offset=0, temp_offset=0;
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

    s_buffer = melissa_malloc (options->global_vect_size * sizeof(float));

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

    if (options->sobol_op == 1)
    {
        for (param=0; param<options->nb_parameters; param++)
        {
            time_value = 0;
            for (t=0; t<options->nb_time_steps; t++)
            {
                time_value += 0.0012;
                sprintf(file_name, "results.%s_sobol%d.%.*d", field, param, max_size_time, t+1);
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if (comm_data->rcounts[i] > 0)
                    {
                        for (j=0; j<comm_data->rcounts[i]; j++)
                        {
                            s_buffer[j + offset + comm_data->rdispls[i]] = (float)(*data)[i].sobol_indices[t].sobol_martinez[param].first_order_values[j];
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
                    sprintf(c_buffer, "%s_sobol%d (time values: %d, %g)", field, param, t, time_value);
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
        for (param=0; param<options->nb_parameters; param++)
        {
            time_value = 0;
            for (t=0; t<options->nb_time_steps; t++)
            {
                time_value += 0.0012;
                sprintf(file_name, "results.%s_sobol_tot%d.%.*d", field, param, max_size_time, t+1);
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if (comm_data->rcounts[i] > 0)
                    {
                        for (j=0; j<comm_data->rcounts[i]; j++)
                        {
                            s_buffer[j + offset + comm_data->rdispls[i]] = (float)(*data)[i].sobol_indices[t].sobol_martinez[param].total_order_values[j];
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
                    sprintf(c_buffer, "%s_sobol_tot%d (time values: %d, %g)", field, param, t, time_value);
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
    melissa_free (s_buffer);
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_io
 *
 * This function reads data from Ensight files
 *
 *******************************************************************************
 *
 * @param[in] *options
 * option structure
 *
 * @param[in] *comm_data
 * structure containing communications parameters
 *
 * @param[in] *in_vect
 * input vector
 *
 * @param[in] *local_vect_sizes
 * all local vector sises
 *
 * @param[in] *file_name
 * name of the file
 *
 *******************************************************************************/

void read_ensight (melissa_options_t  *options,
                   comm_data_t      *comm_data,
                   double           *in_vect,
                   int              *local_vect_sizes,
                   char             *file_name)
{
    long int    i, j, offset=0;
    FILE*       f;
    float      *r_buffer;
    char        c_buffer[81];
    MPI_Status  status;
    int32_t     n;

    if (comm_data->rank == 0)
    {
        r_buffer = melissa_malloc (options->global_vect_size * sizeof(float));
    }
    else
    {
        r_buffer = melissa_malloc (local_vect_sizes[comm_data->rank] * sizeof(float));
    }

    if (comm_data->rank == 0)
    {
        f = fopen(file_name, "r");
        fread (c_buffer, sizeof(char), 80, f);
        fread (c_buffer, sizeof(char), 80, f);
        fread (&n, sizeof(int32_t), 1, f);
        fread (c_buffer, sizeof(char), 80, f);
        fread (r_buffer, sizeof(float), options->global_vect_size, f);
        fclose(f);
    }
#ifdef BUILD_WITH_MPI
    if (comm_data->rank == 0)
    {
        offset = 0;
        for (j=1; j<comm_data->comm_size; j++)
        {
            offset += local_vect_sizes[j];
            MPI_Send (&r_buffer[offset], local_vect_sizes[j], MPI_FLOAT, j, j+121, comm_data->comm);
        }
    }
    else
    {
        MPI_Recv(r_buffer, local_vect_sizes[comm_data->rank], MPI_FLOAT, 0, comm_data->rank+121, comm_data->comm, &status);
    }

    for (i=0; i<local_vect_sizes[comm_data->rank]; i++)
    {
        in_vect[i] = (double)r_buffer[i];
    }
    melissa_free (r_buffer);
#endif // BUILD_WITH_MPI
}