/**
 *
 * @file save_data.c
 * @brief Functions needed to write and read Melissa structures on disc.
 * @author Terraz Théophile
 * @date 2016-27-10
 *
 * @defgroup save_stats Backup functions
 *
 **/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stats.h"
#include "mean.h"
#include "variance.h"
#include "covariance.h"
#include "sobol.h"

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function writes the option structure on disc
 *
 *******************************************************************************
 *
 * @param[in] *options
 * pointer to the structure containing global options
 *
 *******************************************************************************/

void write_options (stats_options_t *options)
{
    FILE* f;

    f = fopen("options.save", "wb+");

    fwrite(options, sizeof(*options), 1, f);

    fclose(f);
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function reads a saved option structure on disc
 *
 *******************************************************************************
 *
 * @param[in,out] *options
 * pointer to the structure containing global options
 *
 *******************************************************************************/

int read_options (stats_options_t *options)
{
    FILE* f = NULL;
    int ret = 1;

    f = fopen("options.save", "rb");

    if (f != NULL)
    {
        if (1 == fread(options, sizeof(*options), 1, f));
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
 * @ingroup save_stats
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
 * @ingroup save_stats
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
 * @ingroup save_stats
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

void save_stats (stats_data_t *data,
                 comm_data_t  *comm_data,
                 char         *field_name)
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
 * @ingroup save_stats
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

void read_saved_stats (stats_data_t *data,
                       comm_data_t  *comm_data,
                       char         *field_name,
                       int           client_rank)
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
