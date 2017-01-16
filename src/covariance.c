/**
 *
 * @file covariance.c
 * @brief Functions needed to compute covariances.
 * @author Terraz Théophile
 * @date 2016-01-07
 *
 **/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stats.h"

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function initializes a covariance structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *covariance
 * the covariance structure to initialize
 *
 * @param[in] vect_size
 * size of the covariance vector
 *
 *******************************************************************************/

void init_covariance (covariance_t *covariance,
                      const int     vect_size)
{
    covariance->covariance = calloc (vect_size, sizeof(double));
    init_mean (&covariance->mean1, vect_size);
    init_mean (&covariance->mean2, vect_size);
    covariance->increment = 0;
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function updates the incremental covariance.
 *
 *******************************************************************************
 *
 * @param[in] in_vect1[]
 * first input vector of double values
 *
 * @param[in] in_vect2[]
 * second input vector of double values
 *
 * @param[in,out] *covariance
 * input: previously computed covariance,
 * output: incremented covariance
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void increment_covariance (double        in_vect1[],
                           double        in_vect2[],
                           covariance_t *covariance,
                           const int     vect_size)
{
    double  temp;
    int     i;

    covariance->increment += 1;
    if (covariance->increment > 2)
    {
        temp = (double)(covariance->increment - 1) / (double)(covariance->increment);
#pragma omp parallel for
        for (i=0; i<vect_size; i++)
        {
            covariance->covariance[i] *= temp;
            covariance->covariance[i] +=  (in_vect1[i] - covariance->mean1.mean[i]) * (in_vect2[i] - covariance->mean2.mean[i]) / (double)(covariance->increment);
        }
    }
    increment_mean(in_vect1, &(covariance->mean1), vect_size);
    increment_mean(in_vect2, &(covariance->mean2), vect_size);
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function updates the incremental covariance.
 *
 *******************************************************************************
 *
 * @param[in] *covariance1
 * first input partial covariance
 *
 * @param[in] *covariance2
 * second input partial covariance
 *
 * @param[out] *updated_covariance
 * updated covariance
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void update_covariance (covariance_t *covariance1,
                        covariance_t *covariance2,
                        covariance_t *updated_covariance,
                        const int     vect_size)
{
    int     i;

    updated_covariance->increment = covariance1->increment + covariance2->increment;
#pragma omp parallel for
    for (i=0; i<vect_size; i++)
    {
        updated_covariance->covariance[i] = covariance1->covariance[i] + covariance2->covariance[i]
                + ((covariance1->increment * covariance2->increment)/updated_covariance->increment)
                * (covariance2->mean1.mean[i] - covariance1->mean1.mean[i])
                * (covariance2->mean2.mean[i] - covariance1->mean2.mean[i]);
    }
    update_mean (&covariance1->mean1, &covariance2->mean1, &updated_covariance->mean1, vect_size);
    update_mean (&covariance1->mean2, &covariance2->mean2, &updated_covariance->mean2, vect_size);
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function frees a covariance structure.
 *
 *******************************************************************************
 *
 * @param[in] *covariance
 * the covariance structure to free
 *
 *******************************************************************************/

void free_covariance (covariance_t *covariance)
{
    free (covariance->covariance);
    free_mean (&covariance->mean1);
    free_mean (&covariance->mean2);
}
