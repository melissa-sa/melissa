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

/**
 *
 * @file test_base.c
 * @brief Basic tests to try basic statistic computations.
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 **/

#include "mean.h"
#include "covariance.h"
#include "variance.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    const size_t num_samples = 1000;
    const size_t dimension = 10000;

    covariance_t my_covariance;
    init_covariance (&my_covariance, dimension);

    double* tableau1 = calloc (num_samples * dimension, sizeof(double));
    double* tableau2 = calloc (num_samples * dimension, sizeof(double));
    double* ref_mean1 = calloc (dimension, sizeof(double));
    double* ref_mean2 = calloc (dimension, sizeof(double));
    double* ref_covariance = calloc (dimension, sizeof(double));

    // the code below draws from a uniform distribution in the interval [a,b]
    const double a = 0;
    const double b = 1000;

    for(size_t j = 0; j < dimension * num_samples; ++j)
    {
        tableau1[j] = rand() / (double)RAND_MAX * (b-a) + a;
        tableau2[j] = rand() / (double)RAND_MAX * (b-a) + a;
    }
    for(size_t i = 0; i < dimension; ++i)
    {
        for(size_t j = 0; j < num_samples; ++j)
        {
            ref_mean1[i] += tableau1[i + j*dimension];
            ref_mean2[i] += tableau2[i + j*dimension];
        }
        ref_mean1[i] /= num_samples;
        ref_mean2[i] /= num_samples;
    }

    for(size_t j = 0; j < num_samples; ++j)
    {
        increment_covariance(
            &my_covariance, &tableau1[j * dimension], &tableau2[j * dimension],
            dimension
        );
    }

    for(size_t i = 0; i < dimension; ++i)
    {
        for(size_t j = 0; j < num_samples; ++j)
        {
            ref_covariance[i] +=
                (tableau1[i + j*dimension] - ref_mean1[i]) \
                * (tableau2[i + j*dimension] - ref_mean2[i])
            ;
        }
        ref_covariance[i] /= (num_samples-1);
    }

    int ret = 0;

    for(size_t i = 0; i < dimension; ++i)
    {
        if (fabs((my_covariance.covariance[i] - ref_covariance[i])/ref_covariance[i]) > 10E-12)
        {
            fprintf (stdout, "covariance failed (%g, i=%zu)\num_samples", fabs(my_covariance.covariance[i] - ref_covariance[i]), i);
            ret = 1;
        }
    }
    return ret;
}
