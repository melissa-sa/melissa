/**
 *
 * @file quantile.h
 * @author Terraz Théophile
 * @date 2017-18-05
 *
 **/

#ifndef QUANTILE_H
#define QUANTILE_H
#include <stdio.h>

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * @struct quantile_s
 *
 * Structure containing an array of quantiles,
 * the corresponding increment,
 * alpha (the quantile partition) and gamma parameters.
 *
 *******************************************************************************/

struct quantile_s
{
    double *quantile;  /**< quantile[vect_size]        */
    int     increment; /**< increment of this quantile */
    double  alpha;     /**< alpha */
    double  gamma;     /**< gamma */
};

typedef struct quantile_s quantile_t; /**< type corresponding to quantile_s */

void init_quantile (quantile_t   *quantile,
                    const int     vect_size,
                    const double  alpha,
                    const double  gamma);

void increment_quantile (quantile_t *quantile,
                         double      in_vect[],
                         const int   vect_size);

void save_quantile(quantile_t *quantile,
                   int         vect_size,
                   int         nb_time_steps,
                   FILE*       f);

void read_quantile(quantile_t *quantile,
                   int         vect_size,
                   int         nb_time_steps,
                   FILE*       f);

void free_quantile(quantile_t *quantile);

#endif // QUANTILE_H
