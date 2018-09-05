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
#include <stdint.h>
#include <math.h>
//#include "hdf5.h"
#include "melissa_data.h"
#include "melissa_utils.h"
#include "fault_tolerance.h"

#define PLOP 0

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
 * @param[in] *options
 * Melissa option structure
 *
 *******************************************************************************/

int read_client_data (int                *client_comm_size,
                      int               **client_vect_sizes,
                      melissa_options_t  *options)
{
    FILE* f = NULL;
    char  file_name[256];
    int   ret = 1;

    sprintf(file_name, "%s/client_data.save", options->restart_dir);
    melissa_print (VERBOSE_DEBUG, options->verbose_lvl, "DEBUG: Client data save file: %s/client_data.save", file_name);
    f = fopen(file_name, "rb");

    if (f != NULL)
    {
        melissa_print (VERBOSE_DEBUG, options->verbose_lvl, "DEBUG: File %s/client_data.save opened", file_name);
        if (1 == fread(client_comm_size, sizeof(int), 1, f))
        {
            ret = 0;
        }
        *client_vect_sizes = malloc (*client_comm_size * sizeof(int));
        if (*client_comm_size == fread(*client_vect_sizes, sizeof(int), *client_comm_size, f))
        {
            ret = 0;
        }
    }
    else
    {
        options->restart = 0;
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
    int        i, j;
    FILE*      f = NULL;

    for (i=0; i<comm_data->client_comm_size; i++)
    {
        sprintf(file_name, "%s%d_%d.data", field_name, comm_data->rank, i);
        f = fopen(file_name, "wb+");
        if (f == NULL)
        {
            melissa_print (VERBOSE_ERROR, 2, "ERROR: Can not open %s%d_%d.data (save_stats)\n", field_name, comm_data->rank, i);
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
            melissa_print (VERBOSE_DEBUG, data[i].options->verbose_lvl, "DEBUG: Save min and max (field %s, server rank %d, client rank %d) (save_stats)\n", field_name, comm_data->rank, i);
            save_moments(data[i].moments, data[i].vect_size, data[i].options->nb_time_steps, f);
            if (data[i].options->min_and_max_op != 0)
            {
                melissa_print (VERBOSE_DEBUG, data[i].options->verbose_lvl, "DEBUG: Save min and max (field %s, server rank %d, client rank %d) (save_stats)\n", field_name, comm_data->rank, i);
                save_min_max(data[i].min_max, data[i].vect_size, data[i].options->nb_time_steps, f);
            }
            if (data[i].options->threshold_op != 0)
            {
                melissa_print (VERBOSE_DEBUG, data[i].options->verbose_lvl, "DEBUG: Save threshold exceedances (field %s, server rank %d, client rank %d) (save_stats)\n", field_name, comm_data->rank, i);
                save_threshold(data[i].thresholds, data[i].vect_size, data[i].options->nb_time_steps, data[i].options->nb_thresholds, f);
            }
            if (data[i].options->quantile_op != 0)
            {
                melissa_print (VERBOSE_DEBUG, data[i].options->verbose_lvl, "DEBUG: Save quantiles (field %s, server rank %d, client rank %d) (save_stats)\n", field_name, comm_data->rank, i);
                save_quantile(data[i].quantiles, data[i].vect_size, data[i].options->nb_time_steps, data[i].options->nb_quantiles, f);
            }
            if (data[i].options->sobol_op != 0)
            {
                melissa_print (VERBOSE_DEBUG, data[i].options->verbose_lvl, "DEBUG: Save Sobol indices (field %s, server rank %d, client rank %d) (save_stats)\n", field_name, comm_data->rank, i);
                data[i].save_sobol(data[i].sobol_indices, data[i].vect_size, data[i].options->nb_time_steps, data[i].options->nb_parameters, f);
            }
            melissa_print (VERBOSE_DEBUG, data[i].options->verbose_lvl, "DEBUG: Save simulation steps (field %s, server rank %d, client rank %d) (save_stats)\n", field_name, comm_data->rank, i);
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
    int        j, temp_size;
    FILE*      f = NULL;

    sprintf(file_name, "%s/%s%d_%d.data", data[client_rank].options->restart_dir, field_name, comm_data->rank, client_rank);
    f = fopen(file_name, "rb");
    if (f == NULL)
    {
        melissa_print (VERBOSE_ERROR, 2, "ERROR: can not open %s/%s%d_%d.data (read_saved_stats)\n", data[client_rank].options->restart_dir, field_name, comm_data->rank, client_rank);
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
            melissa_print (VERBOSE_DEBUG, data[client_rank].options->verbose_lvl, "DEBUG: Read min and max (field %s, server rank %d, client rank %d) (read_saved_stats)\n", field_name, comm_data->rank, client_rank);
            read_min_max(data[client_rank].min_max, data[client_rank].vect_size, data[client_rank].options->nb_time_steps, f);
        }
        if (data[client_rank].options->threshold_op != 0)
        {
            melissa_print (VERBOSE_DEBUG, data[client_rank].options->verbose_lvl, "DEBUG: Read min and max (field %s, server rank %d, client rank %d) (read_saved_stats)\n", field_name, comm_data->rank, client_rank);
            read_threshold(data[client_rank].thresholds, data[client_rank].vect_size, data[client_rank].options->nb_time_steps, data[client_rank].options->nb_thresholds, f);
        }
        if (data[client_rank].options->quantile_op != 0)
        {
            melissa_print (VERBOSE_DEBUG, data[client_rank].options->verbose_lvl, "DEBUG: Read min and max (field %s, server rank %d, client rank %d) (read_saved_stats)\n", field_name, comm_data->rank, client_rank);
            read_quantile(data[client_rank].quantiles, data[client_rank].vect_size, data[client_rank].options->nb_time_steps, data[client_rank].options->nb_quantiles, f);
        }
        if (data[client_rank].options->sobol_op != 0)
        {
            melissa_print (VERBOSE_DEBUG, data[client_rank].options->verbose_lvl, "DEBUG: Read min and max (field %s, server rank %d, client rank %d) (read_saved_stats)\n", field_name, comm_data->rank, client_rank);
            data[client_rank].read_sobol(data[client_rank].sobol_indices, data[client_rank].vect_size, data[client_rank].options->nb_time_steps, data[client_rank].options->nb_parameters, f);
        }
        if (data[client_rank].options->restart == 1)
        {
            melissa_print (VERBOSE_DEBUG, data[client_rank].options->verbose_lvl, "DEBUG: Read simu steps (field %s, server rank %d, client rank %d) (read_saved_stats)\n", field_name, comm_data->rank, client_rank);
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
 *******************************************************************************
 *
 * @ingroup melissa_io
 *
 * This function saves current simulation states on disc
 *
 *******************************************************************************
 *
 * @param[in] *simu
 * simulations vector
 *
 * @param[in] *comm_data
 * communication structure
 *
 * @param[in] verbose_lvl
 * requested level of verbosity
 *
 *******************************************************************************/

void save_simu_states (vector_t    *simu,
                       comm_data_t *comm_data,
                       int          verbose_lvl)
{
    char                  file_name[256];
    FILE*                 f = NULL;
    melissa_simulation_t *simu_ptr;
    int                   i;

    sprintf(file_name, "simu_%d.data",comm_data->rank);
    f = fopen(file_name, "wb+");
    if (f == NULL)
    {
        melissa_print (VERBOSE_WARNING, verbose_lvl, "WARNING: Can not open simu_state_%d.data (save_simu_states)\n", comm_data->rank);
        return;
    }
    fwrite(&simu->size, sizeof(int), 1, f);
    for (i=0; i<simu->size; i++)
    {
        simu_ptr = simu->items[i];
        melissa_print (VERBOSE_DEBUG, verbose_lvl, "DEBUG: Simulation %d status: %d (save_simu_states)", i, simu_ptr->status);
        fwrite(&simu_ptr->status, sizeof(int), 1, f);
    }
    fclose(f);
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_io
 *
 * This function reads simulation states from disc
 *
 *******************************************************************************
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
 *******************************************************************************/

void read_simu_states (vector_t          *simu,
                       melissa_options_t *options,
                       comm_data_t       *comm_data)
{
    char                  file_name[256];
    FILE*                 f = NULL;
    int                   i, size, max_size;
    melissa_simulation_t *simu_ptr;
#ifdef BUILD_WITH_MPI
    MPI_Request          *request;
#endif // BUILD_WITH_MPI

    sprintf(file_name, "%s/simu_%d.data", options->restart_dir, comm_data->rank);
    f = fopen(file_name, "rb");
    if (f == NULL)
    {
      melissa_print (VERBOSE_WARNING, options->verbose_lvl, "WARNING: Can not open %s (read_simu_states)\n", file_name);
      return;
    }

    fread(&size, sizeof(int), 1, f);
    melissa_print (VERBOSE_GOSSIP, options->verbose_lvl, "INFO: Read file %s\n", file_name);
    melissa_print (VERBOSE_WARNING, options->verbose_lvl, "DEBUG: Size : %d (process %d)\n", size, comm_data->rank);
#ifdef BUILD_WITH_MPI
    MPI_Allreduce (&size, &max_size, 1, MPI_INT, MPI_MAX, comm_data->comm);
    melissa_print (VERBOSE_DEBUG, options->verbose_lvl, "DEBUG: Max size : %d\n", max_size);
    request = melissa_malloc(max_size * sizeof(MPI_Request));
#else // BUILD_WITH_MPI
    max_size = size;
#endif // BUILD_WITH_MPI
    alloc_vector (simu, max_size);
    for (i=0; i<max_size; i++)
    {
        vector_add (simu, add_simulation());
    }
    for (i=0; i<size; i++)
    {
        simu_ptr = vector_get(simu, i);
        fread(&simu_ptr->status, sizeof(int), 1, f);
#ifdef BUILD_WITH_MPI
        MPI_Allreduce (MPI_IN_PLACE, &simu_ptr->status, 1, MPI_INT, MPI_MIN, comm_data->comm);
    }
    for (i=size; i<max_size; i++)
    {
        simu_ptr = vector_get(simu, i);
        MPI_Allreduce (MPI_IN_PLACE, &simu_ptr->status, 1, MPI_INT, MPI_MIN, comm_data->comm);
#endif // BUILD_WITH_MPI
    }
#ifdef BUILD_WITH_MPI
    melissa_free (request);
#endif // BUILD_WITH_MPI

    fclose(f);
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
 * Melissa option structure
 *
 * @param[in] comm_data
 * structure containing communications parameters
 *
 * @param[in] *field
 * name of the statistic field to write
 *
 *******************************************************************************/

void write_stats_bin (melissa_data_t    **data,
                      melissa_options_t  *options,
                      comm_data_t        *comm_data,
                      char               *field
                      )
{
    long int   i, temp_offset = 0;
    int        t, offset = 0;
#ifdef BUILD_WITH_MPI
    MPI_File   f;
    MPI_Status status;
#else // BUILD_WITH_MPI
    FILE* f;
#endif // BUILD_WITH_MPI
    char       file_name[256];
    int        max_size_time;
    int        vect_size = 0;
    int       *local_vect_sizes;
    int        global_vect_size = 0;

//// https://support.hdfgroup.org/HDF5/Tutor/parallel.html
//    /*
//     * HDF5 APIs definitions
//     */
//    hid_t   file_id;         /* file and dataset identifiers */
//    hid_t	plist_id;        /* property list identifier( access template) */
//    herr_t	status2;

    max_size_time=floor(log10(options->nb_time_steps))+1;

    // ============================================ //
    //                                              //
    // Communicate local vect size to every process //
    //                                              //
    // ============================================ //

    local_vect_sizes = melissa_calloc (comm_data->comm_size, sizeof(int));
    for (i=0; i<comm_data->client_comm_size; i++)
    {
        vect_size += (*data)[i].vect_size;
    }
#ifdef BUILD_WITH_MPI
    MPI_Allgather (&vect_size, 1, MPI_INT, local_vect_sizes, 1, MPI_INT, comm_data->comm);
#endif // BUILD_WITH_MPI

    for (i=0; i<comm_data->comm_size; i++)
    {
        global_vect_size += local_vect_sizes[i];
    }

    // ============================================ //

    for (i=0; i<comm_data->rank; i++)
    {
        offset += local_vect_sizes[i];
    }

    if (options->mean_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "%s_mean_%.*d", field, max_size_time, (int)t+1);
//            /*
//             * Set up file access property list with parallel I/O access
//             */
//             plist_id = H5Pcreate(H5P_FILE_ACCESS);
//             H5Pset_fapl_mpio(plist_id, comm_data->comm, info);

//             /*
//              * Create a new file collectively.
//              */
//             file_id = H5Fcreate(file_name, H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
            temp_offset = 0;
#else // BUILD_WITH_MPI
            f = fopen(file_name, "wb");
#endif // BUILD_WITH_MPI
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
#ifdef BUILD_WITH_MPI
                    MPI_File_write_at (f, offset + temp_offset, (*data)[i].moments[t].m1, (*data)[i].vect_size, MPI_DOUBLE, &status);
                    temp_offset += (*data)[i].vect_size;
#else // BUILD_WITH_MPI
                    fwrite((*data)[i].moments[t].m1, sizeof(double), (*data)[i].vect_size, f);
#endif // BUILD_WITH_MPI
                }
            }
#ifdef BUILD_WITH_MPI
            MPI_File_close (&f);
#else // BUILD_WITH_MPI
            fclose(f);
#endif // BUILD_WITH_MPI

//        /*
//         * Close property list.
//         */
//        H5Pclose(plist_id);

//        /*
//         * Close the file.
//         */
//        H5Fclose(file_id);
        }
    }

    if (options->variance_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "%s_variance_%.*d", field, max_size_time, (int)t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
            temp_offset = 0;
#else // BUILD_WITH_MPI
            f = fopen(file_name, "wb");
#endif // BUILD_WITH_MPI
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
#ifdef BUILD_WITH_MPI
                    MPI_File_write_at (f, offset + temp_offset, (*data)[i].moments[t].theta2, (*data)[i].vect_size, MPI_DOUBLE, &status);
                    temp_offset += (*data)[i].vect_size;
#else // BUILD_WITH_MPI
                    fwrite((*data)[i].moments[t].theta2, sizeof(double), (*data)[i].vect_size, f);
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

    if (options->min_and_max_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "%s_min_%.*d", field, max_size_time, (int)t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
            temp_offset = 0;
#else // BUILD_WITH_MPI
            f = fopen(file_name, "wb");
#endif // BUILD_WITH_MPI
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
#ifdef BUILD_WITH_MPI
                    MPI_File_write_at (f, offset + temp_offset, (*data)[i].min_max[t].min, (*data)[i].vect_size, MPI_DOUBLE, &status);
                    temp_offset += (*data)[i].vect_size;
#else // BUILD_WITH_MPI
                    fwrite((*data)[i].min_max[t].min, sizeof(double), (*data)[i].vect_size, f);
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
            sprintf(file_name, "%s_max_%.*d", field, max_size_time, (int)t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
            temp_offset = 0;
#else // BUILD_WITH_MPI
            f = fopen(file_name, "wb");
#endif // BUILD_WITH_MPI
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
#ifdef BUILD_WITH_MPI
                    MPI_File_write_at (f, offset + temp_offset, (*data)[i].min_max[t].max, (*data)[i].vect_size, MPI_DOUBLE, &status);
                    temp_offset += (*data)[i].vect_size;
#else // BUILD_WITH_MPI
                    fwrite((*data)[i].min_max[t].max, sizeof(double), (*data)[i].vect_size, f);
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
            sprintf(file_name, "%s_threshold_exceedance_%.*d", field, max_size_time, (int)t+1);
#ifdef BUILD_WITH_MPI
            MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
            temp_offset = 0;
#else // BUILD_WITH_MPI
            f = fopen(file_name, "wb");
#endif // BUILD_WITH_MPI
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
#ifdef BUILD_WITH_MPI
                    MPI_File_write_at (f, offset + temp_offset, (*data)[i].thresholds[t], (*data)[i].vect_size, MPI_INT, &status);
                    temp_offset += (*data)[i].vect_size;
#else // BUILD_WITH_MPI
                    fwrite((*data)[i].thresholds[t], sizeof(int), (*data)[i].vect_size, f);
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

    if (options->quantile_op == 1)
    {
        int value;
        for (value=0; value<options->nb_quantiles; value++)
        {
            for (t=0; t<options->nb_time_steps; t++)
            {
                sprintf(file_name, "%s_quantile%g_%.*d", field, options->quantile_order[value], max_size_time, (int)t+1);
#ifdef BUILD_WITH_MPI
                MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
                temp_offset = 0;
#else // BUILD_WITH_MPI
                f = fopen(file_name, "wb");
#endif // BUILD_WITH_MPI
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
#ifdef BUILD_WITH_MPI
                        MPI_File_write_at (f, offset + temp_offset, (*data)[i].quantiles[t][value].quantile, (*data)[i].vect_size, MPI_DOUBLE, &status);
                        temp_offset += (*data)[i].vect_size;
#else // BUILD_WITH_MPI
                        fwrite((*data)[i].quantiles[t][value].quantile, sizeof(double), (*data)[i].vect_size, f);
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

    if (options->sobol_op == 1)
    {
        int p;
        for (t=0; t<options->nb_time_steps; t++)
        {
            for (p=0; p<options->nb_parameters; p++)
            {
                sprintf(file_name, "%s_sobol_indices_%.*d.%d",field,  max_size_time, (int)t+1, p);
#ifdef BUILD_WITH_MPI
                MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
                temp_offset = 0;
#else // BUILD_WITH_MPI
                f = fopen(file_name, "wb");
#endif // BUILD_WITH_MPI
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
#ifdef BUILD_WITH_MPI
                        MPI_File_write_at (f, offset + temp_offset, (*data)[i].sobol_indices[t].sobol_martinez[p].first_order_values, (*data)[i].vect_size, MPI_INT, &status);
                        temp_offset += (*data)[i].vect_size;
#else // BUILD_WITH_MPI
                        fwrite((*data)[i].sobol_indices[t].sobol_martinez[p].first_order_values, sizeof(double), (*data)[i].vect_size, f);
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
        // total order
        for (t=0; t<options->nb_time_steps; t++)
        {
            for (p=0; p<options->nb_parameters; p++)
            {
                sprintf(file_name, "%s_sobol_total_indices_%.*d.%d",field,  max_size_time, (int)t+1, p);
#ifdef BUILD_WITH_MPI
                MPI_File_open (comm_data->comm, file_name, MPI_MODE_CREATE|MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
                temp_offset = 0;
#else // BUILD_WITH_MPI
                f = fopen(file_name, "wb");
#endif // BUILD_WITH_MPI
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
#ifdef BUILD_WITH_MPI
                        MPI_File_write_at (f, offset + temp_offset, (*data)[i].sobol_indices[t].sobol_martinez[p].total_order_values, (*data)[i].vect_size, MPI_INT, &status);
                        temp_offset += (*data)[i].vect_size;
#else // BUILD_WITH_MPI
                        fwrite((*data)[i].sobol_indices[t].sobol_martinez[p].total_order_values, sizeof(double), (*data)[i].vect_size, f);
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
    melissa_free (local_vect_sizes);
}

#ifdef PLOP

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
 * @param[in] *field
 * name of the field on which are computed the statistics
 *
 *******************************************************************************/

void write_stats_ensight (melissa_data_t    **data,
                          melissa_options_t  *options,
                          comm_data_t        *comm_data,
                          char               *field)
{
    long int    temp_offset=0;
#ifdef BUILD_WITH_MPI
    MPI_Status  status;
#endif // BUILD_WITH_MPI
    long int    i, j, offset=0;
    int         t, param;
    FILE*       f;
    char        file_name[256];
    int         max_size_time;
    int         vect_size = 0;
    double      time_value = 0;
    float      *s_buffer;
    char        c_buffer[81], c_buffer2[81];
    int        *local_vect_sizes;
    int         global_vect_size = 0;
    int32_t     n;
//    double temp1, temp2;

    max_size_time=floor(log10(options->nb_time_steps))+1;

    local_vect_sizes = melissa_calloc (comm_data->comm_size, sizeof(int));
    for (i=0; i<comm_data->client_comm_size; i++)
    {
        vect_size += (*data)[i].vect_size;
    }
#ifdef BUILD_WITH_MPI
    MPI_Allgather (&vect_size, 1, MPI_INT, local_vect_sizes, 1, MPI_INT, comm_data->comm);
#endif // BUILD_WITH_MPI

    for (i=0; i<comm_data->rank; i++)
    {
        offset += local_vect_sizes[i];
    }

    for (i=0; i<comm_data->comm_size; i++)
    {
        global_vect_size += local_vect_sizes[i];
    }

    s_buffer = melissa_malloc (global_vect_size * sizeof(float));

#ifdef BUILD_WITH_MPI
    MPI_Barrier(comm_data->comm);
#endif // BUILD_WITH_MPI

    sprintf(file_name, "results.%s_server_mpi_rank_id", field);
    temp_offset = 0;
    for (i=0; i<comm_data->client_comm_size; i++)
    {
        if ((*data)[i].vect_size > 0)
        {
            for (j=0; j<(*data)[i].vect_size; j++)
            {
                s_buffer[j + offset + temp_offset] = (float)comm_data->rank;
            }
            temp_offset += (*data)[i].vect_size;
        }
    }
#ifdef BUILD_WITH_MPI
    if (comm_data->rank == 0)
    {
        temp_offset = 0;
        for (j=1; j<comm_data->comm_size; j++)
        {
            temp_offset += local_vect_sizes[j-1];
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
        sprintf(c_buffer, "%s_rank (time values: %d, %g)", field, 0, time_value);
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
        fwrite (s_buffer, sizeof(float), global_vect_size, f);

        fclose(f);
    }

    if (options->mean_op == 1)
    {
        time_value = 0;
        for (t=0; t<options->nb_time_steps; t++)
        {
            time_value += 0.0012;
            sprintf(file_name, "results.%s_mean.%.*d", field, max_size_time, (int)t+1);
            temp_offset = 0;
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    for (j=0; j<(*data)[i].vect_size; j++)
                    {
                        s_buffer[j + offset + temp_offset] = (float)(*data)[i].moments[t].m1[j];
                    }
                    temp_offset += (*data)[i].vect_size;
                }
            }
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                temp_offset = 0;
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j-1];
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
                sprintf(c_buffer, "%s_mean (time values: %d, %g)", field, (int)t, time_value);
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
                fwrite (s_buffer, sizeof(float), global_vect_size, f);

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
            sprintf(file_name, "results.%s_variance.%.*d", field, max_size_time, (int)t+1);
            temp_offset = 0;
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    for (j=0; j<(*data)[i].vect_size; j++)
                    {
                        s_buffer[j + offset + temp_offset] = (float)(*data)[i].moments[t].theta2[j];
                    }
                    temp_offset += (*data)[i].vect_size;
                }
            }
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                temp_offset = 0;
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j-1];
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
                sprintf(c_buffer, "%s_variance (time values: %d, %g)", field, (int)t, time_value);
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
                fwrite (s_buffer, sizeof(float), global_vect_size, f);

                fclose(f);
            }
        }
    }

    if (options->skewness_op == 1)
    {
        time_value = 0;
        for (t=0; t<options->nb_time_steps; t++)
        {
            time_value += 0.0012;
            sprintf(file_name, "results.%s_skewness.%.*d", field, max_size_time, (int)t+1);
            temp_offset = 0;
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    for (j=0; j<(*data)[i].vect_size; j++)
                    {
                        s_buffer[j + offset + temp_offset] = (float)((*data)[i].moments[t].theta3[j]/pow((*data)[i].moments[t].theta2[j], 1.5));
                    }
                    temp_offset += (*data)[i].vect_size;
                }
            }
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                temp_offset = 0;
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j-1];
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
                sprintf(c_buffer, "%s_skewness (time values: %d, %g)", field, (int)t, time_value);
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
                fwrite (s_buffer, sizeof(float), global_vect_size, f);

                fclose(f);
            }
        }
    }

    if (options->kurtosis_op == 1)
    {
        time_value = 0;
        for (t=0; t<options->nb_time_steps; t++)
        {
            time_value += 0.0012;
            sprintf(file_name, "results.%s_kurtosis.%.*d", field, max_size_time, (int)t+1);
            temp_offset = 0;
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    for (j=0; j<(*data)[i].vect_size; j++)
                    {
                        s_buffer[j + offset + temp_offset] = (float)((*data)[i].moments[t].theta4[j]/pow((*data)[i].moments[t].theta3[j], 2));
                    }
                    temp_offset += (*data)[i].vect_size;
                }
            }
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                temp_offset = 0;
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j-1];
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
                sprintf(c_buffer, "%s_kurtosis (time values: %d, %g)", field, (int)t, time_value);
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
                fwrite (s_buffer, sizeof(float), global_vect_size, f);

                fclose(f);
            }
        }
    }

    if (options->min_and_max_op == 1)
    {
        time_value = 0;
        for (t=0; t<options->nb_time_steps; t++)
        {
            time_value += 0.0012;
            sprintf(file_name, "results.%s_min.%.*d", field, max_size_time, (int)t+1);
            temp_offset = 0;
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    for (j=0; j<(*data)[i].vect_size; j++)
                    {
                        s_buffer[j + offset + temp_offset] = (float)(*data)[i].min_max[t].min[j];
                    }
                    temp_offset += (*data)[i].vect_size;
                }
            }
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                temp_offset = 0;
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j-1];
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
                sprintf(c_buffer, "%s_min (time values: %d, %g)", field, (int)t, time_value);
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
                fwrite (s_buffer, sizeof(float), global_vect_size, f);

                fclose(f);
            }
        }

        time_value = 0;
        for (t=0; t<options->nb_time_steps; t++)
        {
            time_value += 0.0012;
            sprintf(file_name, "results.%s_max.%.*d", field, max_size_time, (int)t+1);
            temp_offset = 0;
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    for (j=0; j<(*data)[i].vect_size; j++)
                    {
                        s_buffer[j + offset + temp_offset] = (float)(*data)[i].min_max[t].max[j];
                    }
                    temp_offset += (*data)[i].vect_size;
                }
            }
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                temp_offset = 0;
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j-1];
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
                sprintf(c_buffer, "%s_max (time values: %d, %g)", field, (int)t, time_value);
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
                fwrite (s_buffer, sizeof(float), global_vect_size, f);

                fclose(f);
            }
        }
    }

    if (options->threshold_op == 1)
    {
        int value;
        for (value=0; value<options->nb_thresholds; value++)
        {
            time_value = 0;
            for (t=0; t<options->nb_time_steps; t++)
            {
                time_value += 0.0012;
                sprintf(file_name, "results.%s_threshold%g.%.*d", field, options->threshold[value], max_size_time,(int) t+1);
                temp_offset = 0;
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
                        for (j=0; j<(*data)[i].vect_size; j++)
                        {
                            s_buffer[j + offset + temp_offset] = (float)(*data)[i].thresholds[t][value].threshold_exceedance[j];
                        }
                        temp_offset += (*data)[i].vect_size;
                    }
                }
#ifdef BUILD_WITH_MPI
                if (comm_data->rank == 0)
                {
                    temp_offset = 0;
                    for (j=1; j<comm_data->comm_size; j++)
                    {
                        temp_offset += local_vect_sizes[j-1];
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
                    sprintf(c_buffer, "%s_threshold (time values: %d, %g)", field, (int)t, time_value);
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
                    fwrite (s_buffer, sizeof(float), global_vect_size, f);

                    fclose(f);
                }
            }
        }
    }

    if (options->quantile_op == 1)
    {
        int value;
        for (value=0; value<options->nb_quantiles; value++)
        {
            time_value = 0;
            for (t=0; t<options->nb_time_steps; t++)
            {
                time_value += 0.0012;
                sprintf(file_name, "results.%s_quantile%g.%.*d", field, options->quantile_order[value], max_size_time,(int) t+1);
                temp_offset = 0;
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
                        for (j=0; j<(*data)[i].vect_size; j++)
                        {
                            s_buffer[j + offset + temp_offset] = (float)(*data)[i].quantiles[t][value].quantile[j];
                        }
                        temp_offset += (*data)[i].vect_size;
                    }
                }
#ifdef BUILD_WITH_MPI
                if (comm_data->rank == 0)
                {
                    temp_offset = 0;
                    for (j=1; j<comm_data->comm_size; j++)
                    {
                        temp_offset += local_vect_sizes[j-1];
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
                    sprintf(c_buffer, "%s_quantile%g (time values: %d, %g)", field, options->quantile_order[value], (int)t, time_value);
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
                    fwrite (s_buffer, sizeof(float), global_vect_size, f);

                    fclose(f);
                }
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
                sprintf(file_name, "results.%s_sobol%d.%.*d", field, param, max_size_time, (int)(t+1));
                temp_offset = 0;
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
                        for (j=0; j<(*data)[i].vect_size; j++)
                        {
                            s_buffer[j + offset + temp_offset] = (float)(*data)[i].sobol_indices[t].sobol_martinez[param].first_order_values[j];
                        }
                        temp_offset += (*data)[i].vect_size;
                    }
                }
