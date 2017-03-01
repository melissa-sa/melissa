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
    mean_t        my_mean;
    int           n = 5; // n expériences
    int           vect_size = 5; // size points de l'espace
    int           i, j;
    int           ret = 0;

    init_mean (&my_mean, vect_size);
    tableau = calloc (n * vect_size, sizeof(double));
    ref_mean = calloc (vect_size, sizeof(double));

    for (j=0; j<vect_size * n; j++)
    {
        tableau[j] = rand() / (double)RAND_MAX * (1000);
    }
    fprintf (stdout, "ref mean       = ");
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
        increment_mean (&tableau[j * vect_size], &my_mean, vect_size);
    }
    fprintf (stdout, "\niterative mean = ");
    for (i=0; i<vect_size; i++)
    {
        fprintf (stdout, "%g ", my_mean.mean[i]);
    }
    fprintf (stdout, "\n");
    for (i=0; i<vect_size; i++)
    {
        if (fabs((my_mean.mean[i] - ref_mean[i])/ref_mean[i]) > 10E-12)
        {
            fprintf (stdout, "\nmean failed (iterative mean = %g, ref mean = %g, i=%d)\n", my_mean.mean[i], ref_mean[i], i);
            ret = 1;
        }
    }
    return ret;
}
