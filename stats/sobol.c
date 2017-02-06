/**
 *
 * @file sobol.c
 * @brief Functions needed to compute sobol indices.
 * @author Terraz Théophile
 * @date 2016-29-02
 *
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "mean.h"
#include "variance.h"
#include "covariance.h"
#include "sobol.h"

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function initialise a Martinez Sobol indices structure
 *
 *******************************************************************************
 *
 * @param[in,out] *sobol_indices
 * input: reference or pointer to an uninitialised sobol indices structure,
 * output: initialised structure, with values and variances set to 0
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void init_sobol_martinez (sobol_martinez_t *sobol_indices,
                          int               vect_size)
{
    init_covariance (&(sobol_indices->first_order_covariance), vect_size);
    init_covariance (&(sobol_indices->total_order_covariance), vect_size);
    init_variance (&(sobol_indices->variance_k), vect_size);

    sobol_indices->first_order_values = melissa_calloc (vect_size, sizeof(double));
    sobol_indices->total_order_values = melissa_calloc (vect_size, sizeof(double));
    sobol_indices->confidence_interval[0] = 1;
    sobol_indices->confidence_interval[1] = 1;
}

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function computes Sobol indices using Martinez formula
 *
 *******************************************************************************
 *
 * @param[out] *sobol_array
 * computed sobol indices, using Martinez formula
 *
 * @param[in] nb_parameters
 * size of sobol_array->sobol_martinez
 *
 * @param[in] **in_vect_tab
 * array of input vectors
 *
 * @param[in] vect_size
 * size of input vectors
 *
 *******************************************************************************/

void increment_sobol_martinez (sobol_array_t *sobol_array,
                               int            nb_parameters,
                               double       **in_vect_tab,
                               int            vect_size)
{
    int i, j;

    increment_variance (in_vect_tab[0], &(sobol_array->variance_a), vect_size);
    increment_variance (in_vect_tab[1], &(sobol_array->variance_b), vect_size);

    for (i=0; i< nb_parameters; i++)
    {
        increment_variance (in_vect_tab[i+2], &(sobol_array->sobol_martinez[i].variance_k), vect_size);
        increment_covariance (in_vect_tab[1], in_vect_tab[i+2], &(sobol_array->sobol_martinez[i].first_order_covariance), vect_size);
        increment_covariance (in_vect_tab[0], in_vect_tab[i+2], &(sobol_array->sobol_martinez[i].total_order_covariance), vect_size);

#pragma omp parallel for
        for (j=0; j<vect_size; j++)
            sobol_array->sobol_martinez[i].first_order_values[j] = sobol_array->sobol_martinez[i].first_order_covariance.covariance[j]
                    / ( sqrt(sobol_array->variance_b.variance[j])
                        * sqrt(sobol_array->sobol_martinez[i].variance_k.variance[j]) );

#pragma omp parallel for
        for (j=0; j<vect_size; j++)
            sobol_array->sobol_martinez[i].total_order_values[j] = 1.0 - sobol_array->sobol_martinez[i].total_order_covariance.covariance[j]
                    / ( sqrt(sobol_array->variance_a.variance[j])
                        * sqrt(sobol_array->sobol_martinez[i].variance_k.variance[j]) );
    }
    sobol_array->iteration += 1;
}

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function computes the confidence interval for Martinez Sobol indices
 *
 *******************************************************************************
 *
 * @param[out] *sobol_array
 * Sobol indices
 *
 * @param[in] nb_parameters
 * size of sobol_array->sobol_martinez
 *
 * @param[in] vect_size
 * size of input vectors
 *
 *******************************************************************************/

void confidence_sobol_martinez(sobol_array_t *sobol_array,
                               int            nb_parameters,
                               int            vect_size)
{
    int i, j;
    double temp1, temp2, interval;

    if (sobol_array->iteration < 4)
    {
        return;
    }

    temp2 = 1.96/(sqrt(sobol_array->iteration-3));
    for (j=0; j< nb_parameters; j++)
    {
        interval = 0;
        sobol_array->sobol_martinez[j].confidence_interval[0]=0;
        sobol_array->sobol_martinez[j].confidence_interval[1]=0;
        for (i=0; i<vect_size; i++)
        {
            temp1 = 0.5 * log((1.0+sobol_array->sobol_martinez[j].first_order_values[i])/(1.0-sobol_array->sobol_martinez[j].first_order_values[i]));
            interval = tanh(temp1 + temp2) - tanh(temp1 - temp2);
            if (sobol_array->sobol_martinez[j].confidence_interval[0] < interval)
            {
                sobol_array->sobol_martinez[j].confidence_interval[0] = interval;
            }
        }
        interval = 0;
        for (i=0; i<vect_size; i++)
        {
            temp1 = 0.5 * log((2.0-sobol_array->sobol_martinez[j].total_order_values[i])/sobol_array->sobol_martinez[j].total_order_values[i]);
            interval = (1-tanh(temp1 - temp2)) - (1-tanh(temp1 + temp2));
            if (sobol_array->sobol_martinez[j].confidence_interval[1] < interval)
            {
                sobol_array->sobol_martinez[j].confidence_interval[1] = interval;
            }
        }
    }
}

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function check if the Sobol indice convergence has been reached
 *
 *******************************************************************************
 *
 * @param[out] **sobol_array
 * Sobol indices
 *
 * @param[in] confidence_value
 * value to reach for the worst confidence interval
 *
 * @param[in] nb_time_steps
 * number of time steps of the study
 *
 * @param[in] nb_parameters
 * size of sobol_array->sobol_martinez
 *
 * @return[out] int
 * 0 if convergence is not reached
 * 1 if convergence is reached
 *
 *******************************************************************************/

