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

#ifndef THRESHOLD_H
#define THRESHOLD_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * @struct threshold_s
 *
 * Structure containing threshold exceedance arrays
 *
 *******************************************************************************/

struct threshold_s
{
    int    *threshold_exceedance; /**< threshold arrays           */
    double  value;                /**< threshold values           */
};

typedef struct threshold_s threshold_t; /**< type corresponding to variance_s */

void init_threshold (threshold_t  *threshold,
                     const int     vect_size,
                     const double  value);

void update_threshold_exceedance (threshold_t *threshold,
                                  double       in_vect[],
                                  const int    vect_size);

void save_threshold(threshold_t **threshold,
                    int           vect_size,
                    int           nb_time_steps,
                    int           nb_values,
                    FILE*         f);

void read_threshold(threshold_t **threshold,
                    int           vect_size,
                    int           nb_time_steps,
                    int           nb_values,
                    FILE*         f);

void free_threshold(threshold_t *threshold);

#ifdef __cplusplus
}
#endif

#endif // THRESHOLD_H
