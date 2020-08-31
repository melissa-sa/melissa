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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mean.h"
#include "variance.h"
#include "general_moments.h"
#include "covariance.h"
#include "melissa_utils.h"

int main()
{
    double       *tableau = NULL;
    double       *ref_mean = NULL;
    variance_t    my_variance;
    variance_t    my_other_variance;
    moments_t     my_moments;
    double       *ref_variance;
    int           n = 10000; // n expériences
    int           vect_size = 1000; // size points de l'espace
    int           i, j;
    int           ret = 0;
    double        start_time = 0;
    double        end_time = 0;

    init_variance (&my_variance, vect_size);
    init_variance (&my_other_variance, vect_size);
    init_moments (&my_moments, vect_size, 2);
    tableau = melissa_calloc (n * vect_size, sizeof(double));
    ref_mean = melissa_calloc (vect_size, sizeof(double));
    ref_variance = melissa_calloc (vect_size, sizeof(double));

    for (j=0; j<vect_size * n; j++)
    {
        tableau[j] = rand() / (double)RAND_MAX * (1000);
    }
    for (i=0; i<vect_size; i++)
    {
        for (j=0; j<n; j++)
        {
            ref_mean[i] += tableau[i + j*vect_size];
        }
        ref_mean[i] /= (double)n;
    }
    start_time = melissa_get_time();
    for (j=0; j<n; j++)
    {
        increment_variance (&my_variance, &tableau[j * vect_size], vect_size);
        increment_moments (&my_moments, &tableau[j * vect_size], vect_size);
        // More stable algo :
        for (i=0; i<vect_size; i++)
        {
            double temp = my_other_variance.mean_structure.mean[i];
            my_other_variance.mean_structure.mean[i] = my_other_variance.mean_structure.mean[i] + (tableau[j * vect_size + i] - my_other_variance.mean_structure.mean[i]) / (my_other_variance.mean_structure.increment + 1);
            my_other_variance.variance[i] = my_other_variance.variance[i] + my_other_variance.mean_structure.increment * (tableau[j * vect_size + i] - temp) * (tableau[j * vect_size + i] - temp) / (my_other_variance.mean_structure.increment + 1);
        }
        my_other_variance.mean_structure.increment += 1;
    }
    end_time = melissa_get_time();
    fprintf (stdout, "variance time: %g\n", end_time - start_time);

    // variance //

    for (i=0; i<vect_size; i++)
    {
        for (j=0; j<n; j++)
        {
            ref_variance[i] += (tableau[i + j*vect_size] - ref_mean[i])*(tableau[i + j*vect_size] - ref_mean[i]);
        }
    }
    for (i=0; i<vect_size; i++)
    {
        if (fabs((my_variance.variance[i] - (ref_variance[i] / (n-1)))/(ref_variance[i] / (n-1))) > 10E-12)
        {
            fprintf (stdout, "variance failed\n");
            ret = 1;
        }
    }
    compute_variance (&my_moments, my_variance.variance, vect_size);
    for (i=0; i<vect_size; i++)
    {
        if (fabs((my_variance.variance[i] - (ref_variance[i] / (n-1)))/(ref_variance[i] / (n-1))) > 10E-12)
        {
            fprintf (stdout, "variance failed (moments)\n");
            ret = 1;
        }
    }

    for (i=0; i<vect_size; i++)
    {
        if (fabs((my_other_variance.variance[i]/(my_other_variance.mean_structure.increment-1) - (ref_variance[i] / (n-1)))/(ref_variance[i] / (n-1))) > 10E-12)
        {
            fprintf (stdout, "other variance failed (%g and %g)\n", my_other_variance.variance[i]/my_other_variance.mean_structure.increment, ref_variance[i] / n);
            ret = 1;
        }
    }

    free_variance(&my_variance);
    free_variance(&my_other_variance);
    melissa_free(tableau);
    melissa_free(ref_mean);
    melissa_free(ref_variance);
    return ret;
}
