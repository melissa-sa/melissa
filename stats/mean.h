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
 * @file mean.h
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 **/

#ifndef MEAN_H
#define MEAN_H
#ifdef BUILD_WITH_MPI
#include <mpi.h>
#endif // BUILD_WITH_MPI
#include <stdio.h>

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * @struct mean_s
 *
 * Structure containing an array of means, and the corresponding increment
 *
 *******************************************************************************/

struct mean_s
{
    double *mean;      /**< mean[vect_size]        */
    int     increment; /**< increment of this mean */
};

typedef struct mean_s mean_t; /**< type corresponding to mean_s */

void init_mean(mean_t    *mean,
               const int  vect_size);

void increment_mean (mean_t    *mean,
                     double     in_vect[],
                     const int  vect_size);

void update_mean (mean_t    *mean1,
                  mean_t    *mean2,
                  mean_t    *updated_mean,
                  const int  vect_size);

#ifdef BUILD_WITH_MPI
void update_global_mean (mean_t    *mean,
                         const int  vect_size,
                         const int  rank,
                         const int  comm_size,
                         MPI_Comm   comm);
#endif // BUILD_WITH_MPI

void save_mean(mean_t *means,
               int     vect_size,
               int     nb_time_steps,
               FILE*   f);

void read_mean(mean_t *means,
               int     vect_size,
               int     nb_time_steps,
               FILE*   f);

void free_mean(mean_t *mean);

#endif // MEAN_H
