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

#include <melissa/server/fault_tolerance.h>
#include <melissa/server/output.h>
#include <melissa/utils.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>

static inline void dgather_data(comm_data_t *comm_data,
                               int *local_vect_sizes,
                               double *d_buffer)
{
    int temp_offset = 0;
    int j;
    MPI_Status  status;
    if (comm_data->rank == 0)
    {
        for (j=1; j<comm_data->comm_size; j++)
        {
            temp_offset += local_vect_sizes[j-1];
            if (local_vect_sizes[j] > 0)
            {
                int result = MPI_Recv (&d_buffer[temp_offset], local_vect_sizes[j], MPI_DOUBLE, j, j+121, comm_data->comm, &status);
                if (result != MPI_SUCCESS)
                {
                  printf("MPI_ERROR_CODE: %d\n", result);
                }
            }
        }
        temp_offset = 0;
    }
    else
    {
        if (local_vect_sizes[comm_data->rank] > 0)
        {
            int result = MPI_Send(d_buffer, local_vect_sizes[comm_data->rank], MPI_DOUBLE, 0, comm_data->rank+121, comm_data->comm);
            if (result != MPI_SUCCESS)
            {
              printf("MPI_ERROR_CODE: %d\n", result);
            }
        }
    }
}

static inline void igather_data(comm_data_t *comm_data,
                               int *local_vect_sizes,
                               int *i_buffer)
{
    int temp_offset = 0;
    int j;
    MPI_Status  status;
    if (comm_data->rank == 0)
    {
        for (j=1; j<comm_data->comm_size; j++)
        {
            temp_offset += local_vect_sizes[j-1];
            if (local_vect_sizes[j] > 0)
            {
                MPI_Recv (&i_buffer[temp_offset], local_vect_sizes[j], MPI_INT, j, j+121, comm_data->comm, &status);
            }
        }
        temp_offset = 0;
    }
    else
    {
        if (local_vect_sizes[comm_data->rank] > 0)
        {
            MPI_Send(i_buffer, local_vect_sizes[comm_data->rank], MPI_INT, 0, comm_data->rank+121, comm_data->comm);
        }
    }
}

/**
 * This function writes the computed statistics on files
 *
 * @param[in] **data
 * pointer to the array of structures containing statistics data
 *
 * @param[in] *options
 * Melissa option structure
 *
 * @param[in] comm_data
 * structure containing communications parameters
 *
 * @param[in] *field
 * name of the statistic field to write
 *
 */

