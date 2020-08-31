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
#include "melissa_utils.h"

int main()
{
    double         *tableau = NULL;
    sobol_array_t   sobol_indices;
    double        **vect_tab;
    int             nb_parameters = 3; // number of parameters
    int             n = 1000; // sampling size
    int             vect_size = 10000; // space size
    int             i, j;
    int             ret = 0;
    double          start_time = 0;
    double          end_time = 0;

    init_sobol_martinez (&sobol_indices, nb_parameters, vect_size);
    tableau = melissa_calloc ((nb_parameters+2) * vect_size, sizeof(double));
    vect_tab = melissa_malloc ((nb_parameters+2) * sizeof(double*));

    for (j=0; j<n; j++)
    {
        for (i=0; i<vect_size * (nb_parameters+2); i++)
        {
            tableau[i] = rand() / (double)RAND_MAX * (1000);
        }
        for (i=0; i<(nb_parameters+2); i++)
        {
            vect_tab[i] = &tableau[vect_size * i];
        }
        start_time = melissa_get_time();
        increment_sobol_martinez (&sobol_indices, nb_parameters, vect_tab, vect_size);
        end_time += melissa_get_time() - start_time;
    }
    fprintf (stdout, "Sobol time: %g\n", end_time);

    melissa_free(vect_tab);
    melissa_free(tableau);
    free_sobol_martinez (&sobol_indices, nb_parameters);

    return ret;
}