#ifdef BUILD_WITH_MPI
                if (comm_data->rank == 0)
                {
                    temp_offset = 0;
                    for (j=1; j<comm_data->comm_size; j++)
                    {
                        temp_offset += local_vect_sizes[j-1];
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
                    sprintf(c_buffer, "%s_sobol%d (time values: %d, %g)", field, param, (int)t, time_value);
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
                    fwrite (s_buffer, sizeof(float), global_vect_size, f);

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
                sprintf(file_name, "results.%s_sobol_tot%d.%.*d", field, param, max_size_time, (int)t+1);
                temp_offset = 0;
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
                        for (j=0; j<(*data)[i].vect_size; j++)
                        {
                            s_buffer[j + offset + temp_offset] = (float)(*data)[i].sobol_indices[t].sobol_martinez[param].total_order_values[j];
                        }
                        temp_offset += (*data)[i].vect_size;
                    }
                }
#ifdef BUILD_WITH_MPI
                if (comm_data->rank == 0)
                {
                    temp_offset = 0;
                    for (j=1; j<comm_data->comm_size; j++)
                    {
                        temp_offset += local_vect_sizes[j-1];
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
                    sprintf(c_buffer, "%s_sobol_tot%d (time values: %d, %g)", field, param, (int)t, time_value);
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
                    fwrite (s_buffer, sizeof(float), global_vect_size, f);

                    fclose(f);
                }
            }
        }
        //======================//
        // Confidence intervals //
        //======================//
