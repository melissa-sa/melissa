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
 * @file mean.c
 * @brief Mean related functions.
 * @author Terraz Théophile
 * @date 2016-15-02
 *
 **/

#if BUILD_WITH_MPI == 0
#undef BUILD_WITH_MPI
#endif // BUILD_WITH_MPI

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#ifdef BUILD_WITH_OPENMP
#include <omp.h>
#endif // BUILD_WITH_OPENMP
#include "mean.h"
#include "melissa_utils.h"

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function initializes a mean structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *mean
 * the mean structure to initialize
 *
 * @param[in] vect_size
 * size of the mean vector
 *
 *******************************************************************************/

void init_mean (mean_t    *mean,
                const int  vect_size)
{
    mean->mean = melissa_calloc (vect_size, sizeof(double));
    mean->increment = 0;
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function updates the incremental mean.
 *
 *******************************************************************************
 *
 * @param[in,out] *mean
 * input: previously computed iterative mean,
 * output: updated mean
 *
 * @param[in] in_vect[]
 * input vector of double values
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void increment_mean (mean_t    *mean,
                     double     in_vect[],
                     const int  vect_size)
{
    int     i;
    double temp;

    mean->increment += 1;
#pragma omp parallel for schedule(static) private(temp)
    for (i=0; i<vect_size; i++)
    {
        temp = mean->mean[i];
        // mean = temp + in_vect/increment
        mean->mean[i] = temp + (in_vect[i] - temp)/mean->increment;
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function agregates two partial means.
 *
 *******************************************************************************
 *
 * @param[in] *mean1
 * first input vector of partial means
 *
 * @param[in] *mean2
 * second input vector of partial means
 *
 * @param[out] *updated_mean
 * the updated mean
 *
 * @param[in] vect_size
 * size of the input and output vectors
 *
 *******************************************************************************/

void update_mean (mean_t    *mean1,
                  mean_t    *mean2,
                  mean_t    *updated_mean,
                  const int  vect_size)
{
    int     i;
    double delta;

    updated_mean->increment = mean2->increment + mean1->increment;

#pragma omp parallel for schedule(static) private(delta)
    for (i=0; i<vect_size; i++)
    {
        delta = (mean2->mean[i] - mean1->mean[i]);
        updated_mean->mean[i] = mean1->mean[i] + mean2->increment * delta / updated_mean->increment;
    }
}

#ifdef BUILD_WITH_MPI

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function agregates the partial means from all process on precess 0.
 *
 *******************************************************************************
 *
 * @param[in,out] *mean[]
 * input: partial mean,
 * output: global mean on process 0
 *
 * @param[in] vect_size
 * size of the input vector
 *
 * @param[in] rank
 * process rank in "comm"
 *
 * @param[in] comm_size
 * nomber of process in "comm"
 *
 * @param[in] comm
 * MPI communicator
 *
 *******************************************************************************/

void update_global_mean (mean_t    *mean,
                         const int  vect_size,
                         const int  rank,
                         const int  comm_size,
                         MPI_Comm   comm)
{
    double     *global_mean = NULL;
    double     *global_mean_ptr = NULL;
    double     *mean_ptr = NULL;
    double      delta;
    int         temp_inc;
    int         i, j;
    MPI_Status  status;

    if (rank == 0)
    {
        global_mean = melissa_malloc (vect_size * sizeof(double));
        memcpy (global_mean, mean->mean, vect_size * sizeof(double));

        for (i=1; i<comm_size; i++)
        {
            MPI_Recv (&temp_inc, 1, MPI_INT, i, i, comm, &status);
            MPI_Recv (mean->mean, vect_size, MPI_DOUBLE, i, comm_size+i, comm, &status);

            mean_ptr = mean->mean;
            global_mean_ptr = global_mean;

            for (j=0; j<vect_size; j++, mean_ptr++, global_mean_ptr++)
            {
                delta = (*global_mean_ptr - *mean_ptr);
                *global_mean_ptr = *mean_ptr + mean->increment * delta / (mean->increment + temp_inc);
            }
            mean->increment += temp_inc;
        }
        memcpy (mean->mean, global_mean, vect_size * sizeof(double));
        melissa_free (global_mean);
    }
    else // rank == 0
    {
        MPI_Send (&(mean->increment), 1, MPI_INT, 0, rank, comm);
        MPI_Send (mean->mean, vect_size, MPI_DOUBLE, 0, comm_size+rank, comm);
    }
}
#endif // BUILD_WITH_MPI

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function writes an array of mean structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *means
 * mean structures to save, size nb_time_steps
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

void save_mean(mean_t *means,
               int     vect_size,
               int     nb_time_steps,
               FILE*   f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fwrite(means[i].mean, sizeof(double), vect_size, f);
        fwrite(&means[i].increment, sizeof(int), 1, f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function reads an array of mean structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *means
 * mean structures to read, size nb_time_steps
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

void read_mean(mean_t *means,
               int     vect_size,
               int     nb_time_steps,
               FILE*   f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fread(means[i].mean, sizeof(double), vect_size, f);
        fread(&means[i].increment, sizeof(int), 1, f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function frees a mean structure.
 *
 *******************************************************************************
 *
 * @param[in] *mean
 * the mean structure to free
 *
 *******************************************************************************/

void free_mean (mean_t *mean)
{
    melissa_free (mean->mean);
}
