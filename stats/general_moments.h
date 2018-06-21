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
 * @file general_moments.h
 * @author Terraz Théophile
 * @date 2017-19-10
 *
 **/

#ifndef MOMENTS_H
#define MOMENTS_H

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * @struct moments_s
 *
 * Structure containing general moments intermediate values
 *
 *******************************************************************************/

struct moments_s
{
    double *m1;        /**< mean1[vect_size] */
    double *m2;        /**< mean1[vect_size] */
    double *m3;        /**< mean3[vect_size] */
    double *m4;        /**< mean4[vect_size] */
//    double *gamma1;    /**< gamma1[vect_size] */
//    double *gamma2;    /**< gamma2[vect_size] */
    double *theta2;    /**< theta2[vect_size] */
//    double *gamma3;    /**< gamma3[vect_size] */
    double *theta3;    /**< theta3[vect_size] */
//    double *gamma4;    /**< gamma4[vect_size] */
    double *theta4;    /**< theta4[vect_size] */
    int     increment; /**< increment         */
    int     max_order; /**< max moment order  */
};

typedef struct moments_s moments_t; /**< type corresponding to moments_s */

void init_moments(moments_t *moments,
                  const int  vect_size,
                  const int  max_order);

void increment_moments (moments_t *moments,
                        double     in_vect[],
                        const int  vect_size);

void save_moments(moments_t *moments,
                  int        vect_size,
                  int        nb_time_steps,
                  FILE*      f);

void read_moments(moments_t *moments,
                  int        vect_size,
                  int        nb_time_steps,
                  FILE*      f);

void compute_mean (moments_t *moments,
                   double     mean[],
                   const int  vect_size);

void compute_variance (moments_t *moments,
                       double     variance[],
                       const int  vect_size);

void compute_skewness (moments_t *moments,
                       double     skewness[],
                       const int  vect_size);

void compute_kurtosis (moments_t *moments,
                       double     kurtosis[],
                       const int  vect_size);

void free_moments (moments_t *moments);

#endif // MOMENTS_H
