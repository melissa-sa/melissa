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

int main(int argc, char **argv)
{
    double       *tableau = NULL;
    double       *ref_mean = NULL;
    variance_t    my_variance;
    double       *ref_variance;
    int           n = 5; // n expériences
    int           vect_size = 5; // size points de l'espace
    int           i, j;
    int           ret = 0;

    init_variance (&my_variance, vect_size);
    tableau = calloc (n * vect_size, sizeof(double));
    ref_mean = calloc (vect_size, sizeof(double));
    ref_variance = calloc (vect_size, sizeof(double));

    for (j=0; j<vect_size * n; j++)
    {
        tableau[j] = rand() / (double)RAND_MAX * (1000);
    }
    fprintf (stdout, "ref_mean      = ");
    for (i=0; i<vect_size; i++)
    {
        for (j=0; j<n; j++)
        {
            ref_mean[i] += tableau[i + j*vect_size];
        }
        ref_mean[i] /= (double)n;
        fprintf (stdout, "%g ", ref_mean[i]);
    }
    for (j=0; j<n; j++)
    {
        increment_variance (&tableau[j * vect_size], &my_variance, vect_size);
    }
    fprintf (stdout, "\nvariance mean = ");
    for (i=0; i<vect_size; i++)
    {
        fprintf (stdout, "%g ", my_variance.mean_structure.mean[i]);
    }

    // variance //

    fprintf (stdout, "\nref_variance       = ");
    for (i=0; i<vect_size; i++)
    {
        for (j=0; j<n; j++)
        {
            ref_variance[i] += (tableau[i + j*vect_size] - ref_mean[i])*(tableau[i + j*vect_size] - ref_mean[i]);
        }
        ref_variance[i] /= (n-1);
        fprintf (stdout, "%g ", ref_variance[i]);
    }
    fprintf (stdout, "\niterative variance = ");
    for (i=0; i<vect_size; i++)
    {
        fprintf (stdout, "%g ", my_variance.variance[i]);
    }
    fprintf (stdout, "\n");
    for (i=0; i<vect_size; i++)
    {
        if (fabs((my_variance.variance[i] - ref_variance[i])/ref_variance[i]) > 10E-12)
        {
            fprintf (stdout, "variance failed\n");
            ret = 1;
        }
    }
    return ret;
}
