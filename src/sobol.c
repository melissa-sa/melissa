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
 * This function initialise a Martinez sobol indices structure
 *
 *******************************************************************************
 *
 * @param[in,out] *sobol_indices
 * input: reference or pointer to an uninitialised sobol indices structure,
 * output: initialised structure, with values and variances set to 0
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void init_sobol_martinez (sobol_martinez_t *sobol_indices,
                          int               vect_size)
{
    int i;

    init_covariance (&(sobol_indices->covariance), vect_size);
    init_variance (&(sobol_indices->variance1), vect_size);
    init_variance (&(sobol_indices->variance2), vect_size);

    sobol_indices->values = calloc (vect_size, sizeof(double));
}

//void compute_sobol_index (sobol_index_t *sobol_index,
//                          stats_data_t  *data,
//                          int            time_step)
//{
//    int i;

//    compute_conditional_variance (&(sobol_index->conditional_variance), &(data->cond_means[time_step]), data);

//#pragma omp parallel for
//    for (i=0; i<data->vect_size; i++)
//        sobol_index->values[i] = sobol_index->conditional_variance.variance.variance[i] / data->variances[time_step].variance[i];
//}

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function compute sobol indices using Martinez formula
 *
 *******************************************************************************
 *
 * @param[out] *sobol_indices
 * computed sobol indices, using Martinez formula
 *
 * @param[in] Yb[]
 * first input vector
 *
 * @param[in] Yck[]
 * second input vector
 *
 * @param[in] *vect_size
 * size of input vectors
 *
 *******************************************************************************/

void increment_sobol_martinez (sobol_martinez_t *sobol_indices,
                               double            Yb[],
                               double            Yck[],
                               int               vect_size)
{
    int i;

    increment_covariance (Yb, Yck, &(sobol_indices->covariance), vect_size);
    increment_variance (Yb, &(sobol_indices->variance1), vect_size);
    increment_variance (Yck, &(sobol_indices->variance2), vect_size);

#pragma omp parallel for
    for (i=0; i<vect_size; i++)
        sobol_indices->values[i] = sobol_indices->covariance.covariance[i]
                         / ( sqrt(sobol_indices->variance1.variance[i])
                           * sqrt(sobol_indices->variance2.variance[i]) );
}

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function frees a Martinez sobol indices structure
 *
 *******************************************************************************
 *
 * @param[in] *sobol_indices
 * reference or pointer to a sobol index structure to free
 *
 *******************************************************************************/

void free_sobol_martinez (sobol_martinez_t *sobol_indices)
{
    free_covariance (&(sobol_indices->covariance));
    free_variance (&(sobol_indices->variance1));
    free_variance (&(sobol_indices->variance2));
    free (sobol_indices->values);
}
