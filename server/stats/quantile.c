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
 * @file quantile.c
 * @brief Quantile related functions.
 * @author Terraz Théophile
 * @date 2017-18-05
 *
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#ifdef BUILD_WITH_OPENMP
#include <omp.h>
#endif // BUILD_WITH_OPENMP
#include "quantile.h"
#include "melissa_utils.h"

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function initializes a quantile structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *quantile
 * the quantile structure to initialize
 *
 * @param[in] vect_size
 * size of the quantile vector
 *
 * @param[in] alpha
 * the quantile order
 *
 *******************************************************************************/

void init_quantile (quantile_t   *quantile,
                    const int     vect_size,
                    const double  alpha)
{
    quantile->quantile = melissa_calloc (vect_size, sizeof(double));
    quantile->increment = 0;
    quantile->alpha = alpha;
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function updates the incremental quantile.
 *
 *******************************************************************************
 *
 * @param[in,out] *quantile
 * input: previously computed iterative quantile,
 * output: updated partial quantile
 *
 * @param[in] nmax
 * maximum number of iterations
 *
 * @param[in] in_vect[]
 * input vector of double values
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void increment_quantile (quantile_t *quantile,
                         const int   nmax,
                         double      in_vect[],
                         const int   vect_size)
{
    int    i;
    double temp, gamma;

    quantile->increment += 1;

    if (quantile->increment > 1)
    {
#pragma omp parallel for schedule(static) private(temp)
        for (i=0; i<vect_size; i++)
        {
            gamma = (quantile->increment - 1) * 0.9 / (nmax-1) + 0.1;
            if (quantile->quantile[i] >= in_vect[i])
            {
                temp = 1 - quantile->alpha;
            }
            else
            {
                temp = 0 - quantile->alpha;
            }
            quantile->quantile[i] -= temp/pow(quantile->increment, gamma);
        }
    }
    else
    {
//#pragma omp parallel for schedule(static)
//        for (i=0; i<vect_size; i++)
//        {
//            quantile->quantile[i] = in_vect[i];
//        }
        memcpy (quantile->quantile, in_vect, vect_size * sizeof(double));
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function writes an array of quantile structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *quantiles
 * quantile structures to save, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] nb_quantiles
 * number of quantile values
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void save_quantile(quantile_t **quantiles,
                   int          vect_size,
                   int          nb_time_steps,
                   int          nb_quantiles,
                   FILE*        f)
{
    int i, j;
    for (i=0; i<nb_time_steps; i++)
    {
        for (j=0; j<nb_quantiles; j++)
        {
            fwrite(quantiles[i][j].quantile, sizeof(double), vect_size, f);
            fwrite(&quantiles[i][j].increment, sizeof(int), 1, f);
            fwrite(&quantiles[i][j].alpha, sizeof(double), 1, f);
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function reads an array of quantile structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *quantiles
 * quantile structures to read, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] nb_quantiles
 * number of quantile values
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void read_quantile(quantile_t **quantiles,
                   int          vect_size,
                   int          nb_time_steps,
                   int          nb_quantiles,
                   FILE*        f)
{
    int i, j;
    for (i=0; i<nb_time_steps; i++)
    {
        for (j=0; j<nb_quantiles; j++)
        {
            fread(quantiles[i][j].quantile, sizeof(double), vect_size, f);
            fread(&quantiles[i][j].increment, sizeof(int), 1, f);
            fread(&quantiles[i][j].alpha, sizeof(double), 1, f);
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function frees a quantile structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *quantile
 * the quantile structure to free
 *
 *******************************************************************************/

void free_quantile (quantile_t *quantile)
{
    melissa_free (quantile->quantile);
}