void melissa_write_stats_seq(melissa_data_t    **data,
                             melissa_options_t  *options,
                             comm_data_t        *comm_data,
                             const char         *field)
{
    long int    temp_offset=0;
    long int    i;
    int        *offsets;
    int         t, p;
    char        file_name[256];
    int         max_size_time;
    int         vect_size = 0;
    int        *local_vect_sizes;
    int         global_vect_size = 0;
    int        *i_buffer;
    double     *d_buffer;
//    double temp1, temp2;

    max_size_time=floor(log10(options->nb_time_steps))+1;

    // ================= first test =============== //
    //                                              //
    // Communicate local vect size to every process //
    //                                              //
    // ============================================ //

    local_vect_sizes = melissa_calloc (comm_data->comm_size, sizeof(int));
    offsets = melissa_calloc (comm_data->comm_size, sizeof(int));
    for (i=0; i<comm_data->client_comm_size; i++)
    {
        vect_size += (*data)[i].vect_size;
    }
    MPI_Allgather (&vect_size, 1, MPI_INT, local_vect_sizes, 1, MPI_INT, comm_data->comm);

    for (i=0; i<comm_data->comm_size-1; i++)
    {
        offsets[i+1] = offsets[i] + local_vect_sizes[i];
        global_vect_size += local_vect_sizes[i];
    }
    global_vect_size += local_vect_sizes[comm_data->comm_size-1];

    // ============================================ //

    d_buffer = melissa_malloc (global_vect_size * sizeof(double));

    if (options->mean_op == 1)
    {
        MPI_Barrier(comm_data->comm);
        for (t=0; t<options->nb_time_steps; t++)
        {
            snprintf(file_name, sizeof(file_name), "results.%s_mean.%.*d", field, max_size_time, t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    memcpy(&d_buffer[temp_offset], (*data)[i].moments[t].m1, (*data)[i].vect_size*sizeof(double));
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
            dgather_data (comm_data, local_vect_sizes, d_buffer);
            if (comm_data->rank == 0)
            {
                (*write_output_d)(file_name,
                                  field,
                                  "mean",
                                  t,
                                  global_vect_size,
                                  d_buffer);
            }
        }
    }

    if (options->variance_op == 1)
    {
        MPI_Barrier(comm_data->comm);
        for (t=0; t<options->nb_time_steps; t++)
        {
            snprintf(file_name, sizeof(file_name), "results.%s_variance.%.*d", field, max_size_time, t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    memcpy(&d_buffer[temp_offset], (*data)[i].moments[t].theta2, (*data)[i].vect_size*sizeof(double));
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
            dgather_data (comm_data, local_vect_sizes, d_buffer);
            if (comm_data->rank == 0)
            {
                (*write_output_d)(file_name,
                                  field,
                                  "variance",
                                  t,
                                  global_vect_size,
                                  d_buffer);
            }
        }
    }

    if (options->skewness_op == 1)
    {
        MPI_Barrier(comm_data->comm);
        for (t=0; t<options->nb_time_steps; t++)
        {
            snprintf(file_name, sizeof(file_name), "results.%s_skewness.%.*d", field, max_size_time, t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    compute_skewness (&(*data)[i].moments[t], &d_buffer[temp_offset], (*data)[i].vect_size);
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
            dgather_data (comm_data, local_vect_sizes, d_buffer);
            if (comm_data->rank == 0)
            {
                (*write_output_d)(file_name,
                                  field,
                                  "skewness",
                                  t,
                                  global_vect_size,
                                  d_buffer);
            }
        }
    }

    if (options->kurtosis_op == 1)
    {
        MPI_Barrier(comm_data->comm);
        for (t=0; t<options->nb_time_steps; t++)
        {
            snprintf(file_name, sizeof(file_name), "results.%s_kurtosis.%.*d", field, max_size_time, t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    compute_kurtosis (&(*data)[i].moments[t], &d_buffer[temp_offset], (*data)[i].vect_size);
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
            dgather_data (comm_data, local_vect_sizes, d_buffer);
            if (comm_data->rank == 0)
            {
                (*write_output_d)(file_name,
                                  field,
                                  "kurtosis",
                                  t,
                                  global_vect_size,
                                  d_buffer);
            }
        }
    }

    if (options->min_and_max_op == 1)
    {
        MPI_Barrier(comm_data->comm);
        for (t=0; t<options->nb_time_steps; t++)
        {
            snprintf(file_name, sizeof(file_name), "results.%s_min.%.*d", field, max_size_time, t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    memcpy(&d_buffer[temp_offset], (*data)[i].min_max[t].min, (*data)[i].vect_size*sizeof(double));
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
            dgather_data (comm_data, local_vect_sizes, d_buffer);
            if (comm_data->rank == 0)
            {
                (*write_output_d)(file_name,
                                  field,
                                  "min",
                                  t,
                                  global_vect_size,
                                  d_buffer);
            }
        }

        MPI_Barrier(comm_data->comm);
        i_buffer = (int*)d_buffer;
        for (t=0; t<options->nb_time_steps; t++)
        {
            snprintf(file_name, sizeof(file_name), "results.%s_min_id.%.*d", field, max_size_time, t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    memcpy(&i_buffer[temp_offset], (*data)[i].min_max[t].min_id, (*data)[i].vect_size*sizeof(int));
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
            igather_data (comm_data, local_vect_sizes, i_buffer);
            if (comm_data->rank == 0)
            {
                (*write_output_i)(file_name,
                                  field,
                                  "min_id",
                                  t,
                                  global_vect_size,
                                  i_buffer);
            }
        }

        MPI_Barrier(comm_data->comm);
        for (t=0; t<options->nb_time_steps; t++)
        {
            snprintf(file_name, sizeof(file_name), "results.%s_max.%.*d", field, max_size_time, t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    memcpy(&d_buffer[temp_offset], (*data)[i].min_max[t].max, (*data)[i].vect_size*sizeof(double));
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
            dgather_data (comm_data, local_vect_sizes, d_buffer);
            if (comm_data->rank == 0)
            {
                (*write_output_d)(file_name,
                                  field,
                                  "max",
                                  t,
                                  global_vect_size,
                                  d_buffer);
            }
        }

        MPI_Barrier(comm_data->comm);
        i_buffer = (int*)d_buffer;
        for (t=0; t<options->nb_time_steps; t++)
        {
            snprintf(file_name, sizeof(file_name), "results.%s_max_id.%.*d", field, max_size_time, t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    memcpy(&i_buffer[temp_offset], (*data)[i].min_max[t].max_id, (*data)[i].vect_size*sizeof(int));
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
            igather_data (comm_data, local_vect_sizes, i_buffer);
            if (comm_data->rank == 0)
            {
                (*write_output_i)(file_name,
                                  field,
                                  "max_id",
                                  t,
                                  global_vect_size,
                                  i_buffer);
            }
        }
    }

    if (options->threshold_op == 1)
    {
        MPI_Barrier(comm_data->comm);
        i_buffer = (int*)d_buffer;
        int value;
        for (value=0; value<options->nb_thresholds; value++)
        {
            for (t=0; t<options->nb_time_steps; t++)
            {
                snprintf(file_name, sizeof(file_name), "results.%s_threshold%g.%.*d", field, options->threshold[value], max_size_time, t+1);
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
                        memcpy(&i_buffer[temp_offset], (*data)[i].thresholds[t][value].threshold_exceedance, (*data)[i].vect_size*sizeof(int));
                        temp_offset += (*data)[i].vect_size;
                    }
                }
                temp_offset = 0;
                igather_data (comm_data, local_vect_sizes, i_buffer);
                if (comm_data->rank == 0)
                {
                    (*write_output_i)(file_name,
                                      field,
                                      "threshold",
                                      t,
                                      global_vect_size,
                                      i_buffer);
                }
            }
        }
    }

    if (options->quantile_op == 1)
    {
        MPI_Barrier(comm_data->comm);
        int value;
        for (value=0; value<options->nb_quantiles; value++)
        {
            for (t=0; t<options->nb_time_steps; t++)
            {
                snprintf(file_name, sizeof(file_name), "results.%s_quantile%g.%.*d", field, options->quantile_order[value], max_size_time, t+1);
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
                        memcpy(&d_buffer[temp_offset], (*data)[i].quantiles[t][value].quantile, (*data)[i].vect_size*sizeof(double));
                        temp_offset += (*data)[i].vect_size;
                    }
                }
                temp_offset = 0;
                dgather_data (comm_data, local_vect_sizes, d_buffer);
                if (comm_data->rank == 0)
                {
                  char statistics_name[256];
                  sprintf(statistics_name, "quantile%g", options->quantile_order[value]);
                    (*write_output_d)(file_name,
                                      field,
                                      statistics_name,
                                      t,
                                      global_vect_size,
                                      d_buffer);
                }
            }
        }
    }

    if (options->sobol_op == 1)
    {
        MPI_Barrier(comm_data->comm);
        for (p=0; p<options->nb_parameters; p++)
        {
            for (t=0; t<options->nb_time_steps; t++)
            {
                snprintf(file_name, sizeof(file_name), "results.%s_sobol%d.%.*d", field, p, max_size_time, (int)(t+1));
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
                        memcpy(&d_buffer[temp_offset], (*data)[i].sobol_indices[t].sobol_martinez[p].first_order_values, (*data)[i].vect_size*sizeof(double));
                        temp_offset += (*data)[i].vect_size;
                    }
                }
                temp_offset = 0;
                dgather_data (comm_data, local_vect_sizes, d_buffer);
                if (comm_data->rank == 0)
                {
                    char statistics_name[256];
                    sprintf(statistics_name, "sobol%d", p);
                    (*write_output_d)(file_name,
                                      field,
                                      statistics_name,
                                      t,
                                      global_vect_size,
                                      d_buffer);
                }
            }
        }
        for (p=0; p<options->nb_parameters; p++)
        {
            for (t=0; t<options->nb_time_steps; t++)
            {
                snprintf(file_name, sizeof(file_name), "results.%s_sobol_tot%d.%.*d", field, p, max_size_time, (int)(t+1));
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
                        memcpy(&d_buffer[temp_offset], (*data)[i].sobol_indices[t].sobol_martinez[p].total_order_values, (*data)[i].vect_size*sizeof(double));
                        temp_offset += (*data)[i].vect_size;
                    }
                }
                temp_offset = 0;
                dgather_data (comm_data, local_vect_sizes, d_buffer);
                if (comm_data->rank == 0)
                {
                    char statistics_name[256];
                    sprintf(statistics_name, "sobol_tot%d", p);
                    (*write_output_d)(file_name,
                                      field,
                                      statistics_name,
                                      t,
                                      global_vect_size,
                                      d_buffer);
                }
            }
        }
    }

    // looks like we have mpi buffer overflows if we are on large scale...
    // The slave processes (rank != 0) quit already while the master is still writing.
    // This leads to a deadlock in MPI_Recv..
    // so lets sync here.
    // REM: if this happens again sync in each statistic ;)
    // -> it happend again. so now their are many many MPI_Barriers
    MPI_Barrier(comm_data->comm);

    melissa_free (d_buffer);
    melissa_free (offsets);
    melissa_free (local_vect_sizes);
}

/**
 * This function writes the simu_id and the corresponding parameter set
 *
 * @param[out] simulations The list of running simulations
 * @param[out] nb_parameters the number of simulation parameters
 */

void write_simu_param (vector_t *simulations,
                       int       nb_parameters)
{
    const char            file_name[] = "simu_param.txt";
    FILE*                 f = NULL;
    int                   i, j;
    melissa_simulation_t *simu_ptr;

    melissa_print (VERBOSE_DEBUG, "Write simulation parameters in %s (write_simu_param)\n", file_name);
    f = fopen(file_name, "w");
    if (f == NULL)
    {
      melissa_print (VERBOSE_WARNING, "Can not open %s (write_simu_param)\n", file_name);
      return;
    }

    for (i=0; i<simulations->size; i++)
    {
        simu_ptr = simulations->items[i];
        fprintf (f, "%d ", i);
        for (j=0; j<nb_parameters; j++)
        {
            fprintf (f, "%g ", simu_ptr->parameters[j]);
        }
        fprintf (f, "\n");
    }
    fclose(f);
}
