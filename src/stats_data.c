/**
 *
 * @file stats_data.c
 * @brief Routines related to the stats_data structure.
 * @author Terraz Théophile
 * @date 2016-05-24
 *
 * @defgroup stats_data Stats data
 *
 **/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stats.h"


static inline void count_mem_cond_mean (conditional_mean_t *mean,
                                        int                *mem,
                                        int                *count,
                                        stats_data_t       *data)
{
    int i;

    *count += 1;
    *mem += data->vect_size * sizeof(double);
    *mem += data->options->nb_parameters * sizeof(int);
    *mem += 3 * sizeof(int);
    if (mean->is_leaf == 1)
    {
        return;
    }
    *mem += mean->indice_ptr_size * sizeof(int);
    *mem += sizeof(conditional_mean_t);
    for (i=0; i<mean->indice_ptr[mean->indice_ptr_size-1]; i++)
    {
        count_mem_cond_mean (&mean->next_conditional_means[i],
                             mem,
                             count,
                             data);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_data
 *
 * This function computes and displays the memory consumption of the data structure
 *
 *******************************************************************************
 *
 * @param[in] *data
 * pointer to the structure containing global parameters
 *
 *******************************************************************************/

void mem_conso (stats_data_t *data)
{
    int i = 0;
    int memory = 0;
    int temp_mem = 0;

    if (data->options->mean_op != 0)
    {
        temp_mem = data->vect_size * data->options->nb_time_steps * sizeof(double) + sizeof(int);
        fprintf(stdout, "Mean memory usage: %d bytes\n", temp_mem);
        memory += temp_mem;
    }
    if (data->options->variance_op != 0 && data->options->mean_op == 0)
    {
        temp_mem = 2 * data->vect_size * data->options->nb_time_steps * sizeof(double) + sizeof(int);
        fprintf(stdout, "variance memory usage: %d bytes\n", temp_mem);
        memory += temp_mem;
    }
    if (data->options->variance_op != 0 && data->options->mean_op != 0)
    {
        temp_mem = data->vect_size * data->options->nb_time_steps * sizeof(double) + sizeof(int);
        fprintf(stdout, "Variance memory usage: %d bytes\n", temp_mem);
        memory += temp_mem;
    }
    if (data->options->min_and_max_op != 0)
    {
        temp_mem = 2 * data->vect_size * data->options->nb_time_steps * sizeof(double) + sizeof(int);
        fprintf(stdout, "Min and max memory usage: %d bytes\n", temp_mem);
        memory += temp_mem;
    }
    if (data->options->threshold_op != 0)
    {
        temp_mem = data->vect_size * data->options->nb_time_steps * sizeof(int);
        fprintf(stdout, "Threshold exceedance memory usage: %d bytes\n", temp_mem);
        memory += temp_mem;
    }
    if (data->options->sobol_op != 0)
    {
        temp_mem = 0;
        count_mem_cond_mean(&(data->cond_means[0]), &temp_mem, &i, data);
        fprintf(stdout, "Partial means memory usage: %d bytes\n", temp_mem);
        fprintf(stdout, "Number of partial means: %d\n", i);
        memory += temp_mem;
    }
    fprintf(stdout, "Total memory usage: %d bytes\n", memory);
}

static void malloc_data (stats_data_t *data)
{
    int i;

    if (data->is_valid != 1)
    {
        fprintf (stderr, "ERROR: data structure not valid (malloc_data)\n");
        exit (1);
    }

    if (data->options->mean_op == 1 && data->options->variance_op == 0)
    {
        data->means = malloc (data->options->nb_time_steps * sizeof(mean_t));
        for (i=0; i<data->options->nb_time_steps; i++)
            init_mean (&(data->means[i]), data->vect_size);
    }

    if (data->options->variance_op == 1)
    {
        data->variances = malloc (data->options->nb_time_steps * sizeof(variance_t));
        for (i=0; i<data->options->nb_time_steps; i++)
            init_variance (&(data->variances[i]), data->vect_size);
    }

    if (data->options->min_and_max_op == 1)
    {
        data->min_max = malloc (data->options->nb_time_steps * sizeof(min_max_t));
        for (i=0; i<data->options->nb_time_steps; i++)
            init_min_max (&(data->min_max[i]), data->vect_size);
    }

    if (data->options->threshold_op == 1)
    {
        data->thresholds = malloc (data->options->nb_time_steps * sizeof(int*));
        for (i=0; i<data->options->nb_time_steps; i++)
            data->thresholds[i] = calloc (data->vect_size, sizeof(int));
    }

    if (data->options->sobol_op == 1)
    {
        data->cond_means = malloc (data->options->nb_time_steps * sizeof(conditional_mean_t));
        for (i=0; i<data->options->nb_time_steps; i++)
            init_conditional_means (&(data->cond_means[i]), data);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_data
 *
 * This function initializes the data structure
 *
 *******************************************************************************
 *
 * @param[out] *data
 * pointer to the structure containing global parameters
 *
 * @param[in] *options
 * pointer to the structure containing options parsed from command line
 *
 * @param[in] vect_size
 * sise of the local input vector
 *
 *******************************************************************************/

void init_data (stats_data_t    *data,
                stats_options_t *options,
                int              vect_size)
{
    data->vect_size       = vect_size;
    data->options         = options;
    data->is_valid        = 0;
    data->means           = NULL;
    data->variances       = NULL;
    data->min_max         = NULL;
    data->thresholds      = NULL;
    data->cond_means      = NULL;
    data->sobol_indices   = NULL;
    check_data (data);
    malloc_data (data);
}

/**
 *******************************************************************************
 *
 * @ingroup stats_data
 *
 * This function checks the data structure. It terminates the program if one of
 * the mandatory option is missing or invalid, and correct mistakes for other options
 *
 *******************************************************************************
 *
 * @param[in,out] *data
 * pointer to the structure containing global parameters
 *
 *******************************************************************************/

void check_data (stats_data_t *data)
{
    int i, ret;

    // check errors

    if (data->vect_size < 1)
    {
        fprintf (stderr, "BAD PARAMETER: vector_size must have at least 1\n");
        ret = ERROR_BAD_PARAMETER;
    }

    if (data->options->nb_time_steps < 1)
    {
        fprintf (stderr, "BAD PARAMETER: study must have at least 1 time step\n");
        ret = ERROR_BAD_PARAMETER;
    }

    if (data->options->nb_parameters < 1)
    {
        fprintf (stderr, "BAD PARAMETER: study must have at least 1 variable parameter\n");
        ret = 1;
    }
    else
    {
        for (i=0; i<data->options->nb_parameters; i++)
        {
            if (data->options->size_parameters[i] < 1)
            {
                fprintf (stderr, "BAD PARAMETER: parameter size must be at least 1\n");
                ret = ERROR_BAD_PARAMETER;
            }
        }
    }

    if (ret == ERROR_BAD_PARAMETER)
        exit (ERROR_BAD_PARAMETER);

    // check consistency

    if (data->options->mean_op == 0 &&
        data->options->variance_op == 0 &&
        data->options->min_and_max_op == 0 &&
        data->options->threshold_op == 0 &&
        data->options->sobol_op == 0)
    {
        // default values
        fprintf (stdout, "WARNING: no operation given, set to mean and variance\n");
        data->options->mean_op == 1;
        data->options->variance_op == 1;
    }

    if (data->options->sobol_op != 0 && data->options->variance_op == 0)
    {
        // variance needed in sobol index computation
        data->options->variance_op = 1;
    }

    if (data->options->sobol_op != 0 && data->options->sobol_order > data->options->nb_parameters - 1)
    {
        data->options->sobol_order = data->options->nb_parameters - 1;
        fprintf (stdout, "WARNING: max sobol order set to %d\n", data->options->sobol_order);
    }

    if (data->options->sobol_op != 0 && data->options->sobol_order < 1)
    {
        // default value
        data->options->sobol_order = 1;
        fprintf (stdout, "WARNING: max sobol order set to 1\n");
    }

    // every parameter is now validated
    data->is_valid = 1;
}

/**
 *******************************************************************************
 *
 * @ingroup stats_data
 *
 * This function frees the memory in the data structure
 *
 *******************************************************************************
 *
 * @param[in] *data
 * pointer to the structure containing global parameters
 *
 *******************************************************************************/

void free_data (stats_data_t *data)
{
    int i;

    if (data->is_valid != 1)
    {
        fprintf (stderr, "ERROR: data structure not valid (free_data)\n");
        exit (1);
    }

    if (data->options->mean_op == 1 && data->options->variance_op == 0)
    {
        for (i=0; i<data->options->nb_time_steps; i++)
            free_mean (&(data->means[i]));
        free (data->means);
    }

    if (data->options->variance_op == 1)
    {
        for (i=0; i<data->options->nb_time_steps; i++)
            free_variance (&(data->variances[i]));
        free (data->variances);
    }

    if (data->options->min_and_max_op == 1)
    {
        for (i=0; i<data->options->nb_time_steps; i++)
            free_min_max (&(data->min_max[i]));
        free (data->min_max);
    }

    if (data->options->threshold_op == 1)
    {
        for (i=0; i<data->options->nb_time_steps; i++)
            free (data->thresholds[i]);
        free (data->thresholds);
    }

    if (data->options->sobol_op == 1)
    {
        for (i=0; i<data->options->nb_time_steps; i++)
            free_conditional_mean (&(data->cond_means[i]));
        free (data->cond_means);
        for (i=0; i<data->options->nb_time_steps; i++)
            free_sobol_index (data->sobol_indices[i].sobol_indices);
        free (data->sobol_indices);
    }

    data->options = NULL;

    data->is_valid = 0;
}
