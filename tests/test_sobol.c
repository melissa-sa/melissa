/**
 *
 * @file test_sobol.c
 * @brief Basic tests to try sobol indices computations.
 * @author Terraz Théophile
 * @date 2016-15-02
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mean.h"
#include "variance.h"
#include "covariance.h"
#include "sobol.h"

int main(int argc, char **argv)
{
    double        *tableau1 = NULL, *tableau2 = NULL, *tableau3 = NULL;
    sobol_array_t sobol_indices;
    double*        vect_tab[3];
    int            nb_parameters = 3; // number of parameters
    int            n = 5; // sampling size
    int            vect_size = 5; // space size
    int            j;
    int            ret = 0;

    init_sobol_martinez (&sobol_indices, nb_parameters, vect_size);
    tableau1 = calloc (n * vect_size, sizeof(double));
    tableau2 = calloc (n * vect_size, sizeof(double));
    tableau3 = calloc (n * vect_size, sizeof(double));

    for (j=0; j<vect_size * n; j++)
    {
        tableau1[j] = rand() / (double)RAND_MAX * (1000);
        tableau2[j] = rand() / (double)RAND_MAX * (1000);
        tableau3[j] = rand() / (double)RAND_MAX * (1000);
    }
    for (j=0; j<n; j++)
    {
        vect_tab[0] = &tableau1[j*vect_size];
        vect_tab[1] = &tableau2[j*vect_size];
        vect_tab[2] = &tableau3[j*vect_size];
        increment_sobol_martinez (&sobol_indices, nb_parameters, vect_tab, vect_size);
    }

    free_sobol_martinez (&sobol_indices, nb_parameters);

    return ret;
}
