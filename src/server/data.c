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
#include <melissa/stats/sobol.h>
#include <melissa/utils.h>

#include <stdio.h>
#include <stdlib.h>

static void melissa_init_steps (melissa_data_t *data)
{
    int      i;

    if (data->is_valid != 1)
    {
        melissa_print (VERBOSE_ERROR, "Data structure not valid (malloc_data)\n");
        exit (1);
    }

    if (data->steps_init == 0)
    {
        alloc_vector (&data->step_simu, data->options->sampling_size);
        for (i=0; i<data->options->sampling_size; i++)
        {
            vector_add (&data->step_simu, melissa_calloc((data->options->nb_time_steps+31)/32, sizeof(int32_t)));
        }
        data->steps_init = 1;
    }
}

static void melissa_alloc_data (melissa_data_t *data)
{
    int      i, j;

    if (data->is_valid != 1)
    {
        melissa_print (VERBOSE_ERROR, "Data structure not valid (malloc_data)\n");
        exit (1);
    }

    if (data->steps_init == 0)
    {
        melissa_init_steps (data);
    }

    if (data->vect_size <= 0)
    {
        return;
    }

    data->moments = melissa_malloc (data->options->nb_time_steps * sizeof(moments_t));

    if (data->options->kurtosis_op == 1)
    {
        j = 4;
    }
    else if (data->options->skewness_op == 1)
    {
        j = 3;
    }
    else if (data->options->variance_op == 1)
    {
        j = 2;
    }
    else if (data->options->mean_op == 1)
    {
        j = 1;
    }
    else
    {
        j = 0;
    }

    for (i=0; i<data->options->nb_time_steps; i++)
        init_moments (&(data->moments[i]), data->vect_size, j);

    if (data->options->min_and_max_op == 1)
    {
        data->min_max = melissa_malloc (data->options->nb_time_steps * sizeof(min_max_t));
        for (i=0; i<data->options->nb_time_steps; i++)
            init_min_max (&(data->min_max[i]), data->vect_size);
    }

    if (data->options->threshold_op == 1)
    {
        data->thresholds = melissa_malloc (data->options->nb_time_steps * sizeof(threshold_t*));
        for (i=0; i<data->options->nb_time_steps; i++)
        {
            data->thresholds[i] = melissa_calloc (data->options->nb_thresholds, sizeof(threshold_t));
            for (j=0; j<data->options->nb_thresholds; j++)
            {
                init_threshold (&(data->thresholds[i][j]), data->vect_size, data->options->threshold[j]);
            }
        }
    }

    if (data->options->quantile_op == 1)
    {
        data->quantiles = melissa_malloc (data->options->nb_time_steps * sizeof(quantile_t*));
        for (i=0; i<data->options->nb_time_steps; i++)
        {
            data->quantiles[i] = melissa_malloc (data->options->nb_quantiles * sizeof(quantile_t));
            for (j=0; j<data->options->nb_quantiles; j++)
            {
                init_quantile (&(data->quantiles[i][j]), data->vect_size, data->options->quantile_order[j]);
            }
        }
    }

    if (data->options->sobol_op == 1)
    {
        data->sobol_indices = melissa_malloc (data->options->nb_time_steps * sizeof(sobol_array_t));
        data->init_sobol = init_sobol_martinez;
        data->read_sobol = read_sobol_martinez;
        data->save_sobol = save_sobol_martinez;
        data->increment_sobol = increment_sobol_martinez;
        data->free_sobol = free_sobol_martinez;
        for (i=0; i<data->options->nb_time_steps; i++)
        {
            data->init_sobol (&data->sobol_indices[i], data->options->nb_parameters, data->vect_size);
        }
    }
    data->stats_init = 1;
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_data
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

void melissa_init_data (melissa_data_t    *data,
                        melissa_options_t *options,
                        int                vect_size)
{
    data->vect_size       = vect_size;
    data->options         = options;
    data->moments         = NULL;
    data->min_max         = NULL;
    data->thresholds      = NULL;
    data->quantiles       = NULL;
    data->sobol_indices   = NULL;
    melissa_check_data (data);
    if (vect_size > 0)
    {
        melissa_alloc_data (data);
    }
    else
    {
        melissa_init_steps (data);
    }
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

void melissa_check_data (melissa_data_t *data)
{
    if (data->is_valid != 1)
    {
        // check options
        melissa_check_options(data->options);

        // every parameter is now validated
        data->is_valid = 1;
    }
}

/**
 *******************************************************************************
 *
 * @ingroup melissa_data
 *
 * This function frees the memory in the data structure
 *
 *******************************************************************************
 *
 * @param[in] *data
 * pointer to the structure containing global parameters
 *
 *******************************************************************************/

void melissa_free_data (melissa_data_t *data)
{
    int i, j;

    if (data->is_valid != 1)
    {
        melissa_print (VERBOSE_ERROR, "Data structure not valid (free_data)\n");
        exit (1);
    }

    for (i=0; i<data->options->nb_time_steps; i++)
        free_moments (&(data->moments[i]));
    melissa_free (data->moments);

//    if (data->options->mean_op == 1 && data->options->variance_op == 0)
//    {
//        for (i=0; i<data->options->nb_time_steps; i++)
//            free_mean (&(data->means[i]));
//        melissa_free (data->means);
//    }

//    if (data->options->variance_op == 1)
//    {
//        for (i=0; i<data->options->nb_time_steps; i++)
//            free_variance (&(data->variances[i]));
//        melissa_free (data->variances);
//    }

    if (data->options->min_and_max_op == 1)
    {
        for (i=0; i<data->options->nb_time_steps; i++)
            free_min_max (&(data->min_max[i]));
        melissa_free (data->min_max);
    }

    if (data->options->threshold_op == 1)
    {
        for (i=0; i<data->options->nb_time_steps; i++)
        {
            for (j=0; j<data->options->nb_thresholds; j++)
            {
                free_threshold (&(data->thresholds[i][j]));
            }
            melissa_free (data->thresholds[i]);
        }
        melissa_free (data->thresholds);
    }

    if (data->options->quantile_op == 1)
    {
        for (i=0; i<data->options->nb_time_steps; i++)
        {
            for (j=0; j<data->options->nb_quantiles; j++)
            {
                free_quantile (&(data->quantiles[i][j]));
            }
            melissa_free (data->quantiles[i]);
        }
        melissa_free (data->quantiles);
    }

    if (data->options->sobol_op == 1)
    {
        for (i=0; i<data->options->nb_time_steps; i++)
        {
            data->free_sobol (&data->sobol_indices[i], data->options->nb_parameters);
        }
        melissa_free (data->sobol_indices);
    }

    for (i=0; i<data->step_simu.size; i++)
    {
        melissa_free(vector_get(&data->step_simu, i));
    }
    free_vector (&data->step_simu);

    data->options = NULL;

    data->is_valid = 0;
}
