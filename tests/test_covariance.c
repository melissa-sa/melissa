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
 * @file test_base.c
 * @brief Basic tests to try basic statistic computations.
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mean.h"
#include "variance.h"
#include "covariance.h"
#include "melissa_utils.h"

int main(int argc, char **argv)
{
    double       *tableau1 = NULL, *tableau2 = NULL;
    double       *ref_mean1 = NULL, *ref_mean2 = NULL;
    covariance_t  my_covariance;
    double       *ref_covariance;
    int           n = 1000; // n expériences
    int           vect_size = 10000; // size points de l'espace
    int           i, j;
    int           ret = 0;
    double        start_time = 0;
    double        end_time = 0;

    init_covariance (&my_covariance, vect_size);
    tableau1 = calloc (n * vect_size, sizeof(double));
    tableau2 = calloc (n * vect_size, sizeof(double));
    ref_mean1 = calloc (vect_size, sizeof(double));
    ref_mean2 = calloc (vect_size, sizeof(double));
    ref_covariance = calloc (vect_size, sizeof(double));

    for (j=0; j<vect_size * n; j++)
    {
        tableau1[j] = rand() / (double)RAND_MAX * (1000);
        tableau2[j] = rand() / (double)RAND_MAX * (1000);
    }
    for (i=0; i<vect_size; i++)
    {
        for (j=0; j<n; j++)
        {
            ref_mean1[i] += tableau1[i + j*vect_size];
            ref_mean2[i] += tableau2[i + j*vect_size];
        }
        ref_mean1[i] /= (double)n;
        ref_mean2[i] /= (double)n;
    }
    start_time = melissa_get_time();
    for (j=0; j<n; j++)
    {
        increment_covariance (&my_covariance, &tableau1[j * vect_size], &tableau2[j * vect_size], vect_size);
    }
    end_time = melissa_get_time();
    fprintf (stdout, "covariance time: %g\n", end_time - start_time);

    // covariance //

    for (i=0; i<vect_size; i++)
    {
        for (j=0; j<n; j++)
        {
            ref_covariance[i] += (tableau1[i + j*vect_size] - ref_mean1[i])*(tableau2[i + j*vect_size] - ref_mean2[i]);
        }
        ref_covariance[i] /= (n-1);
    }
    for (i=0; i<vect_size; i++)
    {
        if (fabs((my_covariance.covariance[i] - ref_covariance[i])/ref_covariance[i]) > 10E-12)
        {
            fprintf (stdout, "covariance failed (%g, i=%d)\n", fabs(my_covariance.covariance[i] - ref_covariance[i]), i);
            ret = 1;
        }
    }
    return ret;
}