//        temp2 = 1.96/(sqrt((*data)[0].sobol_indices[0].iteration-3));
//        for (param=0; param<options->nb_parameters; param++)
//        {
//            time_value = 0;
//            for (t=0; t<options->nb_time_steps; t++)
//            {
//                time_value += 0.0012;
//                sprintf(file_name, "results.%s_sobol_confidence%d.%.*d", field, param, max_size_time, (int)(t+1));
//                temp_offset = 0;
//                for (i=0; i<comm_data->client_comm_size; i++)
//                {
//                    if ((*data)[i].vect_size > 0)
//                    {
//                        for (j=0; j<(*data)[i].vect_size; j++)
//                        {
//                            temp1 = 0.5 * log((1.0+(*data)[i].sobol_indices[t].sobol_martinez[param].first_order_values[j])/(1.0-(*data)[i].sobol_indices[t].sobol_martinez[param].first_order_values[j]));
//                            s_buffer[j + offset + temp_offset] = (float)(tanh(temp1 + temp2) - tanh(temp1 - temp2));
//                            temp_offset += (*data)[i].vect_size;
//                        }
//                    }
//                }
//#ifdef BUILD_WITH_MPI
//                if (comm_data->rank == 0)
//                {
//                    temp_offset = 0;
//                    for (j=1; j<comm_data->comm_size; j++)
//                    {
//                        temp_offset += local_vect_sizes[j-1];
//                        MPI_Recv (&s_buffer[temp_offset], local_vect_sizes[j], MPI_FLOAT, j, j+121, comm_data->comm, &status);
//                    }
//                }
//                else
//                {
//                    MPI_Send(&s_buffer[offset], local_vect_sizes[comm_data->rank], MPI_FLOAT, 0, comm_data->rank+121, comm_data->comm);
//                }
//#endif // BUILD_WITH_MPI
//                if (comm_data->rank == 0)
//                {
//                    f = fopen(file_name, "w");
//                    sprintf(c_buffer, "%s_sobol_confidence%d (time values: %d, %g)", field, param, (int)t, time_value);
//                    strncpy(c_buffer2, c_buffer, 80);
//                    for (i=strlen(c_buffer); i < 80; i++)
//                      c_buffer2[i] = ' ';
//                    c_buffer2[80] = '\0';
//                    fwrite (c_buffer2, sizeof(char), 80, f);
//                    n = 1;
//                    sprintf(c_buffer, "part");
//                    strncpy(c_buffer2, c_buffer, 80);
//                    for (i=strlen(c_buffer); i < 80; i++)
//                      c_buffer2[i] = ' ';
//                    c_buffer2[80] = '\0';
//                    fwrite (c_buffer2, sizeof(char), 80, f);
//                    fwrite (&n, sizeof(int32_t), 1, f);
//                    sprintf(c_buffer, "hexa8");
//                    strncpy(c_buffer2, c_buffer, 80);
//                    for (i=strlen(c_buffer); i < 80; i++)
//                      c_buffer2[i] = ' ';
//                    c_buffer2[80] = '\0';
//                    fwrite (c_buffer2, sizeof(char), 80, f);
//                    fwrite (s_buffer, sizeof(float), global_vect_size, f);

