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
 * @file compute_stats.c
 * @brief Functions called by the server.
 * @author Terraz Théophile
 * @date 2016-15-03
 *
 * @defgroup intern_API internal API
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include "melissa_data.h"
#include "melissa_utils.h"

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

void compute_stats (melissa_data_t  *data,
                    const int        time_step,
                    const int        nb_vect,
                    double         **in_vect_tab)
{
    int i;

    if (data->is_valid != 1)
    {
        melissa_print (VERBOSE_ERROR, "Data structure not valid (compute_stats)\n");
        exit (1);
    }

    increment_moments(&(data->moments[time_step]), in_vect_tab[0], data->vect_size);

    if (data->options->min_and_max_op == 1)
    {
        min_and_max (&(data->min_max[time_step]), in_vect_tab[0], data->vect_size);
    }

    if (data->options->threshold_op == 1)
    {
        for (i=0; i<data->options->nb_thresholds; i++)
        {
            update_threshold_exceedance (&(data->thresholds[time_step][i]),
                                         in_vect_tab[0],
                                         data->vect_size);
        }
    }

    if (data->options->quantile_op == 1)
    {
        for (i=0; i<data->options->nb_quantiles; i++)
        {
            increment_quantile (&(data->quantiles[time_step][i]),
                                data->options->sampling_size,
                                in_vect_tab[0],
                                data->vect_size);
        }
    }

    if (data->options->sobol_op == 1)
    {
        if (nb_vect != data->options->nb_parameters + 2)
        {
            melissa_print (VERBOSE_ERROR, "Invalid vector number (compute_stats)\n");
            exit (1);
        }
        data->increment_sobol (&(data->sobol_indices[time_step]),
                               data->options->nb_parameters,
                               in_vect_tab,
                               data->vect_size);

        increment_moments(&(data->moments[time_step]), in_vect_tab[1], data->vect_size);

        if (data->options->min_and_max_op == 1)
        {
            min_and_max (&(data->min_max[time_step]), in_vect_tab[1], data->vect_size);
        }

        if (data->options->threshold_op == 1)
        {
            for (i=0; i<data->options->nb_thresholds; i++)
            {
                update_threshold_exceedance (&(data->thresholds[time_step][i]),
                                             in_vect_tab[1],
                                             data->vect_size);
            }
        }

        if (data->options->quantile_op == 1)
        {
            for (i=0; i<data->options->nb_quantiles; i++)
            {
                increment_quantile (&(data->quantiles[time_step][i]),
                                    data->options->sampling_size,
                                    in_vect_tab[1],
                                    data->vect_size);
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

void finalize_stats (melissa_data_t *data)
{
//    int time_step;
//    for (time_step = 0; time_step<data->options->nb_time_steps; time_step++)
//    {
//        if (data->options->sobol_op != 1 || data->moments[time_step].max_order > 2)
//        {
//            if (data->options->mean_op == 1)
//            {
//                compute_mean (&(data->moments[time_step]), &(data->means[time_step]), data->vect_size);
//            }

//            if (data->options->variance_op == 1)
//            {
//                compute_variance (&(data->moments[time_step]), &(data->variances[time_step]), data->vect_size);
//            }
//        }
//        else
//        {
//            if (data->options->mean_op == 1 && data->options->variance_op == 0)
//            {
//                update_mean(&(data->sobol_indices[time_step].variance_a.mean_structure),
//                            &(data->sobol_indices[time_step].variance_b.mean_structure),
//                            &(data->means[time_step]),
//                            data->vect_size);
//            }

//            if (data->options->variance_op == 1)
//            {
//                update_variance(&(data->sobol_indices[time_step].variance_a),
//                                &(data->sobol_indices[time_step].variance_b),
//                                &(data->variances[time_step]),
//                                data->vect_size);
//            }
//        }
//    }
}
