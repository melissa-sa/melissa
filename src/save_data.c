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
#include <stdio.h>
#include "stats.h"

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

void write_options (const stats_options_t *options)
{
    FILE*      f;

    fopen("options.save", "w");

    fprintf(f, "%d%d%d%d%d%d%d%d%g%d%d%d%d\n",
            options->nb_time_steps,
            options->nb_parameters,
            options->nb_groups,
            options->nb_simu,
            options->mean_op,
            options->variance_op,
            options->min_and_max_op,
            options->threshold_op,
            options->threshold,
            options->sobol_op,
            options->sobol_order,
            options->global_vect_size,
            options->restart);

    close(f);
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

void read_options (stats_options_t  *options)
{
    FILE*      f;

    fopen("options.save", "r");

    fread(&options->nb_time_steps,    sizeof(int),    1, f);
    fread(&options->nb_parameters,    sizeof(int),    1, f);
    fread(&options->nb_groups,        sizeof(int),    1, f);
    fread(&options->nb_simu,          sizeof(int),    1, f);
    fread(&options->mean_op,          sizeof(int),    1, f);
    fread(&options->variance_op,      sizeof(int),    1, f);
    fread(&options->min_and_max_op,   sizeof(int),    1, f);
    fread(&options->threshold_op,     sizeof(int),    1, f);
    fread(&options->threshold,        sizeof(double), 1, f);
    fread(&options->sobol_op,         sizeof(int),    1, f);
    fread(&options->sobol_order,      sizeof(int),    1, f);
    fread(&options->global_vect_size, sizeof(int),    1, f);
    fread(&options->restart,          sizeof(int),    1, f);

    close(f);
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function writes an array of mean structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *means
 * mean structures to save, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void write_mean(mean_t *means,
                int     vect_size,
                int     nb_time_steps,
                FILE*   f)
{
    int i, j;
    for (i=0; i<nb_time_steps; i++)
    {
        for (j=0; j<vect_size; j++)
        {
            fprintf(f, "%g\n", means[i].mean[j]);
        }
        fprintf(f, "%d\n", means[i].increment);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function reads an array of mean structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *means
 * mean structures to read, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void read_mean(mean_t *means,
               int     vect_size,
               int     nb_time_steps,
               FILE*   f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fread(means[i].mean, sizeof(double), vect_size, f);
        fread(&means[i].increment, sizeof(int), 1, f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function writes an array of variances structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *vars
 * variance structures to save, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void write_variance(variance_t *vars,
                    int         vect_size,
                    int         nb_time_steps,
                    FILE*       f)
{
    int i, j;
    for (i=0; i<nb_time_steps; i++)
    {
        for (j=0; j<vect_size; j++)
        {
            fprintf(f, "%g\n", vars[i].variance[j]);
            write_mean (&vars[i].mean_structure, vect_size, 1, f);
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function reads an array of variances structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *vars
 * mean structures to read, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void read_variance(variance_t *vars,
                   int         vect_size,
                   int         nb_time_steps,
                   FILE*       f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fread(vars[i].variance, sizeof(double), vect_size, f);
        read_mean (&vars->mean_structure, vect_size, nb_time_steps,f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function writes an array of min and max structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *minmax
 * min and max structures to save, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void write_min_max(min_max_t *minmax,
                   int        vect_size,
                   int        nb_time_steps,
                   FILE*      f)
{
    int i, j;
    for (i=0; i<nb_time_steps; i++)
    {
        for (j=0; j<vect_size; j++)
        {
            fprintf(f, "%g\n", minmax[i].min[j]);
            fprintf(f, "%g\n", minmax[i].max[j]);
        }
        fprintf(f, "%d\n", minmax[i].is_init);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function reads an array of min and max structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *minmax
 * min and max structures to read, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void read_min_max(min_max_t *minmax,
                  int        vect_size,
                  int        nb_time_steps,
                  FILE*      f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fread(minmax[i].min, sizeof(double), vect_size, f);
        fread(minmax[i].max, sizeof(double), vect_size, f);
        fread(&minmax[i].is_init, sizeof(int), 1, f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function writes an array of threshold exceedance vectors on disc
 *
 *******************************************************************************
 *
 * @param[in] **threshold
 * threshold exceedance array of vectors to save, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void write_threshold(int  **threshold,
                     int    vect_size,
                     int    nb_time_steps,
                     FILE*  f)
{
    int i, j;
    for (i=0; i<nb_time_steps; i++)
    {
        for (j=0; j<vect_size; j++)
        {
            fprintf(f, "%d\n", threshold[i][j]);
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function reads an array of threshold exceedance vectors on disc
 *
 *******************************************************************************
 *
 * @param[in] **threshold
 * threshold exceedance array of vectors to read, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void read_threshold(int  **threshold,
                    int    vect_size,
                    int    nb_time_steps,
                    FILE*  f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fread(threshold[i], sizeof(int), vect_size, f);
    }
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
            f = fopen(file_name, "w");
            fprintf(f, "%d\n", data[i].vect_size);
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
            fprintf(f, "%d\n", data[i].computed);
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
 *******************************************************************************/

void read_saved_stats (stats_data_t *data,
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
            f = fopen(file_name, "w");
            fread(&data[i].vect_size, sizeof(int), 1, f);
            if (data[i].options->mean_op != 0 && data[i].options->variance_op == 0)
            {
                read_mean(data[i].means, data[i].vect_size, data[i].options->nb_time_steps, f);
            }
            if (data[i].options->variance_op != 0)
            {
                read_variance(data[i].variances, data[i].vect_size, data[i].options->nb_time_steps, f);
            }
            if (data[i].options->min_and_max_op != 0)
            {
                read_min_max(data[i].min_max, data[i].vect_size, data[i].options->nb_time_steps, f);
            }
            if (data[i].options->threshold_op != 0)
            {
                read_threshold(data[i].thresholds, data[i].vect_size, data[i].options->nb_time_steps, f);
            }
            if (data[i].options->sobol_op != 0)
            {
//                TODO
//                read_sobol(data->thresholds, data->vect_size, data->options->nb_time_steps, f);
            }
            fread(&data->computed, sizeof(int), 1, f);
        }
    }
}