//                    fclose(f);
//                }
//            }
//        }
//        for (param=0; param<options->nb_parameters; param++)
//        {
//            time_value = 0;
//            for (t=0; t<options->nb_time_steps; t++)
//            {
//                time_value += 0.0012;
//                sprintf(file_name, "results.%s_sobol_tot_confidence%d.%.*d", field, param, max_size_time, (int)t+1);
//                for (i=0; i<comm_data->client_comm_size; i++)
//                {
//                    if ((*data)[i].vect_size > 0)
//                    temp_offset = 0;
//                    {
//                        for (j=0; j<(*data)[i].vect_size; j++)
//                        {
//                            temp1 = 0.5 * log((2.0-(*data)[i].sobol_indices[t].sobol_martinez[param].total_order_values[j])/(*data)[i].sobol_indices[t].sobol_martinez[param].total_order_values[j]);
//                            s_buffer[j + offset + temp_offset] = (float)(1-tanh(temp1 - temp2)) - (1-tanh(temp1 + temp2));
//                        }
//                        temp_offset += (*data)[i].vect_size;
//                    }
//                }
//#ifdef BUILD_WITH_MPI
//                if (comm_data->rank == 0)
//                {
//                    temp_offset = 0;
//                    for (j=1; j<comm_data->comm_size; j++)
//                    {
//                        temp_offset += local_vect_sizes[j-1];
//                        MPI_Recv (&s_buffer[temp_offset], local_vect_sizes[j], MPI_FLOAT, j, j+121, comm_data->comm, &status);
//                    }
//                }
//                else
//                {
//                    MPI_Send(&s_buffer[offset], local_vect_sizes[comm_data->rank], MPI_FLOAT, 0, comm_data->rank+121, comm_data->comm);
//                }
//#endif // BUILD_WITH_MPI
//                if (comm_data->rank == 0)
//                {
//                    f = fopen(file_name, "w");
//                    sprintf(c_buffer, "%s_sobol_tot_confidence%d (time values: %d, %g)", field, param, (int)t, time_value);
//                    strncpy(c_buffer2, c_buffer, 80);
//                    for (i=strlen(c_buffer); i < 80; i++)
//                      c_buffer2[i] = ' ';
//                    c_buffer2[80] = '\0';
//                    fwrite (c_buffer2, sizeof(char), 80, f);
//                    n = 1;
//                    sprintf(c_buffer, "part");
//                    strncpy(c_buffer2, c_buffer, 80);
//                    for (i=strlen(c_buffer); i < 80; i++)
//                      c_buffer2[i] = ' ';
//                    c_buffer2[80] = '\0';
//                    fwrite (c_buffer2, sizeof(char), 80, f);
//                    fwrite (&n, sizeof(int32_t), 1, f);
//                    sprintf(c_buffer, "hexa8");
//                    strncpy(c_buffer2, c_buffer, 80);
//                    for (i=strlen(c_buffer); i < 80; i++)
//                      c_buffer2[i] = ' ';
//                    c_buffer2[80] = '\0';
//                    fwrite (c_buffer2, sizeof(char), 80, f);
//                    fwrite (s_buffer, sizeof(float), global_vect_size, f);

