/**
 *
 * @file stats_base.c
 * @brief Basic incremental vector statistics.
 * @author Terraz Théophile
 * @date 2016-15-02
 *
 **/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stats.h"

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
    mean->mean = calloc (vect_size, sizeof(double));
    mean->increment = 0;
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function initializes a variance structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *variance
 * the variance structure to initialize
 *
 * @param[in] vect_size
 * size of the variance vector
 *
 *******************************************************************************/

void init_variance (variance_t *variance,
                    const int   vect_size)
{
    variance->variance = calloc (vect_size, sizeof(double));
    init_mean (&(variance->mean_structure),
               vect_size);
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function initializes a min and max structure.
 *
 *******************************************************************************
 *
 * @param[in,out] *min_max
 * the min and max structure to initialize
 *
 * @param[in] vect_size
 * size of the vectors
 *
 *******************************************************************************/

void init_min_max (min_max_t *min_max,
                   const int  vect_size)
{
    min_max->min = malloc (vect_size * sizeof(double));
    min_max->max = malloc (vect_size * sizeof(double));
    min_max->is_init = 0;
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
 * @param[in] in_vect[]
 * input vector of double values
 *
 * @param[in,out] *partial_mean
 * input: previously computed partial mean,
 * output: updated partial mean
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void increment_mean (double     in_vect[],
                     mean_t    *partial_mean,
                     const int  vect_size)
{
    double  temp;
    int     i;

    partial_mean->increment += 1;

#pragma omp parallel for
    for (i=0; i<vect_size; i++)
    {
        temp = partial_mean->mean[i];
        partial_mean->mean[i] = temp + (in_vect[i] - temp)/partial_mean->increment;
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function updates the incremental mean and variance.
 *
 *******************************************************************************
 *
 * @param[in] in_vect[]
 * input vector of double values
 *
 * @param[in,out] *partial_variance
 * input: previously computed partial variance,
 * output: updated partial variance
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void increment_mean_and_variance (double      in_vect[],
                                  variance_t *partial_variance,
                                  const int   vect_size)
{
    double  temp;
    int     i;

    partial_variance->mean_structure.increment += 1;

#pragma omp parallel for
    for (i=0; i<vect_size; i++)
    {
        temp = partial_variance->mean_structure.mean[i];
        partial_variance->mean_structure.mean[i] = temp + (in_vect[i] - temp)/partial_variance->mean_structure.increment;
        partial_variance->variance[i] += (in_vect[i] - temp) * (in_vect[i] - partial_variance->mean_structure.mean[i]);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function updates the incremental variance.
 *
 *******************************************************************************
 *
 * @param[in] in_vect[]
 * input vector of double values
 *
 * @param[in,out] *partial_variance
 * input: previously computed partial variance,
 * output: updated partial variance
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void increment_variance (double      in_vect[],
                         variance_t *partial_variance,
                         const int   vect_size)
{
    increment_mean_and_variance(in_vect,
                                partial_variance,
                                vect_size);
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function updates the min and the max values of min and max vectors
 * using the input vector.
 *
 *******************************************************************************
 *
 * @param[in] in_vect[]
 * input vector of double values
 *
 * @param[in,out] *min_max
 * the min and max structure
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void min_and_max (double     in_vect[],
                  min_max_t *min_max,
                  const int  vect_size)
{
    if (min_max->is_init == 0)
    {
        memcpy (min_max->min, in_vect, vect_size * sizeof(double));
        memcpy (min_max->max, in_vect, vect_size * sizeof(double));
        min_max->is_init = 1;
    }
    else
    {
        int     i;
#pragma omp parallel for
        for (i=0; i<vect_size; i++)
        {
            if (min_max->min[i] > in_vect[i])
                min_max->min[i] = in_vect[i];
            if (min_max->max[i] < in_vect[i])
                min_max->max[i] = in_vect[i];
        }
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
    double  delta;
    int     i;

    updated_mean->increment = mean2->increment + mean1->increment;

#pragma omp parallel for
    for (i=0; i<vect_size; i++)
    {
        delta = (mean2->mean[i] - mean1->mean[i]);
        updated_mean->mean[i] = mean1->mean[i] + mean2->increment * delta / updated_mean->increment;
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function agregates two partial variances.
 *
 *******************************************************************************
 *
 * @param[in] *variance1
 * first input partial variances
 *
 * @param[in] *variance2
 * second input partial variances
 *
 * @param[out] *updated_variance
 * the updated variances
 *
 * @param[in] vect_size
 * size of the input and output vectors
 *
 *******************************************************************************/

void update_variance (variance_t *variance1,
                      variance_t *variance2,
                      variance_t *updated_variance,
                      const int   vect_size)
{
    double  delta;
    int     i;

    updated_variance->mean_structure.increment = variance2->mean_structure.increment + variance1->mean_structure.increment;

#pragma omp parallel for
    for (i=0; i<vect_size; i++)
    {
        delta = (variance1->mean_structure.mean[i] - variance2->mean_structure.mean[i]);
        updated_variance->variance[i] = variance1->variance[i] + variance2->variance[i]
                                        + variance1->mean_structure.increment * variance2->mean_structure.increment
                                        * delta * delta / updated_variance->mean_structure.increment;
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
        global_mean = malloc (vect_size * sizeof(double));
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
        free (global_mean);
    }
    else // rank == 0
    {
        MPI_Send (&(mean->increment), 1, MPI_INT, 0, rank, comm);
        MPI_Send (mean->mean, vect_size, MPI_DOUBLE, 0, comm_size+rank, comm);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function agregates the partial variances from all process on precess 0.
 *
 *******************************************************************************
 *
 * @param[in] mean[]
 * input: partial mean vector
 *
 * @param[in,out] *variance[]
 * input: partial variance vector,
 * output: global variance vector on process 0
 *
 * @param[in,out] increment
 * input: local increment,
 * output: global increment on process 0
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

void update_global_variance (variance_t *variance,
                             const int   vect_size,
                             const int   rank,
                             const int   comm_size,
                             MPI_Comm    comm)
{
    update_global_mean_and_variance(variance,
                                    vect_size,
                                    rank,
                                    comm_size,
                                    comm);
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function agregates the partial means and variances from all process on precess 0.
 *
 *******************************************************************************
 *
 * @param[in,out] *variance
 * input: partial variance,
 * output: global variance on process 0
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

void update_global_mean_and_variance (variance_t *variance,
                                      const int   vect_size,
                                      const int   rank,
                                      const int   comm_size,
                                      MPI_Comm    comm)
{
    double     *global_mean = NULL;
    double     *global_mean_ptr = NULL;
    double     *global_variance = NULL;
    double     *global_var_ptr = NULL;
    double     *mean_ptr = NULL;
    double     *var_ptr = NULL;
    double      delta;
    int         temp_inc;
    int         i, j;
    MPI_Status  status;

    if (rank == 0)
    {
        global_mean = malloc (vect_size * sizeof(double));
        memcpy (global_mean, variance->mean_structure.mean, vect_size * sizeof(double));
        global_variance = malloc (vect_size * sizeof(double));
        memcpy (global_variance, variance->variance, vect_size * sizeof(double));

        for (i=1; i<comm_size; i++)
        {
            MPI_Recv (&temp_inc, 1, MPI_INT, i, i, comm, &status);
            MPI_Recv (variance->mean_structure.mean, vect_size, MPI_DOUBLE, i, comm_size+i, comm, &status);
            MPI_Recv (variance->variance, vect_size, MPI_DOUBLE, i, 2*comm_size+i, comm, &status);

            mean_ptr = variance->mean_structure.mean;
            global_mean_ptr = global_mean;
            var_ptr = variance->variance;
            global_var_ptr = global_variance;

            for (j=0; j<vect_size; j++, mean_ptr++, global_mean_ptr++, var_ptr++, global_var_ptr++)
            {
                delta = (*global_mean_ptr - *mean_ptr);
                *global_mean_ptr = *mean_ptr + variance->mean_structure.increment * delta / (variance->mean_structure.increment + temp_inc);
                *global_var_ptr += *var_ptr + variance->mean_structure.increment * temp_inc * delta * delta / (variance->mean_structure.increment + temp_inc);
            }
            variance->mean_structure.increment += temp_inc;
        }
        memcpy (variance->mean_structure.mean, global_mean, vect_size * sizeof(double));
        free (global_mean);
        memcpy (variance->variance, global_variance, vect_size * sizeof(double));
        free (global_variance);
    }
    else // rank == 0
    {
        MPI_Send (&(variance->mean_structure.increment), 1, MPI_INT, 0, rank, comm);
        MPI_Send (variance->mean_structure.mean, vect_size, MPI_DOUBLE, 0, comm_size+rank, comm);
        MPI_Send (variance->variance, vect_size, MPI_DOUBLE, 0, 2*comm_size+rank, comm);
    }
}
#endif // BUILD_WITH_MPI

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function updates the number of values exceeding a given threshold
 *
 *******************************************************************************
 *
 * @param[in] in_vect[]
 * input vector of double values
 *
 * @param[in,out] threshold_exceedance[]
 * number of threshold exceedance occurences
 *
 * @param[in] threshold
 *
 * @param[in] vect_size
 * size of the input vector
 *
 *******************************************************************************/

void update_threshold_exceedance (double    in_vect[],
                                  int       threshold_exceedance[],
                                  int       threshold,
                                  const int vect_size)
{
    int     i;

#pragma omp parallel for
    for (i=0; i<vect_size; i++)
    {
        if (in_vect[i] > threshold)
            threshold_exceedance[i] += 1;
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
    free (mean->mean);
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function frees a variance structure.
 *
 *******************************************************************************
 *
 * @param[in] *variance
 * the variance structure to free
 *
 *******************************************************************************/

void free_variance (variance_t *variance)
{
    free (variance->mean_structure.mean);
    free (variance->variance);
}

/**
 *******************************************************************************
 *
 * @ingroup stats_base
 *
 * This function frees a min and max structure.
 *
 *******************************************************************************
 *
 * @param[in] *min_max
 * the min and max structure to free
 *
 *******************************************************************************/

void free_min_max (min_max_t *min_max)
{
    free (min_max->min);
    free (min_max->max);
}
