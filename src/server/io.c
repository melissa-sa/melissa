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

#include <melissa/server/data.h>
#include <melissa/server/fault_tolerance.h>
#include <melissa/utils.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <mpi.h>

/**
 * This function saves stats on disc
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
 */

void save_stats (melissa_data_t *data,
                 comm_data_t    *comm_data,
                 char           *field_name)
{
    char       file_name[320];
    int        i, j;
    FILE*      f = NULL;

    for (i=0; i<comm_data->client_comm_size; i++)
    {
        snprintf(file_name, sizeof(file_name), "%s%d_%d.data", field_name, comm_data->rank, i);
        f = fopen(file_name, "wb+");
        if (f == NULL)
        {
            melissa_print (VERBOSE_ERROR, "save stats", "ERROR: Can not open %s%d_%d.data (save_stats)\n", field_name, comm_data->rank, i);
            return;
        }
        fwrite(&data[i].vect_size, sizeof(int), 1, f);
        if (data[i].vect_size > 0)
        {
//            if (data[i].options->mean_op != 0 && data[i].options->variance_op == 0)
//            {
//                save_mean(data[i].means, data[i].vect_size, data[i].options->nb_time_steps, f);
//            }
//            if (data[i].options->variance_op != 0)
//            {
//                save_variance(data[i].variances, data[i].vect_size, data[i].options->nb_time_steps, f);
//            }
            melissa_print (VERBOSE_DEBUG, "Save moments (field %s, server rank %d, client rank %d) (save_stats)\n", field_name, comm_data->rank, i);
            save_moments(data[i].moments, data[i].vect_size, data[i].options->nb_time_steps, f);
            if (data[i].options->min_and_max_op != 0)
            {
                melissa_print (VERBOSE_DEBUG, "Save min and max (field %s, server rank %d, client rank %d) (save_stats)\n", field_name, comm_data->rank, i);
                save_min_max(data[i].min_max, data[i].vect_size, data[i].options->nb_time_steps, f);
            }
            if (data[i].options->threshold_op != 0)
            {
                melissa_print (VERBOSE_DEBUG, "Save threshold exceedances (field %s, server rank %d, client rank %d) (save_stats)\n", field_name, comm_data->rank, i);
                save_threshold(data[i].thresholds, data[i].vect_size, data[i].options->nb_time_steps, data[i].options->nb_thresholds, f);
            }
            if (data[i].options->quantile_op != 0)
            {
                melissa_print (VERBOSE_DEBUG, "Save quantiles (field %s, server rank %d, client rank %d) (save_stats)\n", field_name, comm_data->rank, i);
                save_quantile(data[i].quantiles, data[i].vect_size, data[i].options->nb_time_steps, data[i].options->nb_quantiles, f);
            }
            if (data[i].options->sobol_op != 0)
            {
                melissa_print (VERBOSE_DEBUG, "Save Sobol indices (field %s, server rank %d, client rank %d) (save_stats)\n", field_name, comm_data->rank, i);
                data[i].save_sobol(data[i].sobol_indices, data[i].vect_size, data[i].options->nb_time_steps, data[i].options->nb_parameters, f);
            }
            melissa_print (VERBOSE_DEBUG, "Save simulation steps (field %s, server rank %d, client rank %d) (save_stats)\n", field_name, comm_data->rank, i);
            fwrite(&data[i].step_simu.size, sizeof(int), 1, f);
            for (j=0; j<data[i].step_simu.size; j++)
            {
                fwrite(data[i].step_simu.items[j], sizeof(int32_t), (data[i].options->nb_time_steps+31)/32, f);
            }
        }
        fclose(f);
    }
}

/**
 * This function reads stats saved on disc
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
 */