//                    fclose(f);
//                }
//            }
//        }
    }
    melissa_free (s_buffer);
    melissa_free (local_vect_sizes);
}

#ifdef TOTO
///**
// *******************************************************************************
// *
// * @ingroup melissa_io
// *
// * This function reads data from Ensight files
// *
// *******************************************************************************
// *
// * @param[in] *options
// * option structure
// *
// * @param[in] *comm_data
// * structure containing communications parameters
// *
// * @param[in] *in_vect
// * input vector
// *
// * @param[in] *local_vect_sizes
// * all local vector sises
// *
// * @param[in] *file_name
// * name of the file
// *
// *******************************************************************************/

//void read_ensight (melissa_options_t  *options,
//                   comm_data_t      *comm_data,
//                   double           *in_vect,
//                   int              *local_vect_sizes,
//                   char             *file_name)
//{
//#ifdef BUILD_WITH_MPI
//    long int    i, j, offset=0;
//    MPI_Status  status;
//#endif // BUILD_WITH_MPI
//    FILE*       f;
//    float      *r_buffer;
//    char        c_buffer[81];
//    int32_t     n;

//    if (comm_data->rank == 0)
//    {
//        r_buffer = malloc (global_vect_size * sizeof(float));
//    }
//    else
//    {
//        r_buffer = malloc (local_vect_sizes[comm_data->rank] * sizeof(float));
//    }

