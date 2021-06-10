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

#include <melissa/stats/general_moments.h>
#include <melissa/stats/mean.h>
#include <melissa/stats/variance.h>
#include <melissa/utils.h>

#include <math.h>

static inline void increment_moments_mean (double    *mean,
                                           double    *in_vect,
                                           const int  vect_size,
                                           const int  increment,
                                           const int  power)
{
    int     i;
    double temp;

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

    for (i=0; i<vect_size; i++)
    {
        delta = (m2[i] - m1[i]);
        m3[i] = m1[i] + increment2 * delta / increment3;
    }}

/**
 * This function initializes a moments structure.
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
 */

void init_moments(moments_t *moments,
                  const int  vect_size,
                  const int  max_order)
{
    if (max_order > 0)
    {
        moments->m1 = melissa_calloc(vect_size, sizeof(double));
    }
    if (max_order > 1)
    {
        moments->m2 = melissa_calloc(vect_size, sizeof(double));
        moments->theta2 = melissa_calloc(vect_size, sizeof(double));
    }
    if (max_order > 2)
    {
        moments->m3 = melissa_calloc(vect_size, sizeof(double));
        moments->theta3 = melissa_calloc(vect_size, sizeof(double));
    }
    if (max_order > 3)
    {
        moments->m4 = melissa_calloc(vect_size, sizeof(double));
        moments->theta4 = melissa_calloc(vect_size, sizeof(double));
    }
    moments->increment = 0;
    moments->max_order = max_order;
}

/**
 * This function incrementes a moment structure.
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
 */

void increment_moments (moments_t *moments,
                        double     in_vect[],
                        const int  vect_size)
{
    int i;
    moments->increment += 1;
    if (moments->max_order < 1)
    {
        return;
    }

    //means
    increment_moments_mean(moments->m1, in_vect, vect_size, moments->increment, 1);
    if (moments->max_order > 1)
    {
        increment_moments_mean(moments->m2, in_vect, vect_size, moments->increment, 2);
    }
    if (moments->max_order > 2)
    {
        increment_moments_mean(moments->m3, in_vect, vect_size, moments->increment, 3);
    }
    if (moments->max_order > 3)
    {
        increment_moments_mean(moments->m4, in_vect, vect_size, moments->increment, 4);
    }

    // thetas
    if (moments->increment > 1)
    {
        for (i=0; i<vect_size; i++)
        {
            if (moments->max_order > 1)
            {
                moments->theta2[i] = moments->m2[i] - pow(moments->m1[i], 2);
            }
            if (moments->max_order > 2)
            {
                moments->theta3[i] = moments->m3[i] - 3*moments->m1[i]*moments->m2[i] + 2*pow(moments->m1[i], 3);
            }
            if (moments->max_order > 3)
            {
                moments->theta4[i] = moments->m4[i] - 4*moments->m1[i]*moments->m3[i] + 6*pow(moments->m1[i], 2)*moments->m2[i] - 3*pow(moments->m1[i], 4);
            }
        }
    }
}

/**
 * This function agregates two moment structures.
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
 */

void update_moments (moments_t *moments1,
                     moments_t *moments2,
                     moments_t *updated_moments,
                     const int  vect_size)
{
    int i;

    updated_moments->increment = moments1->increment + moments2->increment;
    if (updated_moments->max_order < 1)
    {
        return;
    }

    //means
    update_moments_mean (moments1->m1,
                         moments2->m1,
                         updated_moments->m1,
                         moments2->increment,
                         updated_moments->increment,
                         vect_size);
    if (updated_moments->max_order > 1)
    {
        update_moments_mean (moments1->m2,
                             moments2->m2,
                             updated_moments->m2,
                             moments2->increment,
                             updated_moments->increment,
                             vect_size);
    }
    if (updated_moments->max_order > 2)
    {
        update_moments_mean (moments1->m3,
                             moments2->m3,
                             updated_moments->m3,
                             moments2->increment,
                             updated_moments->increment,
                             vect_size);
    }
    if (updated_moments->max_order > 3)
    {
        update_moments_mean (moments1->m4,
                             moments2->m4,
                             updated_moments->m4,
                             moments2->increment,
                             updated_moments->increment,
                             vect_size);
    }

    // thetas
    for (i=0; i<vect_size; i++)
    {
        if (updated_moments->max_order > 1)
        {
            updated_moments->theta2[i] = updated_moments->m2[i] - pow(updated_moments->m1[i], 2);
        }
        if (updated_moments->max_order > 2)
        {
            updated_moments->theta3[i] = updated_moments->m3[i] - 3*updated_moments->m1[i]*updated_moments->m2[i] + 2*pow(updated_moments->m1[i], 3);
        }
        if (updated_moments->max_order > 3)
        {
            updated_moments->theta4[i] = updated_moments->m4[i] - 4*updated_moments->m1[i]*updated_moments->m3[i] + 6*pow(updated_moments->m1[i], 2)*updated_moments->m2[i] - 3*pow(updated_moments->m1[i], 4);
        }
    }
}

