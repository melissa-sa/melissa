/******************************************************************
*                            Melissa                              *
*-----------------------------------------------------------------*
*   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    *
*                                                                 *
* This source is covered by the BSD 3-Clause License.             *
* Refer to the  LICENCE file for further information.             *
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
 * @file min_max.h
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 **/

#ifndef MIN_MAX_H
#define MIN_MAX_H

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * @struct min_max_s
 *
 * Structure containing two arrays of min and max values
 *
 *******************************************************************************/

struct min_max_s
{
    double *min;     /**< min[vect_size]                          */
    double *max;     /**< max[vect_size]                          */
    int     is_init; /**< 0 before the first update, 1 otherwhise */
};

typedef struct min_max_s min_max_t; /**< type corresponding to min_max_s */

void init_min_max(min_max_t *min_max,
                  const int  vect_size);

void min_and_max (min_max_t *min_max,
                  double     in_vect[],
                  const int  vect_size);

void save_min_max(min_max_t *minmax,
                  int        vect_size,
                  int        nb_time_steps,
                  FILE*      f);

void read_min_max(min_max_t *minmax,
                  int        vect_size,
                  int        nb_time_steps,
                  FILE*      f);

void free_min_max (min_max_t *min_max);

#endif // MIN_MAX_H
