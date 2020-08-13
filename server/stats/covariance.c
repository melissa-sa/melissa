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
 * @file covariance.c
 * @brief Functions needed to compute covariances.
 * @author Terraz Théophile
 * @date 2016-01-07
 *
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef BUILD_WITH_MPI
//#include <mpi.h>
#endif // BUILD_WITH_MPI
#ifdef BUILD_WITH_OPENMP
#include <omp.h>
#endif // BUILD_WITH_OPENMP
#include "mean.h"
#include "variance.h"
#include "covariance.h"
#include "melissa_utils.h"

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function initializes a covariance structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *covariance
 * the covariance structure to initialize
 *
 * @param[in] vect_size
 * size of the covariance vector
 *
 *******************************************************************************/

void init_covariance (covariance_t *covariance,
                      const int     vect_size)
{
    covariance->covariance = melissa_calloc (vect_size, sizeof(double));
    init_mean (&covariance->mean1, vect_size);
    init_mean (&covariance->mean2, vect_size);
    covariance->increment = 0;
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function updates the incremental covariance.
 *
 *******************************************************************************
 *
 * @param[in,out] *covariance
 * input: previously computed covariance,
 * output: incremented covariance
 *
 * @param[in] in_vect1[]
 * first input vector of double values
 *
 * @param[in] in_vect2[]
 * second input vector of double values
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

//void increment_covariance (covariance_t *covariance,
//                           double        in_vect1[],
//                           double        in_vect2[],
//                           const int     vect_size)
//{
//    int     i;
//    int     incr = 0;

//    increment_mean(&(covariance->mean1), in_vect1, vect_size);
//    incr = covariance->increment;
//    if (covariance->increment > 0)
//    {
//#pragma omp parallel for schedule(static) firstprivate(incr)
//        for (i=0; i<vect_size; i++)
//        {
//            covariance->covariance[i] *= (incr - 1);
//            covariance->covariance[i] +=  (in_vect1[i] - covariance->mean1.mean[i]) * (in_vect2[i] - covariance->mean2.mean[i]);
//            covariance->covariance[i] /= (double)(incr);
//        }
//    }
//    increment_mean(&(covariance->mean2), in_vect2, vect_size);
//    covariance->increment += 1;
//}

void increment_covariance (covariance_t *covariance,
                           double        in_vect1[],
                           double        in_vect2[],
                           const int     vect_size)
{
    int     i;
    double  incr = 0;

    covariance->increment += 1;
    incr = (double)covariance->increment;
    increment_mean(&(covariance->mean1), in_vect1, vect_size);
    if (covariance->increment > 1)
    {
#pragma omp parallel for schedule(static) firstprivate(incr)
        for (i=0; i<vect_size; i++)
        {
            covariance->covariance[i] *= (incr - 2);
            covariance->covariance[i] += (in_vect1[i] - covariance->mean1.mean[i]) * (in_vect2[i] - covariance->mean2.mean[i]);
            covariance->covariance[i] /= (incr - 1);
        }
    }
    increment_mean(&(covariance->mean2), in_vect2, vect_size);
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function updates the incremental covariance.
 *
 *******************************************************************************
 *
 * @param[in] *covariance1
 * first input partial covariance
 *
 * @param[in] *covariance2
 * second input partial covariance
 *
 * @param[out] *updated_covariance
 * updated covariance
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void update_covariance (covariance_t *covariance1,
                        covariance_t *covariance2,
                        covariance_t *updated_covariance,
                        const int     vect_size)
{
    int     i;

    updated_covariance->increment = covariance1->increment + covariance2->increment;

#pragma omp parallel for schedule(static)
    for (i=0; i<vect_size; i++)
    {
        updated_covariance->covariance[i] = ((covariance1->increment - 1) * covariance1->covariance[i]
                + (covariance2->increment - 1) * covariance2->covariance[i]
                + ((covariance1->increment * covariance2->increment)/updated_covariance->increment)
                * (covariance2->mean1.mean[i] - covariance1->mean1.mean[i])
                * (covariance2->mean2.mean[i] - covariance1->mean2.mean[i]))
                / (updated_covariance->increment - 1);
    }
    update_mean (&covariance1->mean1, &covariance2->mean1, &updated_covariance->mean1, vect_size);
    update_mean (&covariance1->mean2, &covariance2->mean2, &updated_covariance->mean2, vect_size);
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function writes an array of covariances structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *covars
 * covariance structures to save, size nb_time_steps
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

void save_covariance(covariance_t *covars,
                     int           vect_size,
                     int           nb_time_steps,
                     FILE*         f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fwrite(covars[i].covariance, sizeof(double), vect_size, f);
        save_mean (&covars[i].mean1, vect_size, 1, f);
        save_mean (&covars[i].mean2, vect_size, 1, f);
        fwrite(&covars[i].increment, sizeof(int), 1, f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function reads an array of covariances structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *covars
 * covariance structures to read, size nb_time_steps
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

void read_covariance(covariance_t *covars,
                     int           vect_size,
                     int           nb_time_steps,
                     FILE*         f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fread(covars[i].covariance, sizeof(double), vect_size, f);
        read_mean (&covars[i].mean1, vect_size, 1, f);
        read_mean (&covars[i].mean2, vect_size, 1, f);
        fread(&covars[i].increment, sizeof(int), 1, f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function frees a covariance structure.
 *
 *******************************************************************************
 *
 * @param[in] *covariance
 * the covariance structure to free
 *
 *******************************************************************************/

void free_covariance (covariance_t *covariance)
{
    melissa_free (covariance->covariance);
    free_mean (&covariance->mean1);
    free_mean (&covariance->mean2);
}