//    if (comm_data->rank == 0)
//    {
//        f = fopen(file_name, "r");
//        if (f == NULL) printf ("f = NULL\n");
//        fread (c_buffer, sizeof(char), 80, f);
//        fread (c_buffer, sizeof(char), 80, f);
//        fread (&n, sizeof(int32_t), 1, f);
//        fread (c_buffer, sizeof(char), 80, f);
//        fread (r_buffer, sizeof(float), global_vect_size, f);
//        fclose(f);
//    }
//#ifdef BUILD_WITH_MPI
//    if (comm_data->rank == 0)
//    {
//        offset = 0;
//        for (j=1; j<comm_data->comm_size; j++)
//        {
//            offset += local_vect_sizes[j];
//            MPI_Send (&r_buffer[offset], local_vect_sizes[j], MPI_FLOAT, j, j+121, comm_data->comm);
//        }
//    }
//    else
//    {
//        MPI_Recv(r_buffer, local_vect_sizes[comm_data->rank], MPI_FLOAT, 0, comm_data->rank+121, comm_data->comm, &status);
//    }

//    for (i=0; i<local_vect_sizes[comm_data->rank]; i++)
//    {
//        in_vect[i] = (double)r_buffer[i];
//    }
//    melissa_free (r_buffer);
//#endif // BUILD_WITH_MPI
//}
#endif // TOTO
#endif // PLOP

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
 * Melissa option structure
 *
 * @param[in] comm_data
 * structure containing communications parameters
 *
 * @param[in] *field
 * name of the statistic field to write
 *
 *******************************************************************************/

