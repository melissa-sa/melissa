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
 * @file general_moments.c
 * @author Terraz Théophile
 * @date 2017-19-10
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
#include <math.h>
#include "general_moments.h"
#include "mean.h"
#include "variance.h"
#include "melissa_utils.h"

static inline void increment_moments_mean (double    *mean,
                                           double    *in_vect,
                                           const int  vect_size,
                                           const int  increment,
                                           const int  power)
{
    int     i;
    double temp;

#pragma omp parallel for schedule(static) private(temp)
    for (i=0; i<vect_size; i++)
    {
        temp = mean[i];
        mean[i] = temp + (pow(in_vect[i], power) - temp)/increment;
    }
}

static inline void update_moments_mean (double    *m1,
                                        double    *m2,
                                        double    *m3,
                                        const int  increment2,
                                        const int  increment3,
                                        const int  vect_size)
{
    int    i;
    double delta;

#pragma omp parallel for schedule(static) private(delta)
    for (i=0; i<vect_size; i++)
    {
        delta = (m2[i] - m1[i]);
        m3[i] = m1[i] + increment2 * delta / increment3;
    }}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function initializes a moments structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *moments
 * the moments_t structure to initialize
 *
 * @param[in] vect_size
 * size of the input vector
 *
 * @param[in] max_order
 * maximum moment order
 *
 *******************************************************************************/

