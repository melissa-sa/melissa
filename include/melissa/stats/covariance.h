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

#ifndef COVARIANCE_H
#define COVARIANCE_H

#include <melissa/stats/mean.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * @struct covariance_s
 *
 * Structure containing an array of covariances and the corresponding mean structures
 *
 *******************************************************************************/

struct covariance_s
{
    double *covariance; /**< covariance[vect_size] */
    mean_t  mean1;      /**< corresponding mean    */
    mean_t  mean2;      /**< corresponding mean    */
    int     increment;  /**< increment             */
};

typedef struct covariance_s covariance_t; /**< type corresponding to covariance_s */

void init_covariance(covariance_t *covariance,
                     const int     vect_size);

void increment_covariance (covariance_t *partial_covariance,
                           double        in_vect1[],
                           double        in_vect2[],
                           const int     vect_size);

void update_covariance (covariance_t *covariance1,
                        covariance_t *covariance2,
                        covariance_t *updated_covariance,
                        const int     vect_size);

void save_covariance(covariance_t *covars,
                     int           vect_size,
                     int           nb_time_steps,
                     FILE*         f);

void read_covariance(covariance_t *covars,
                     int           vect_size,
                     int           nb_time_steps,
                     FILE*         f);

void free_covariance (covariance_t *covariance);

#ifdef __cplusplus
}
#endif

#endif // COVARIANCE_H