/**
 * This function writes an array of moment structures on disc
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
 */

void save_moments(moments_t *moments,
                  int        vect_size,
                  int        nb_time_steps,
                  FILE*      f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fwrite(&moments[i].max_order, sizeof(int), 1, f);
        if (moments[i].max_order > 0)
        {
            fwrite(moments[i].m1, sizeof(double), vect_size, f);
        }
        if (moments[i].max_order > 1)
        {
            fwrite(moments[i].m2, sizeof(double), vect_size, f);
        }
        if (moments[i].max_order > 2)
        {
            fwrite(moments[i].m3, sizeof(double), vect_size, f);
        }
        if (moments[i].max_order > 3)
        {
            fwrite(moments[i].m4, sizeof(double), vect_size, f);
        }
        fwrite(&moments[i].increment, sizeof(int), 1, f);
    }
}

/**
 * This function reads an array of moment structures on disc
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
 */

void read_moments(moments_t *moments,
                  int        vect_size,
                  int        nb_time_steps,
                  FILE*      f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fread(&moments[i].max_order, sizeof(int), 1, f);
        if (moments[i].max_order > 0)
        {
            fread(moments[i].m1, sizeof(double), vect_size, f);
        }
        if (moments[i].max_order > 1)
        {
            fread(moments[i].m2, sizeof(double), vect_size, f);
        }
        if (moments[i].max_order > 2)
        {
            fread(moments[i].m3, sizeof(double), vect_size, f);
        }
        if (moments[i].max_order > 3)
        {
            fread(moments[i].m4, sizeof(double), vect_size, f);
        }
        fread(&moments[i].increment, sizeof(int), 1, f);
    }
}

/**
 * This function computes the mean from general moments.
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
 */

void compute_mean (moments_t *moments,
                   double     mean[],
                   const int  vect_size)
{
    int i;
    for (i=0; i<vect_size; i++)
    {
        mean[i] = moments->m1[i];
    }
}

/**
 * This function computes the variance from general moments.
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
 */

void compute_variance (moments_t *moments,
                       double     variance[],
                       const int  vect_size)
{
    int i;
    for (i=0; i<vect_size; i++)
    {
        variance[i] = moments->theta2[i]*moments->increment/(moments->increment-1);
    }
}

/**
 * This function computes the skewness from general moments.
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
 */

void compute_skewness (moments_t *moments,
                       double     skewness[],
                       const int  vect_size)
{
    int i;
    for (i=0; i<vect_size; i++)
    {
        skewness[i] = moments->theta3[i] / pow(moments->theta2[i], 1.5);
    }
}

/**
 * This function computes the kurtosis from general moments.
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
 */

void compute_kurtosis (moments_t *moments,
                       double     kurtosis[],
                       const int  vect_size)
{
    int i;
    for (i=0; i<vect_size; i++)
    {
        kurtosis[i] = moments->theta4[i] / pow(moments->theta3[i], 2);
    }
}

/**
 * This function frees a moment_t structure.
 *
 * @param[in,out] *moments
 * the moments structure to free
 *
 */

void free_moments (moments_t *moments)
{
    if (moments->max_order > 0)
    {
        melissa_free (moments->m1);
    }
    if (moments->max_order > 1)
    {
        melissa_free (moments->m2);
        melissa_free (moments->theta2);
    }
    if (moments->max_order > 2)
    {
        melissa_free (moments->m3);
        melissa_free (moments->theta3);
    }
    if (moments->max_order > 3)
    {
        melissa_free (moments->m4);
        melissa_free (moments->theta4);
    }
}
