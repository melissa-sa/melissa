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

    init_covariance (&(sobol_indices->first_order_covariance), vect_size);
    init_covariance (&(sobol_indices->total_order_covariance), vect_size);
    init_variance (&(sobol_indices->variance_k), vect_size);

    sobol_indices->first_order_values = calloc (vect_size, sizeof(double));
    sobol_indices->total_order_values = calloc (vect_size, sizeof(double));
}

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function computes sobol indices using Martinez formula
 *
 *******************************************************************************
 *
 * @param[out] *sobol_array
 * computed sobol indices, using Martinez formula
 *
 * @param[in] nb_parameters
 * size of sobol_array->sobol_martinez
 *
 * @param[in] **in_vect_tab
 * array of input vectors
 *
 * @param[in] *vect_size
 * size of input vectors
 *
 *******************************************************************************/

void increment_sobol_martinez (sobol_array_t *sobol_array,
                               int            nb_parameters,
                               double       **in_vect_tab,
                               int            vect_size)
{
    int i;

    increment_variance (in_vect_tab[0], &(sobol_array->variance_a), vect_size);
    increment_variance (in_vect_tab[1], &(sobol_array->variance_b), vect_size);

    for (i=0; i< nb_parameters; i++){
        increment_variance (in_vect_tab[i+2], &(sobol_array->sobol_martinez[i].variance_k), vect_size);
        increment_covariance (in_vect_tab[1], in_vect_tab[i+2], &(sobol_array->sobol_martinez[i].first_order_covariance), vect_size);
        increment_covariance (in_vect_tab[0], in_vect_tab[i+2], &(sobol_array->sobol_martinez[i].total_order_covariance), vect_size);
    }

#pragma omp parallel for
    for (i=0; i<vect_size; i++)
        sobol_array->sobol_martinez[i].first_order_values[i] = sobol_array->sobol_martinez[i].first_order_covariance.covariance[i]
                                                             / ( sqrt(sobol_array->variance_b.variance[i])
                                                               * sqrt(sobol_array->sobol_martinez[i].variance_k.variance[i]) );

#pragma omp parallel for
    for (i=0; i<vect_size; i++)
        sobol_array->sobol_martinez[i].total_order_values[i] = 1 - sobol_array->sobol_martinez[i].total_order_covariance.covariance[i]
                                                             / ( sqrt(sobol_array->variance_a.variance[i])
                                                               * sqrt(sobol_array->sobol_martinez[i].variance_k.variance[i]) );
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
    free_covariance (&(sobol_indices->first_order_covariance));
    free_covariance (&(sobol_indices->total_order_covariance));
    free_variance (&(sobol_indices->variance_k));
    free (sobol_indices->first_order_values);
    free (sobol_indices->total_order_values);
}
