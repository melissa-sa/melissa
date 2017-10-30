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
 * @file cond_mean.c
 * @brief Functions needed to compute conditional means.
 * @author Terraz Théophile
 * @date 2016-22-02
 *
 **/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stats.h"

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function initialize the recursive conditional means structure
 * @see conditional_mean_s
 *
 *******************************************************************************
 *
 * @param[in,out] *conditional_means
 * input: reference or pointer to an uninitialised conditional_mean_t structure
 * output: initialised structure, with all means set to 0
 *
 * @param[in] *data
 * structure containing global parameters
 *
 *******************************************************************************/


void init_conditional_means (conditional_mean_t *conditional_means,
                             stats_data_t       *data)
{
    conditional_mean_t *next_cond_mean = NULL;
    int i, j;
    int depth = 0;
    int next_fixed_parameter[2]; // first value : place in next_cond_mean->indices
                                 // second value : value in next_cond_mean->indices[first value]

    init_mean (&(conditional_means->mean), data->vect_size);

    conditional_means->order = 0;

    conditional_means->is_leaf = 0;

    conditional_means->indices = melissa_malloc (data->options->nb_parameters * sizeof(int));
    for (i=0; i<data->options->nb_parameters; i++)
        conditional_means->indices[i] = -1;

    conditional_means->indice_ptr = melissa_malloc ((data->options->nb_parameters +1) * sizeof(int));

    conditional_means->indice_ptr_size = data->options->nb_parameters+1;

    conditional_means->indice_ptr[0] = 0;
    for (i=0; i<data->options->nb_parameters; i++)
    {
//        conditional_means->indice_ptr[i+1] = conditional_means->indice_ptr[i] + data->options->size_parameters[i];
    }

    conditional_means->next_conditional_means = melissa_malloc ((conditional_means->indice_ptr[data->options->nb_parameters]) * sizeof(conditional_mean_t));

    next_cond_mean = conditional_means->next_conditional_means;
    for (i=0; i<data->options->nb_parameters; i++)
    {
        next_fixed_parameter[0] = i;
        for (j=conditional_means->indice_ptr[i]; j<conditional_means->indice_ptr[i+1]; j++, next_cond_mean++)
        {
            next_fixed_parameter[1] = j - conditional_means->indice_ptr[i];
            init_next_conditional_mean (next_cond_mean,
                                        depth + 1,
                                        next_fixed_parameter,
                                        conditional_means->indices,
                                        data);
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function initialize the recursive conditional means structure (order > 0)
 *
 *******************************************************************************
 *
 * @param[in,out] *conditional_means
 * input: reference or pointer to an uninitialised conditional_mean_t structure,
 * output: initialised structure, with all means set to 0
 *
 * @param[in] depth
 * the depth in the tree
 *
 * @param[in] new_fixed_parameter[]
 * the parameter that will be fixed in this mean, and wasn't in his father
 *
 * @param[in] previous_indices[]
 * indices values of the father (value >= 0 if fixed, -1 otherwise)
 *
 * @param[in] *data
 * pointer to the structure containing global parameters
 *
 *******************************************************************************/

void init_next_conditional_mean (conditional_mean_t *conditional_means,
                                 const int           depth,
                                 const int           new_fixed_parameter[],
                                 const int           previous_indices[],
                                 stats_data_t       *data)
{
    conditional_mean_t *next_cond_mean = NULL;
    int i, j, k;
    int next_fixed_parameter[2]; // first value : place in next_cond_mean->indices
                                 // second value : value in next_cond_mean->indices[first value]
    int nb_parameters_to_fix; // number of variable parameters after the new fix one

    init_mean (&(conditional_means->mean), data->vect_size);

    nb_parameters_to_fix = data->options->nb_parameters - new_fixed_parameter[0] - 1;

    conditional_means->order = depth;

    conditional_means->is_leaf = 0;

    conditional_means->indices = melissa_malloc (data->options->nb_parameters * sizeof(int));
    memcpy (conditional_means->indices , previous_indices, data->options->nb_parameters * sizeof(int));

    conditional_means->indices[new_fixed_parameter[0]] = new_fixed_parameter[1];

    // if we don't need to go deeper, we stop here
    if (data->options->sobol_order == depth || nb_parameters_to_fix < 1)
    {
        conditional_means->is_leaf = 1;
        return;
    }

    conditional_means->indice_ptr = melissa_malloc ((nb_parameters_to_fix +1) * sizeof(int));

    conditional_means->indice_ptr_size = nb_parameters_to_fix +1;

    conditional_means->indice_ptr[0] = 0;
    j = 0;
    for (i=new_fixed_parameter[0] + 1; i<data->options->nb_parameters; i++)
    {
        if (conditional_means->indices[i] == -1)
        {
//            conditional_means->indice_ptr[j+1] = conditional_means->indice_ptr[j] + data->options->size_parameters[i];
            j += 1;
        }
    }

    conditional_means->next_conditional_means = melissa_malloc ( (conditional_means->indice_ptr[nb_parameters_to_fix]) * sizeof(conditional_mean_t));

    next_cond_mean = conditional_means->next_conditional_means;

    i = new_fixed_parameter[0]+1;
    for (k=0; k<nb_parameters_to_fix; k++) // loop over parameters
    {
        next_fixed_parameter[0] = i;
        for (j=conditional_means->indice_ptr[k]; j<conditional_means->indice_ptr[k+1]; j++, next_cond_mean++) // boucle sur les valeurs des paramètres
        {
            next_fixed_parameter[1] = j - conditional_means->indice_ptr[k];
            init_next_conditional_mean (next_cond_mean,
                                        depth + 1,
                                        next_fixed_parameter,
                                        conditional_means->indices,
                                        data);
        }
        i += 1;
    }
}

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function increment the conditional means corresponding to the given parameters
 * with the input vector
 *
 *******************************************************************************
 *
 * @param[in,out] *conditional_means
 * input: reference or pointer to a conditional_mean_t structure,
 * output: the structure, with all means updated
 *
 * @param[in] in_vect
 * input vector of values used to update the conditional means
 *
 * @param[in] parameters[]
 * parameters used in the study that produced in_vect
 *
 * @param[in] *data
 * pointer to the structure containing global parameters
 *
 *******************************************************************************/

void increment_conditional_mean (conditional_mean_t *conditional_means,
                                 double              in_vect[],
                                 const int           parameters[],
                                 stats_data_t       *data)
{
    int i, j;
    for (i=0; i<data->options->nb_parameters; i++)
        if (conditional_means->indices[i] != -1 && conditional_means->indices[i] != parameters[i])
            return;

    increment_mean (in_vect, &(conditional_means->mean), data->vect_size);

    if (conditional_means->is_leaf == 1)
        return;

    for (i=0; i<conditional_means->indice_ptr[conditional_means->indice_ptr_size-1]; i++)
        increment_conditional_mean (&conditional_means->next_conditional_means[i],
                                    in_vect,
                                    parameters,
                                    data);

}

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function copy the mean needed by the user from a conditional_mean_t structure to a vector
 *
 *******************************************************************************
 *
 * @param[in] *conditional_means
 * reference or pointer to a conditional_mean_t structure
 *
 * @param[out] *out_mean
 * vector of values of the required conditional mean
 * if the required mean does not exist, *out_mean is not modified by the function
 * the function then returns WARNING_NOTING_RETURNED
 *
 * @param[out] *out_increment
 * increment of the returned mean
 *
 * @param[in] parameters[]
 * list of the parameters defining the required mean
 *
 * @param[in] *data
 * pointer to the structure containing global parameters
 *
 *******************************************************************************
 *
 * \return SUCCESS if the vector has been copied succesfully,
 * \return WARNING_NOTING_RETURNED if no vector has been copied
 * \return ERROR_BAD_PARAMETER if an error occured
 *
 *******************************************************************************/

int get_conditional_mean (conditional_mean_t *conditional_means,
                          double             *out_mean,
                          int                *out_increment,
                          const int           parameters[],
                          stats_data_t       *data)
{
    conditional_mean_t *next_cond_mean = NULL;
    int i, j;
    int curent_fixed_parameter = -1;
    int first_input_fixed_parameter = -1;

    if (conditional_means->order == 0)
    {
        for (i=0; i<data->options->nb_parameters; i++)
        {
            if (parameters[i] > -1)
            {
                first_input_fixed_parameter = i;
                break;
            }
        }

        if (first_input_fixed_parameter != -1)
        {
            next_cond_mean = &(conditional_means->next_conditional_means[conditional_means->indice_ptr[first_input_fixed_parameter] + parameters[first_input_fixed_parameter]]);
            return get_conditional_mean (next_cond_mean,
                                         out_mean,
                                         out_increment,
                                         parameters,
                                         data);
        }
    }

    j = 0;
    for (i=0; i<data->options->nb_parameters; i++)
    {
        if (conditional_means->indices[i] == parameters[i])
        {
            j += 1;
            if (conditional_means->indices[i] != -1)
            {
                curent_fixed_parameter = i; // we get the last fixed parameter
            }
        }
        else if (conditional_means->indices[i] != -1)
        {
            return ERROR_BAD_PARAMETER;
        }
    }

    if (j == data->options->nb_parameters)
    {
        memcpy (out_mean, conditional_means->mean.mean, data->vect_size * sizeof(double));
        *out_increment = conditional_means->mean.increment;
        return SUCCESS;
    }
    if (conditional_means->is_leaf == 1) return WARNING_NOTING_RETURNED;

    for (i=curent_fixed_parameter+1; i<data->options->nb_parameters; i++)
    {
        if (parameters[i] != -1)
        {
            next_cond_mean = &(conditional_means->next_conditional_means[conditional_means->indice_ptr[i-curent_fixed_parameter-1] + parameters[i]]);
            return get_conditional_mean (next_cond_mean,
                                         out_mean,
                                         out_increment,
                                         parameters,
                                         data);
        }

    }
}

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function frees a conditional_mean_t structure
 *
 *******************************************************************************
 *
 * @param[in] *conditional_means
 * reference or pointer to a conditional_mean_t structure to free
 *
 *******************************************************************************/

void free_conditional_mean (conditional_mean_t *conditional_means)
{
    int i;

    if (conditional_means->is_leaf == 0)
    {
        for (i=0; i<conditional_means->indice_ptr[conditional_means->indice_ptr_size-1]; i++)
            free_conditional_mean (&conditional_means->next_conditional_means[i]);

        free (conditional_means->indice_ptr);
        free (conditional_means->next_conditional_means);
    }
    free_mean (&(conditional_means->mean));
    free (conditional_means->indices);
}