void write_stats_txt (melissa_data_t    **data,
                      melissa_options_t  *options,
                      comm_data_t        *comm_data,
                      char               *field)
{
    long int    temp_offset=0;
#ifdef BUILD_WITH_MPI
    MPI_Status  status;
#endif // BUILD_WITH_MPI
    long int    i, j;
    int        *offsets;
    int         t, p;
    FILE*       f;
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
#ifdef BUILD_WITH_MPI
    MPI_Allgather (&vect_size, 1, MPI_INT, local_vect_sizes, 1, MPI_INT, comm_data->comm);
#endif // BUILD_WITH_MPI

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
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "results.%s_mean.%.*d", field, max_size_time, (int)t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    memcpy(&d_buffer[temp_offset], (*data)[i].moments[t].m1, (*data)[i].vect_size*sizeof(double));
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j-1];
                    MPI_Recv (&(d_buffer[temp_offset]), local_vect_sizes[j], MPI_DOUBLE, j, j+121, comm_data->comm, &status);
                }
                temp_offset = 0;
            }
            else
            {
                MPI_Send(d_buffer, local_vect_sizes[comm_data->rank], MPI_DOUBLE, 0, comm_data->rank+121, comm_data->comm);
            }
#endif // BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                f = fopen(file_name, "w");
                for (i=0; i<global_vect_size; i++)
                {
                    fprintf (f, "%g\n", d_buffer[i]);
                }
                fclose(f);
            }
        }
    }

    if (options->variance_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "results.%s_variance.%.*d", field, max_size_time, (int)t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    memcpy(&d_buffer[temp_offset], (*data)[i].moments[t].theta2, (*data)[i].vect_size*sizeof(double));
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j-1];
                    MPI_Recv (&d_buffer[temp_offset], local_vect_sizes[j], MPI_DOUBLE, j, j+121, comm_data->comm, &status);
                }
                temp_offset = 0;
            }
            else
            {
                MPI_Send(d_buffer, local_vect_sizes[comm_data->rank], MPI_DOUBLE, 0, comm_data->rank+121, comm_data->comm);
            }
#endif // BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                f = fopen(file_name, "w");
                for (i=0; i<global_vect_size; i++)
                {
                    fprintf (f, "%g\n", d_buffer[i]);
                }
                fclose(f);
            }
        }
    }

    if (options->skewness_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "results.%s_skewness.%.*d", field, max_size_time, (int)t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    compute_skewness (&(*data)[i].moments[t], &d_buffer[temp_offset], (*data)[i].vect_size);
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j-1];
                    MPI_Recv (&d_buffer[temp_offset], local_vect_sizes[j], MPI_DOUBLE, j, j+121, comm_data->comm, &status);
                }
                temp_offset = 0;
            }
            else
            {
                MPI_Send(d_buffer, local_vect_sizes[comm_data->rank], MPI_DOUBLE, 0, comm_data->rank+121, comm_data->comm);
            }
#endif // BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                f = fopen(file_name, "w");
                for (i=0; i<global_vect_size; i++)
                {
                    fprintf (f, "%g\n", d_buffer[i]);
                }
                fclose(f);
            }
        }
    }

    if (options->kurtosis_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "results.%s_kurtosis.%.*d", field, max_size_time, (int)t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    compute_kurtosis (&(*data)[i].moments[t], &d_buffer[temp_offset], (*data)[i].vect_size);
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j-1];
                    MPI_Recv (&d_buffer[temp_offset], local_vect_sizes[j], MPI_DOUBLE, j, j+121, comm_data->comm, &status);
                }
                temp_offset = 0;
            }
            else
            {
                MPI_Send(d_buffer, local_vect_sizes[comm_data->rank], MPI_DOUBLE, 0, comm_data->rank+121, comm_data->comm);
            }
#endif // BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                f = fopen(file_name, "w");
                for (i=0; i<global_vect_size; i++)
                {
                    fprintf (f, "%g\n", d_buffer[i]);
                }
                fclose(f);
            }
        }
    }

    if (options->min_and_max_op == 1)
    {
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "results.%s_min.%.*d", field, max_size_time, (int)t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    memcpy(&d_buffer[temp_offset], (*data)[i].min_max[t].min, (*data)[i].vect_size*sizeof(double));
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j-1];
                    MPI_Recv (&d_buffer[temp_offset], local_vect_sizes[j], MPI_DOUBLE, j, j+121, comm_data->comm, &status);
                }
                temp_offset = 0;
            }
            else
            {
                MPI_Send(d_buffer, local_vect_sizes[comm_data->rank], MPI_DOUBLE, 0, comm_data->rank+121, comm_data->comm);
            }
#endif // BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                f = fopen(file_name, "w");
                for (i=0; i<global_vect_size; i++)
                {
                    fprintf (f, "%g\n", d_buffer[i]);
                }
                fclose(f);
            }
        }

        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "results.%s_max.%.*d", field, max_size_time, (int)t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    memcpy(&d_buffer[temp_offset], (*data)[i].min_max[t].max, (*data)[i].vect_size*sizeof(double));
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j-1];
                    MPI_Recv (&d_buffer[temp_offset], local_vect_sizes[j], MPI_DOUBLE, j, j+121, comm_data->comm, &status);
                }
                temp_offset = 0;
            }
            else
            {
                MPI_Send(d_buffer, local_vect_sizes[comm_data->rank], MPI_DOUBLE, 0, comm_data->rank+121, comm_data->comm);
            }
