/**
 *
 * @file sobol.c
 * @brief Functions needed to compute sobol indices.
 * @author Terraz Théophile
 * @date 2016-29-02
 *
 **/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stats.h"

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function initialise a sobol index structure
 * @see conditional_variance_s
 *
 *******************************************************************************
 *
 * @param[in,out] *sobol_index
 * input: reference or pointer to an uninitialised sobol index structure,
 * output: initialised structure, with values and conditional variances set to 0
 *
 * @param[in] indices_to_fix[]
 * array of indices to be copied in sobol_index->conditional_variance.fixed_indices
 *
 * @param[in] *data
 * pointer to the structure containing global parameters
 *
 *******************************************************************************/

void init_sobol_index (sobol_index_t *sobol_index,
                       const int      indices_to_fix[],
                       stats_data_t  *data)
{
    int i;

    init_conditional_variance (&(sobol_index->conditional_variance), indices_to_fix, data);

    sobol_index->values = calloc (data->vect_size, sizeof(double));

}

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function compute sobol indices (unfinished) (first order only)
 *
 *******************************************************************************
 *
 * @param[out] *sobol_index
 * computed sobol index
 *
 * @param[in] *data
 * pointer to the structure containing global parameters
 *
 * @param[in] time_step
 * time step
 *
 *******************************************************************************/

void compute_sobol_index (sobol_index_t *sobol_index,
                          stats_data_t  *data,
                          int            time_step)
{
    int i;

    compute_conditional_variance (&(sobol_index->conditional_variance), &(data->cond_means[time_step]), data);

#pragma omp parallel for
    for (i=0; i<data->vect_size; i++)
        sobol_index->values[i] = sobol_index->conditional_variance.variance.variance[i] / data->variances[time_step].variance[i];
}

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function frees a sobol index structure
 *
 *******************************************************************************
 *
 * @param[in] *sobol_index
 * reference or pointer to a sobol index structure to free
 *
 *******************************************************************************/

void free_sobol_index (sobol_index_t *sobol_index)
{
    free_conditional_variance (&(sobol_index->conditional_variance));
    free (sobol_index->values);
}
