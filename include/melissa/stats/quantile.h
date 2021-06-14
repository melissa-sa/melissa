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

#ifndef QUANTILE_H
#define QUANTILE_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

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
    double  alpha;     /**< alpha                      */
};

typedef struct quantile_s quantile_t; /**< type corresponding to quantile_s */

void init_quantile (quantile_t   *quantile,
                    const int     vect_size,
                    const double  alpha);

void increment_quantile (quantile_t *quantile,
                         const int   nmax,
                         double      in_vect[],
                         const int   vect_size);

void save_quantile(quantile_t **quantile,
                   int          vect_size,
                   int          nb_time_steps,
                   int          nb_quantiles,
                   FILE*        f);

void read_quantile(quantile_t **quantile,
                   int          vect_size,
                   int          nb_time_steps,
                   int          nb_quantiles,
                   FILE*        f);

void free_quantile(quantile_t *quantile);

#ifdef __cplusplus
}
#endif

#endif // QUANTILE_H