#endif // BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                f = fopen(file_name, "w");
                for (i=0; i<global_vect_size; i++)
                {
                    fprintf (f, "%g\n", d_buffer[i]);
                }
                fclose(f);
            }
        }
    }

    if (options->threshold_op == 1)
    {
        i_buffer = (int*)d_buffer;
        for (t=0; t<options->nb_time_steps; t++)
        {
            sprintf(file_name, "results.%s_threshold.%.*d", field, max_size_time, (int)t+1);
            for (i=0; i<comm_data->client_comm_size; i++)
            {
                if ((*data)[i].vect_size > 0)
                {
                    memcpy(&i_buffer[temp_offset], (*data)[i].thresholds[t], (*data)[i].vect_size*sizeof(int));
                    temp_offset += (*data)[i].vect_size;
                }
            }
            temp_offset = 0;
#ifdef BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                for (j=1; j<comm_data->comm_size; j++)
                {
                    temp_offset += local_vect_sizes[j-1];
                    MPI_Recv (&i_buffer[temp_offset], local_vect_sizes[j], MPI_INT, j, j+121, comm_data->comm, &status);
                }
                temp_offset = 0;
            }
            else
            {
                MPI_Send(i_buffer, local_vect_sizes[comm_data->rank], MPI_INT, 0, comm_data->rank+121, comm_data->comm);
            }
#endif // BUILD_WITH_MPI
            if (comm_data->rank == 0)
            {
                f = fopen(file_name, "w");
                for (i=0; i<global_vect_size; i++)
                {
                    fprintf (f, "%d\n", i_buffer[i]);
                }
                fclose(f);
            }
        }
    }

    if (options->quantile_op == 1)
    {
        int value;
        for (value=0; value<options->nb_quantiles; value++)
        {
            for (t=0; t<options->nb_time_steps; t++)
            {
                sprintf(file_name, "results.%s_quantile%g.%.*d", field, options->quantile_order[value], max_size_time, (int)t+1);
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
                        memcpy(&d_buffer[temp_offset], (*data)[i].quantiles[t][value].quantile, (*data)[i].vect_size*sizeof(double));
                        temp_offset += (*data)[i].vect_size;
                    }
                }
                temp_offset = 0;
#ifdef BUILD_WITH_MPI
                if (comm_data->rank == 0)
                {
                    for (j=1; j<comm_data->comm_size; j++)
                    {
                        temp_offset += local_vect_sizes[j-1];
                        MPI_Recv (&d_buffer[temp_offset], local_vect_sizes[j], MPI_DOUBLE, j, j+121, comm_data->comm, &status);
                    }
                    temp_offset = 0;
                }
                else
                {
                    MPI_Send(d_buffer, local_vect_sizes[comm_data->rank], MPI_DOUBLE, 0, comm_data->rank+121, comm_data->comm);
                }
#endif // BUILD_WITH_MPI
                if (comm_data->rank == 0)
                {
                    f = fopen(file_name, "w");
                    for (i=0; i<global_vect_size; i++)
                    {
                        fprintf (f, "%g\n", d_buffer[i]);
                    }
                    fclose(f);
                }
            }
        }
    }

    if (options->sobol_op == 1)
    {
        for (p=0; p<options->nb_parameters; p++)
        {
            for (t=0; t<options->nb_time_steps; t++)
            {
                sprintf(file_name, "results.%s_sobol%d.%.*d", field, p, max_size_time, (int)(t+1));
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
                        memcpy(&d_buffer[temp_offset], (*data)[i].sobol_indices[t].sobol_martinez[p].first_order_values, (*data)[i].vect_size*sizeof(double));
                        temp_offset += (*data)[i].vect_size;
                    }
                }
                temp_offset = 0;
#ifdef BUILD_WITH_MPI
                if (comm_data->rank == 0)
                {
                    for (j=1; j<comm_data->comm_size; j++)
                    {
                        temp_offset += local_vect_sizes[j-1];
                        MPI_Recv (&d_buffer[temp_offset], local_vect_sizes[j], MPI_DOUBLE, j, j+121, comm_data->comm, &status);
                    }
                    temp_offset = 0;
                }
                else
                {
                    MPI_Send(d_buffer, local_vect_sizes[comm_data->rank], MPI_DOUBLE, 0, comm_data->rank+121, comm_data->comm);
                }
#endif // BUILD_WITH_MPI
                if (comm_data->rank == 0)
                {
                    f = fopen(file_name, "w");
                    for (i=0; i<global_vect_size; i++)
                    {
                        fprintf (f, "%g\n", d_buffer[i]);
                    }
                    fclose(f);
                }
            }
        }
        for (p=0; p<options->nb_parameters; p++)
        {
            for (t=0; t<options->nb_time_steps; t++)
            {
                sprintf(file_name, "results.%s_sobol_tot%d.%.*d", field, p, max_size_time, (int)(t+1));
                for (i=0; i<comm_data->client_comm_size; i++)
                {
                    if ((*data)[i].vect_size > 0)
                    {
                        memcpy(&d_buffer[temp_offset], (*data)[i].sobol_indices[t].sobol_martinez[p].total_order_values, (*data)[i].vect_size*sizeof(double));
                        temp_offset += (*data)[i].vect_size;
                    }
                }
                temp_offset = 0;
#ifdef BUILD_WITH_MPI
                if (comm_data->rank == 0)
                {
                    for (j=1; j<comm_data->comm_size; j++)
                    {
                        temp_offset += local_vect_sizes[j-1];
                        MPI_Recv (&d_buffer[temp_offset], local_vect_sizes[j], MPI_DOUBLE, j, j+121, comm_data->comm, &status);
                    }
                    temp_offset = 0;
                }
                else
                {
                    MPI_Send(d_buffer, local_vect_sizes[comm_data->rank], MPI_DOUBLE, 0, comm_data->rank+121, comm_data->comm);
                }
#endif // BUILD_WITH_MPI
                if (comm_data->rank == 0)
                {
                    f = fopen(file_name, "w");
                    for (i=0; i<global_vect_size; i++)
                    {
                        fprintf (f, "%g\n",d_buffer[i]);
                    }
                    fclose(f);
                }
            }
        }
    }
    melissa_free (d_buffer);
    melissa_free (offsets);
    melissa_free (local_vect_sizes);
}

