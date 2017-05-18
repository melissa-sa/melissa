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
 * alpha parameter of the algotithm
 *
 * @param[in] gamma
 * gamma initial value
 *
 *******************************************************************************/

void init_quantile (quantile_t   *quantile,
                    const int     vect_size,
                    const double  alpha,
                    const double  gamma)
{
    quantile->quantile = melissa_calloc (vect_size, sizeof(double));
    quantile->increment = 0;
    quantile->alpha = alpha;
    quantile->gamma = gamma;
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
 * @param[in] in_vect[]
 * input vector of double values
 *
 * @param[in,out] *partial_mean
 * input: previously computed partial mean,
 * output: updated partial mean
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void increment_quantile (quantile_t *quantile,
                         double      in_vect[],
                         const int   vect_size)
{
    int     i;
    double j;

    quantile->increment += 1;

#ifdef BUILD_WITH_OPENMP
#pragma omp parallel for
#endif // BUILD_WITH_OPENMP
    for (i=0; i<vect_size; i++)
    {
        if (quantile->quantile[i] <= in_vect[i])
        {
            j = 1 - quantile->alpha;
        }
        else
        {
            j = 0 - quantile->alpha;
        }
        quantile->quantile[i] -= (1/(pow(quantile->increment, quantile->gamma))) * j;
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
 * @param[in] *quantile
 * quantile structures to save, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void save_quantile(quantile_t *quantiles,
                   int         vect_size,
                   int         nb_time_steps,
                   FILE*       f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fwrite(quantiles[i].quantile, sizeof(double), vect_size, f);
        fwrite(&quantiles[i].increment, sizeof(int), 1, f);
        fwrite(&quantiles[i].alpha, sizeof(double), 1, f);
        fwrite(&quantiles[i].gamma, sizeof(double), 1, f);
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
 * @param[in] *quantile
 * quantile structures to read, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void read_quantile(quantile_t *quantiles,
                   int         vect_size,
                   int         nb_time_steps,
                   FILE*       f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fread(quantiles[i].quantile, sizeof(double), vect_size, f);
        fread(&quantiles[i].increment, sizeof(int), 1, f);
        fread(&quantiles[i].alpha, sizeof(double), 1, f);
        fread(&quantiles[i].gamma, sizeof(double), 1, f);
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