void read_saved_stats (melissa_data_t *data,
                       comm_data_t    *comm_data,
                       char            field_name[MAX_FIELD_NAME_LEN],
                       int             client_rank)
{
    char       file_name[320];
    int        j, temp_size;
    FILE*      f = NULL;

    snprintf(file_name, sizeof(file_name), "%s/%s%d_%d.data", data[client_rank].options->restart_dir, field_name, comm_data->rank, client_rank);
    f = fopen(file_name, "rb");
    if (f == NULL)
    {
        melissa_print (VERBOSE_ERROR, "read stats", "ERROR: can not open %s/%s%d_%d.data (read_saved_stats)\n", data[client_rank].options->restart_dir, field_name, comm_data->rank, client_rank);
        return;
    }
    fread(&data[client_rank].vect_size, sizeof(int), 1, f);
    if (data[client_rank].vect_size > 0)
    {
//        if (data[client_rank].options->mean_op != 0 && data[client_rank].options->variance_op == 0)
//        {
//            read_mean(data[client_rank].means, data[client_rank].vect_size, data[client_rank].options->nb_time_steps, f);
//        }
//        if (data[client_rank].options->variance_op != 0)
//        {
//            read_variance(data[client_rank].variances, data[client_rank].vect_size, data[client_rank].options->nb_time_steps, f);
//        }
        read_moments(data[client_rank].moments, data[client_rank].vect_size, data[client_rank].options->nb_time_steps, f);
        if (data[client_rank].options->min_and_max_op != 0)
        {
            melissa_print (VERBOSE_DEBUG, "Read min and max (field %s, server rank %d, client rank %d) (read_saved_stats)\n", field_name, comm_data->rank, client_rank);
            read_min_max(data[client_rank].min_max, data[client_rank].vect_size, data[client_rank].options->nb_time_steps, f);
        }
        if (data[client_rank].options->threshold_op != 0)
        {
            melissa_print (VERBOSE_DEBUG, "Read threshold exceedances (field %s, server rank %d, client rank %d) (read_saved_stats)\n", field_name, comm_data->rank, client_rank);
            read_threshold(data[client_rank].thresholds, data[client_rank].vect_size, data[client_rank].options->nb_time_steps, data[client_rank].options->nb_thresholds, f);
        }
        if (data[client_rank].options->quantile_op != 0)
        {
            melissa_print (VERBOSE_DEBUG, "Read quantiles (field %s, server rank %d, client rank %d) (read_saved_stats)\n", field_name, comm_data->rank, client_rank);
            read_quantile(data[client_rank].quantiles, data[client_rank].vect_size, data[client_rank].options->nb_time_steps, data[client_rank].options->nb_quantiles, f);
        }
        if (data[client_rank].options->sobol_op != 0)
        {
            melissa_print (VERBOSE_DEBUG, "Read sobol indices (field %s, server rank %d, client rank %d) (read_saved_stats)\n", field_name, comm_data->rank, client_rank);
            data[client_rank].read_sobol(data[client_rank].sobol_indices, data[client_rank].vect_size, data[client_rank].options->nb_time_steps, data[client_rank].options->nb_parameters, f);
        }
        if (data[client_rank].options->restart == 1)
        {
            melissa_print (VERBOSE_DEBUG, "Read simu steps (field %s, server rank %d, client rank %d) (read_saved_stats)\n", field_name, comm_data->rank, client_rank);
            fread(&temp_size, sizeof(int), 1, f);
            while (temp_size > data[client_rank].step_simu.size)
            {
                vector_add(&data[client_rank].step_simu, melissa_calloc((data[client_rank].options->nb_time_steps+31)/32, sizeof(int32_t)));
            }
            for (j=0; j<data[client_rank].step_simu.size; j++)
            {
                fread(data[client_rank].step_simu.items[j], sizeof(int32_t), (data[client_rank].options->nb_time_steps+31)/32, f);
            }
        }
    }
    fclose(f);
}

/**
 * This function saves current simulation states on disc
 *
 * @param[in] *simu
 * simulations vector
 *
 * @param[in] *comm_data
 * communication structure
 *
 */

void save_simu_states (vector_t    *simu,
                       comm_data_t *comm_data)
{
    char                  file_name[256];
    FILE*                 f = NULL;
    melissa_simulation_t *simu_ptr;
    int                   i;

    snprintf(file_name, sizeof(file_name), "simu_%d.data",comm_data->rank);
    f = fopen(file_name, "wb+");
    if (f == NULL)
    {
        melissa_print (VERBOSE_WARNING, "Can not open simu_state_%d.data (save_simu_states)\n", comm_data->rank);
        return;
    }
    fwrite(&simu->size, sizeof(int), 1, f);
    for (i=0; i<simu->size; i++)
    {
        simu_ptr = simu->items[i];
        melissa_print (VERBOSE_DEBUG, "Simulation %d status: %d (save_simu_states)\n", i, simu_ptr->status);
        fwrite(&simu_ptr->status, sizeof(int), 1, f);
    }
    fclose(f);
}

/**
 * This function reads simulation states from disc
 *
 * @param[out] *simu
 * simulations vector
 *
 * @param[in] *options
 * Melissa option structure
 *
 * @param[in] *comm_data
 * communication structure
 *
 */

void read_simu_states (vector_t          *simu,
                       melissa_options_t *options,
                       comm_data_t       *comm_data)
{
    char                  file_name[320];
    FILE*                 f = NULL;
    int                   i, size, max_size;
    melissa_simulation_t *simu_ptr;
    MPI_Request          *request;

    snprintf(file_name, sizeof(file_name), "%s/simu_%d.data", options->restart_dir, comm_data->rank);
    f = fopen(file_name, "rb");
    if (f == NULL)
    {
      melissa_print (VERBOSE_WARNING, "Can not open %s (read_simu_states)\n", file_name);
      return;
    }

    fread(&size, sizeof(int), 1, f);
    melissa_print (VERBOSE_DEBUG, "Read file %s (read_simu_states)\n", file_name);
    melissa_print (VERBOSE_WARNING, "Size : %d (process %d)\n", size, comm_data->rank);
    MPI_Allreduce (&size, &max_size, 1, MPI_INT, MPI_MAX, comm_data->comm);
    melissa_print (VERBOSE_DEBUG, "Max size : %d\n", max_size);
    request = melissa_malloc(max_size * sizeof(MPI_Request));
    alloc_vector (simu, max_size);
    for (i=0; i<max_size; i++)
    {
        vector_add (simu, add_simulation());
    }
    for (i=0; i<size; i++)
    {
        simu_ptr = vector_get(simu, i);
        fread(&simu_ptr->status, sizeof(int), 1, f);
        MPI_Allreduce (MPI_IN_PLACE, &simu_ptr->status, 1, MPI_INT, MPI_MIN, comm_data->comm);
    }
    for (i=size; i<max_size; i++)
    {
        simu_ptr = vector_get(simu, i);
        MPI_Allreduce (MPI_IN_PLACE, &simu_ptr->status, 1, MPI_INT, MPI_MIN, comm_data->comm);
    }
    melissa_free (request);

    fclose(f);
}
