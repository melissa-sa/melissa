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
    const size_t vector_length = 1000;
    const size_t num_samples = 10000;

    covariance_t covar;
    init_covariance(&covar, num_samples);

    double* tableau1 = calloc(vector_length * num_samples, sizeof(double));
    double* tableau2 = calloc (vector_length * num_samples, sizeof(double));
    double* expected_mean1 = calloc(num_samples, sizeof(double));
    double* expected_mean2 = calloc(num_samples, sizeof(double));
    double* expected_covariance = calloc(num_samples, sizeof(double));

    // the code below draws from a uniform distribution in the interval [a,b]
    const double a = 0;
    const double b = 1000;

    for(size_t j = 0; j < num_samples * vector_length; ++j)
    {
        tableau1[j] = rand() / (double)RAND_MAX * (b-a) + a;
        tableau2[j] = rand() / (double)RAND_MAX * (b-a) + a;
    }
    for(size_t i = 0; i < num_samples; ++i)
    {
        for(size_t j = 0; j < vector_length; ++j)
        {
            expected_mean1[i] += tableau1[i + j*num_samples];
            expected_mean2[i] += tableau2[i + j*num_samples];
        }
        expected_mean1[i] /= vector_length;
        expected_mean2[i] /= vector_length;
    }

    for(size_t j = 0; j < vector_length; ++j)
    {
        increment_covariance(
            &covar, &tableau1[j * num_samples], &tableau2[j * num_samples],
            num_samples
        );
    }

    for(size_t i = 0; i < num_samples; ++i)
    {
        for(size_t j = 0; j < vector_length; ++j)
        {
            expected_covariance[i] +=
                (tableau1[i + j*num_samples] - expected_mean1[i])
                * (tableau2[i + j*num_samples] - expected_mean2[i])
            ;
        }
        expected_covariance[i] /= (vector_length-1);
    }

    int ret = 0;
    const double tolerance = 1e-11;

    for(size_t i = 0; i < num_samples; ++i)
    {
        double covariance_delta = covar.covariance[i] - expected_covariance[i];
        double relative_error = covariance_delta / expected_covariance[i];

        if(fabs(relative_error) > tolerance)
        {
            fprintf(
                stderr,
                "relative error %8.2e for sample %zu larger than tolerance %8.2e\n",
                fabs(relative_error), i, tolerance
            );
            ret = 1;
        }
    }

    return ret;
}