int check_convergence_sobol_martinez(sobol_array_t **sobol_array,
                                     double          confidence_value,
                                     int             nb_time_steps,
                                     int             nb_parameters)
{
    int i, j;
    for (i=0; i<nb_time_steps; i++)
    {
        for (j=0; j<nb_parameters; j++)
        {
            if ((*sobol_array)[i].sobol_martinez[j].confidence_interval[0] > confidence_value)
            {
                return 0;
            }
            if ((*sobol_array)[i].sobol_martinez[j].confidence_interval[1] > confidence_value)
            {
                return 0;
            }
        }
    }
    return 1;
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function writes an array of sobol_martinez structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *sobol_array
 * sobol_array structures to save, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] nb_parameters
 * number of parameters of the study
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void write_sobol(sobol_array_t *sobol_array,
                 int            vect_size,
                 int            nb_time_steps,
                 int            nb_parameters,
                 FILE*          f)
{
    int i, j;
    for (i=0; i<nb_time_steps; i++)
    {
        for (j=0; j<nb_parameters; j++)
        {
            write_covariance (&sobol_array[i].sobol_martinez[j].first_order_covariance, vect_size, 1, f);
            write_covariance (&sobol_array[i].sobol_martinez[j].total_order_covariance, vect_size, 1, f);
            write_variance (&sobol_array[i].sobol_martinez[j].variance_k, vect_size, 1, f);
            fwrite(sobol_array[i].sobol_martinez[j].first_order_values, sizeof(double), vect_size,f);
            fwrite(sobol_array[i].sobol_martinez[j].total_order_values, sizeof(double), vect_size,f);
            fwrite(sobol_array[i].sobol_martinez[j].confidence_interval, sizeof(double), 2,f);
        }
        write_variance(&sobol_array[i].variance_a, vect_size, 1, f);
        write_variance(&sobol_array[i].variance_b, vect_size, 1, f);
        fwrite(&sobol_array[i].iteration, sizeof(int), 1,f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function reads an array of sobol_martinez structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *sobol_array
 * sobol_array structures to read, size nb_time_steps
 *
 * @param[in] vect_size
 * size of double vectors
 *
 * @param[in] nb_time_steps
 * number of time_steps of the study
 *
 * @param[in] nb_parameters
 * number of parameters of the study
 *
 * @param[in] f
 * file descriptor
 *
 *******************************************************************************/

void read_sobol(sobol_array_t *sobol_array,
                 int            vect_size,
                 int            nb_time_steps,
                 int            nb_parameters,
                 FILE*          f)
{
    int i, j;
    for (i=0; i<nb_time_steps; i++)
    {
        for (j=0; j<nb_parameters; j++)
        {
            read_covariance (&sobol_array[i].sobol_martinez[j].first_order_covariance, vect_size, 1, f);
            read_covariance (&sobol_array[i].sobol_martinez[j].total_order_covariance, vect_size, 1, f);
            read_variance (&sobol_array[i].sobol_martinez[j].variance_k, vect_size, 1, f);
            fread(sobol_array[i].sobol_martinez[j].first_order_values, sizeof(double), vect_size,f);
            fread(sobol_array[i].sobol_martinez[j].total_order_values, sizeof(double), vect_size,f);
            fread(sobol_array[i].sobol_martinez[j].confidence_interval, sizeof(double), 2,f);
        }
        read_variance(&sobol_array[i].variance_a, vect_size, 1, f);
        read_variance(&sobol_array[i].variance_b, vect_size, 1, f);
        fread(&sobol_array[i].iteration, sizeof(int), 1,f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup sobol
 *
 * This function frees a Martinez Sobol indices structure
 *
 *******************************************************************************
 *
 * @param[in] *sobol_indices
 * reference or pointer to a sobol index structure to free
 *
 *******************************************************************************/

void free_sobol_martinez (sobol_martinez_t *sobol_indices)
{
    free_covariance (&(sobol_indices->first_order_covariance));
    free_covariance (&(sobol_indices->total_order_covariance));
    free_variance (&(sobol_indices->variance_k));
    melissa_free (sobol_indices->first_order_values);
    melissa_free (sobol_indices->total_order_values);
}
