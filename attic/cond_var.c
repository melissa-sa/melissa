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
 * @file cond_var.c
 * @brief Functions needed to compute conditional variances.
 * @author Terraz Théophile
 * @date 2016-29-02
 *
 **/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stats.h"

///**
// *******************************************************************************
// *
// * @ingroup sobol
// *
// * This function initialise a conditional variance structure
// * @see conditional_variance_s
// *
// *******************************************************************************
// *
// * @param[in,out] *conditional_variance
// * input: reference or pointer to an uninitialised conditional_variance_t structure,
// * output: initialised structure, with mean and variance set to 0
// *
// * @param[in] indices_to_fix[]
// * array of indices to be copied in conditional_variance->fixed_indices
// *
// * @param[in] *data
// * pointer to the structure containing global parameters
// *
// *******************************************************************************/

//void init_conditional_variance (conditional_variance_t *conditional_variance,
//                                const int               indices_to_fix[],
//                                stats_data_t           *data)
//{
//    int i;

//    init_variance (&(conditional_variance->variance), data->vect_size);

//    conditional_variance->order = 0;

//    conditional_variance->fixed_indices = melissa_malloc (data->options->nb_parameters * sizeof(int));
//    memcpy (conditional_variance->fixed_indices, indices_to_fix, data->options->nb_parameters * sizeof(int));

//    for (i=0; i<data->options->nb_parameters; i++)
//        if (conditional_variance->fixed_indices[i] == 1)
//            conditional_variance->order += 1;

//}

///**
// *******************************************************************************
// *
// * @ingroup sobol
// *
// * This function compute the conditional variance of the given conditional means
// *
// *******************************************************************************
// *
// * @param[in,out] *conditional_variance
// * reference or pointer to a conditional_variance_t structure
// *
// * @param[in] *conditional_means
// * reference or pointer to a conditional_mean_t structure
// *
// * @param[in] *data
// * pointer to the structure containing global parameters
// *
// *******************************************************************************/

//int compute_conditional_variance (conditional_variance_t *conditional_variance,
//                                  conditional_mean_t     *conditional_means,
//                                  stats_data_t           *data)
//{
//    int    *param;
//    double *mean;
//    int     increment;
//    int     i, j;
//    int     ret;

//    param = melissa_malloc(data->options->nb_parameters * sizeof(int));
//#pragma omp parallel for
//    for (i=0; i<data->options->nb_parameters; i++)
//    {
//        if (conditional_variance->fixed_indices[i] != 1)
//        {
//            param[i] = -1;
//        }
//        else
//        {
//            param[i] = 0;
//        }
//    }

//    mean = melissa_calloc(data->vect_size, sizeof(double));

//    i = 0;
//    while (i<data->options->nb_parameters)
//    {
//        i = 0;
//        while (param[i] >= data->options->size_parameters[i] || param[i] < 0)
//        {
//            if (i<data->options->nb_parameters)
//            {
//                if (param[i] >= data->options->size_parameters[i])
//                {
//                    param[i] = 0;
//                }
//            }
//            else
//            {
//                break;
//            }
//            i += 1;
//        }

//        if (i<data->options->nb_parameters)
//        {
//            ret = get_conditional_mean(conditional_means, mean, &increment, param, data);
//            if (ret != SUCCESS)
//                exit (1);

//            increment_mean_and_variance(mean,
//                                        &(conditional_variance->variance),
//                                        data->vect_size);

//            param[i] += 1;
//        }
//    }

//#pragma omp parallel for
//    for (i=0; i<data->vect_size; i++)
//    {
//        conditional_variance->variance.variance[i] = conditional_variance->variance.variance[i]
//                                                     / conditional_variance->variance.mean_structure.increment;
//    }

//    free (param);
//    free (mean);

//    return SUCCESS;
//}

///**
// *******************************************************************************
// *
// * @ingroup sobol
// *
// * This function frees a conditional variance structure
// *
// *******************************************************************************
// *
// * @param[in] *conditional_variance
// * reference or pointer to a conditional_variance_t structure to free
// *
// *******************************************************************************/

//void free_conditional_variance (conditional_variance_t *conditional_variance)
//{
//    free_variance(&(conditional_variance->variance));
//    free(conditional_variance->fixed_indices);

//}