void init_moments(moments_t *moments,
                  const int  vect_size,
                  const int  max_order)
{
    moments->m1 = melissa_calloc(vect_size, sizeof(double));
    moments->m2 = melissa_calloc(vect_size, sizeof(double));
    moments->theta2 = melissa_calloc(vect_size, sizeof(double));
    moments->m3 = melissa_calloc(vect_size, sizeof(double));
    moments->theta3 = melissa_calloc(vect_size, sizeof(double));
    moments->m4 = melissa_calloc(vect_size, sizeof(double));
    moments->theta4 = melissa_calloc(vect_size, sizeof(double));
    moments->increment = 0;
    moments->max_order = max_order;
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function incrementes a moment structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *moments
 * the moments_t structure to increment
 *
 * @param[in] in_vect[]
 * the input vector
 *
 * @param[in] vect_size
 * size of the input vector
 *
 *******************************************************************************/

void increment_moments (moments_t *moments,
                        double     in_vect[],
                        const int  vect_size)
{
    int i;
//    double m1, m2, m3, m4;
//    // gammas
//    if (moments->max_order > 0)
//    {
//#pragma omp parallel for schedule(static)
//        for (i=0; i<vect_size; i++)
//        {
//            moments->gamma1[i] += in_vect[i];
//        }
//    }
//    if (moments->max_order > 1)
//    {
//#pragma omp parallel for schedule(static)
//        for (i=0; i<vect_size; i++)
//        {
//            moments->gamma2[i] += pow(in_vect[i], 2);
//        }
//    }
//    if (moments->max_order > 2)
//    {
//#pragma omp parallel for schedule(static)
//        for (i=0; i<vect_size; i++)
//        {
//            moments->gamma3[i] += pow(in_vect[i], 3);
//        }
//    }
//    if (moments->max_order > 3)
//    {
//#pragma omp parallel for schedule(static)
//        for (i=0; i<vect_size; i++)
//        {
//            moments->gamma4[i] += pow(in_vect[i], 4);
//        }
//    }
    moments->increment += 1;

    //means
    increment_moments_mean(moments->m1, in_vect, vect_size, moments->increment, 1);
    increment_moments_mean(moments->m2, in_vect, vect_size, moments->increment, 2);
    increment_moments_mean(moments->m3, in_vect, vect_size, moments->increment, 3);
    increment_moments_mean(moments->m4, in_vect, vect_size, moments->increment, 4);

    // thetas
    if (moments->increment > 1)
    {
#pragma omp parallel for schedule(static) private(m1, m2, m3, m4)
        for (i=0; i<vect_size; i++)
        {
            if (moments->max_order > 1)
            {
//                m1 = moments->gamma1[i]/(moments->increment);
//                m2 = moments->gamma2[i]/(moments->increment);
//                moments->theta2[i] = m2 - pow(m1, 2);
                moments->theta2[i] = moments->m2[i] - pow(moments->m1[i], 2);
            }
            if (moments->max_order > 2)
            {
//                m3 = moments->gamma3[i]/(moments->increment);
//                moments->theta3[i] = m3 - 3*m1*m2 + 2*pow(m1, 3);
                moments->theta3[i] = moments->m3[i] - 3*moments->m1[i]*moments->m2[i] + 2*pow(moments->m1[i], 3);
            }
            if (moments->max_order > 3)
            {
//                m4 = moments->gamma4[i]/(moments->increment);
//                moments->theta4[i] = m4 - 4*m1*m3 + 6*pow(m1, 2)*m2 - 3*pow(m1, 4);
                moments->theta4[i] = moments->m4[i] - 4*moments->m1[i]*moments->m3[i] + 6*pow(moments->m1[i], 2)*moments->m2[i] - 3*pow(moments->m1[i], 4);
            }
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function agregates two moment structures.
 *
 *******************************************************************************
 *
 * @param[in] *moments1
 * first input moments_t structure
 *
 * @param[in] *moments2
 * second input moments_t structure
 *
 * @param[out] *updated_moments
 * updated moments_t structure
 *
 * @param[in] vect_size
 * size of the input vector
 *
 *******************************************************************************/

void update_moments (moments_t *moments1,
                     moments_t *moments2,
                     moments_t *updated_moments,
                     const int  vect_size)
{
    int i;
//    double m1, m2, m3, m4;
//     // gammas
//    if (updated_moments->max_order > 0)
//    {
//#pragma omp parallel for schedule(static)
//        for (i=0; i<vect_size; i++)
//        {
//            updated_moments->gamma1[i] = moments1->gamma1[i] + moments2->gamma1[i];
//        }
//    }
//    if (updated_moments->max_order > 1)
//    {
//#pragma omp parallel for schedule(static)
//        for (i=0; i<vect_size; i++)
//        {
//            updated_moments->gamma2[i] = moments1->gamma2[i] + moments2->gamma2[i];
//        }
//    }
//    if (updated_moments->max_order > 2)
//    {
//#pragma omp parallel for schedule(static)
//        for (i=0; i<vect_size; i++)
//        {
//            updated_moments->gamma3[i] = moments1->gamma3[i] + moments2->gamma3[i];
//        }
//    }
//    if (updated_moments->max_order > 3)
//    {
//#pragma omp parallel for schedule(static)
//        for (i=0; i<vect_size; i++)
//        {
//            updated_moments->gamma4[i] = moments1->gamma4[i] + moments2->gamma4[i];
//        }
//    }
    updated_moments->increment = moments1->increment + moments2->increment;

    //means
    update_moments_mean (moments1->m1,
                         moments2->m1,
                         updated_moments->m1,
                         moments2->increment,
                         updated_moments->increment,
                         vect_size);
    update_moments_mean (moments1->m2,
                         moments2->m2,
                         updated_moments->m2,
                         moments2->increment,
                         updated_moments->increment,
                         vect_size);
    update_moments_mean (moments1->m3,
                         moments2->m3,
                         updated_moments->m3,
                         moments2->increment,
                         updated_moments->increment,
                         vect_size);
    update_moments_mean (moments1->m4,
                         moments2->m4,
                         updated_moments->m4,
                         moments2->increment,
                         updated_moments->increment,
                         vect_size);

    // thetas
#pragma omp parallel for schedule(static) private(m1, m2, m3, m4)
    for (i=0; i<vect_size; i++)
    {
        if (updated_moments->max_order > 1)
        {
//            m1 = updated_moments->gamma1[i]/(updated_moments->increment);
//            m2 = updated_moments->gamma2[i]/(updated_moments->increment);
//            updated_moments->theta2[i] = m2 - pow(m1, 2);
            updated_moments->theta2[i] = updated_moments->m2[i] - pow(updated_moments->m1[i], 2);
        }
        if (updated_moments->max_order > 2)
        {
//            m3 = updated_moments->gamma3[i]/(updated_moments->increment);
//            updated_moments->theta3[i] = m3 - 3*m1*m2 + 2*pow(m1, 3);
            updated_moments->theta3[i] = updated_moments->m3[i] - 3*updated_moments->m1[i]*updated_moments->m2[i] + 2*pow(updated_moments->m1[i], 3);
        }
        if (updated_moments->max_order > 3)
        {
//            m4 = updated_moments->gamma4[i]/(updated_moments->increment);
//            updated_moments->theta4[i] = m4 - 4*m1*m3 + 6*pow(m1, 2)*m2 - 3*pow(m1, 4);
            updated_moments->theta4[i] = updated_moments->m4[i] - 4*updated_moments->m1[i]*updated_moments->m3[i] + 6*pow(updated_moments->m1[i], 2)*updated_moments->m2[i] - 3*pow(updated_moments->m1[i], 4);
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function writes an array of moment structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *moments
 * moment structures to save, size nb_time_steps
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

void save_moments(moments_t *moments,
                  int        vect_size,
                  int        nb_time_steps,
                  FILE*      f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fwrite(moments[i].m1, sizeof(double), vect_size, f);
        fwrite(moments[i].m2, sizeof(double), vect_size, f);
        fwrite(moments[i].m3, sizeof(double), vect_size, f);
        fwrite(moments[i].m4, sizeof(double), vect_size, f);
        fwrite(&moments[i].increment, sizeof(int), 1, f);
        fwrite(&moments[i].max_order, sizeof(int), 1, f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function reads an array of moment structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *moments
 * moment structures to save, size nb_time_steps
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

void read_moments(moments_t *moments,
                  int        vect_size,
                  int        nb_time_steps,
                  FILE*      f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fread(moments[i].m1, sizeof(double), vect_size, f);
        fread(moments[i].m2, sizeof(double), vect_size, f);
        fread(moments[i].m3, sizeof(double), vect_size, f);
        fread(moments[i].m4, sizeof(double), vect_size, f);
        fread(&moments[i].increment, sizeof(int), 1, f);
        fread(&moments[i].max_order, sizeof(int), 1, f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function computes the mean from general moments.
 *
 *******************************************************************************
 *
 * @param[in] *moments
 * the input moments_t structure
 *
 * @param[out] mean[]
 * the computed mean vector
 *
 * @param[in] vect_size
 * size of the input vector
 *
 *******************************************************************************/

void compute_mean (moments_t *moments,
                   double     mean[],
                   const int  vect_size)
{
    int i;
#pragma omp parallel for schedule(static)
    for (i=0; i<vect_size; i++)
    {
//        mean[i] = moments->gamma1[i] / moments->increment;
        mean[i] = moments->m1[i];
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function computes the variance from general moments.
 *
 *******************************************************************************
 *
 * @param[in] *moments
 * the input moments_t structure
 *
 * @param[out] variance[]
 * the computed variance vector
 *
 * @param[in] vect_size
 * size of the input vector
 *
 *******************************************************************************/

void compute_variance (moments_t *moments,
                       double     variance[],
                       const int  vect_size)
{
    int i;
#pragma omp parallel for schedule(static)
    for (i=0; i<vect_size; i++)
    {
        variance[i] = moments->theta2[i];
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function computes the skewness from general moments.
 *
 *******************************************************************************
 *
 * @param[in] *moments
 * the input moments_t structure
 *
 * @param[out] skewness[]
 * the computed skewness vector
 *
 * @param[in] vect_size
 * size of the input vector
 *
 *******************************************************************************/

void compute_skewness (moments_t *moments,
                       double     skewness[],
                       const int  vect_size)
{
    int i;
#pragma omp parallel for schedule(static)
    for (i=0; i<vect_size; i++)
    {
        skewness[i] = moments->theta3[i] / pow(moments->theta2[i], 1.5);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function computes the kurtosis from general moments.
 *
 *******************************************************************************
 *
 * @param[in] *moments
 * the input moments_t structure
 *
 * @param[out] kurtosis[]
 * the computed kurtosis vector
 *
 * @param[in] vect_size
 * size of the input vector
 *
 *******************************************************************************/

void compute_kurtosis (moments_t *moments,
                       double     kurtosis[],
                       const int  vect_size)
{
    int i;
#pragma omp parallel for schedule(static)
    for (i=0; i<vect_size; i++)
    {
        kurtosis[i] = moments->theta4[i] / pow(moments->theta3[i], 2);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function frees a moment_t structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *moments
 * the moments structure to free
 *
 *******************************************************************************/

void free_moments (moments_t *moments)
{
    melissa_free (moments->m1);
    melissa_free (moments->m2);
    melissa_free (moments->m3);
    melissa_free (moments->m4);
    melissa_free (moments->theta2);
    melissa_free (moments->theta3);
    melissa_free (moments->theta4);
}
