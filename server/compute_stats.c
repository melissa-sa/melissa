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
 * @param[in] group_id
 * ID of the simulation providing in_vect_tab
 *
 *******************************************************************************/

void compute_stats (melissa_data_t  *data,
                    const int        time_step,
                    const int        nb_vect,
                    double         **in_vect_tab,
                    const int        group_id)
{
    if (data->is_valid != 1)
    {
        fprintf (stderr, "ERROR: data structure not valid (compute_stats)\n");
        exit (1);
    }

//    if (group_id >= data->step_simu.size)
//    {
//        bits_array_vector_push_to(&data->step_simu, group_id);
//    }

//    if (test_bit (data->step_simu.items[group_id], time_step) != 0)
    if (test_bit (data->step_simu[group_id], time_step) != 0)

    {
        // Time step already computed, message ignored.
        fprintf (stderr, "Warning: allready computed time step (simulation %d, time step %d)\n", group_id, time_step);
        return;
    }

//    increment_moments(&(data->moments[time_step]), in_vect_tab[0], data->vect_size);

    if (data->options->min_and_max_op == 1)
    {
        min_and_max (&(data->min_max[time_step]), in_vect_tab[0], data->vect_size);
    }

    if (data->options->threshold_op == 1)
    {
        update_threshold_exceedance (data->thresholds[time_step], data->options->threshold, in_vect_tab[0], data->vect_size);
    }

    if (data->options->quantile_op == 1)
    {
        increment_quantile (&(data->quantiles[time_step]), data->options->sampling_size, in_vect_tab[0], data->vect_size);
    }

    if (data->options->sobol_op != 1)
    {
        if (data->options->mean_op == 1 && data->options->variance_op == 0)
        {
            increment_mean (&(data->means[time_step]), in_vect_tab[0], data->vect_size);
        }

        if (data->options->variance_op == 1)
        {
            increment_mean_and_variance (&(data->variances[time_step]), in_vect_tab[0], data->vect_size);
        }
    }
    else
    {
        if (nb_vect != data->options->nb_parameters + 2)
        {
            fprintf (stderr, "ERROR: invalid vector number (compute_stats)\n");
            exit (1);
        }
        data->increment_sobol (&(data->sobol_indices[time_step]),
                               data->options->nb_parameters,
                               in_vect_tab,
                               data->vect_size);

        if (data->options->mean_op == 1 && data->options->variance_op == 0)
        {
            update_mean(&(data->sobol_indices[time_step].variance_a.mean_structure),
                        &(data->sobol_indices[time_step].variance_b.mean_structure),
                        &(data->means[time_step]),
                        data->vect_size);
        }

        if (data->options->variance_op == 1)
        {
            update_variance(&(data->sobol_indices[time_step].variance_a),
                            &(data->sobol_indices[time_step].variance_b),
                            &(data->variances[time_step]),
                            data->vect_size);
        }

        if (data->options->min_and_max_op == 1)
        {
            min_and_max (&(data->min_max[time_step]), in_vect_tab[1], data->vect_size);
        }

        if (data->options->threshold_op == 1)
        {
            update_threshold_exceedance (data->thresholds[time_step], data->options->threshold, in_vect_tab[1], data->vect_size);
        }

        if (data->options->quantile_op == 1)
        {
            increment_quantile (&(data->quantiles[time_step]), data->options->sampling_size, in_vect_tab[1], data->vect_size);
        }
    }
    set_bit(data->step_simu[group_id], time_step);
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
//        if (data->options->sobol_op != 1)
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
//    }
    // delete backup files here
}
