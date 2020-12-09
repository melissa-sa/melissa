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
 * @file variance.c
 * @brief Variance related functions.
 * @author Terraz Théophile
 * @date 2016-15-02
 *
 **/

#include <melissa/stats/variance.h>
#include <melissa/utils.h>

#include <string.h>
#ifdef BUILD_WITH_OPENMP
#include <omp.h>
#endif // BUILD_WITH_OPENMP

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
    variance->variance = melissa_calloc (vect_size, sizeof(double));
    init_mean (&(variance->mean_structure),
               vect_size);
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
 * @param[in,out] *partial_variance
 * input: previously computed partial variance,
 * output: updated partial variance
 *
 * @param[in] in_vect[]
 * input vector of double values
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void increment_mean_and_variance (variance_t *partial_variance,
                                  double      in_vect[],
                                  const int   vect_size)
//{
//    int     i;

//#pragma omp parallel for schedule(static)
//    for (i=0; i<vect_size; i++)
//    {
//        double temp = partial_variance->mean_structure.mean[i];
//        partial_variance->mean_structure.mean[i] = temp + (in_vect[i] - temp)/(partial_variance->mean_structure.increment+1);
//        if (partial_variance->mean_structure.increment > 0)
//        {
//            partial_variance->variance[i] = (partial_variance->variance[i]*(partial_variance->mean_structure.increment-1)
//                                             + (in_vect[i] - temp) * (in_vect[i] - partial_variance->mean_structure.mean[i]))
//                                             / (partial_variance->mean_structure.increment);
//        }
//    }

//    partial_variance->mean_structure.increment += 1;
//}

{
    int i;
    double  incr = 0;

    increment_mean(&(partial_variance->mean_structure), in_vect, vect_size);
    incr = (double)partial_variance->mean_structure.increment;
    if (partial_variance->mean_structure.increment > 1)
    {
        for (i=0; i<vect_size; i++)
        {
            partial_variance->variance[i] *= (incr - 2);
            partial_variance->variance[i] += (in_vect[i] - partial_variance->mean_structure.mean[i]) * (in_vect[i] - partial_variance->mean_structure.mean[i]) * (incr/(incr-1));
            partial_variance->variance[i] /= (incr - 1);
        }
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
 * @param[in,out] *partial_variance
 * input: previously computed partial variance,
 * output: updated partial variance
 *
 * @param[in] in_vect[]
 * input vector of double values
 *
 * @param[in] vect_size
 * size of the input vectors
 *
 *******************************************************************************/

void increment_variance (variance_t *partial_variance,
                         double      in_vect[],
                         const int   vect_size)
{
    increment_mean_and_variance (partial_variance,
                                 in_vect,
                                 vect_size);
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
    int     i;

    update_mean(&variance1->mean_structure,
                &variance2->mean_structure,
                &updated_variance->mean_structure,
                vect_size);

#pragma omp parallel for schedule(static)
    for (i=0; i<vect_size; i++)
    {
        double delta = (variance1->mean_structure.mean[i] - variance2->mean_structure.mean[i]);
// Classic :
        updated_variance->variance[i] = variance1->variance[i] + variance2->variance[i]
                                        + variance1->mean_structure.increment * variance2->mean_structure.increment
                                        * delta * delta / updated_variance->mean_structure.increment;
// Unbiased :
//        updated_variance->variance[i] = ((variance1->mean_structure.increment - 1) * variance1->variance[i]
//                                        + (variance2->mean_structure.increment - 1) * variance2->variance[i]
//                                        + variance1->mean_structure.increment * variance2->mean_structure.increment
//                                        * delta * delta / updated_variance->mean_structure.increment)
//                                        / (updated_variance->mean_structure.increment - 1);
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
        global_mean = melissa_malloc (vect_size * sizeof(double));
        memcpy (global_mean, variance->mean_structure.mean, vect_size * sizeof(double));
        global_variance = melissa_malloc (vect_size * sizeof(double));
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
        melissa_free (global_mean);
        memcpy (variance->variance, global_variance, vect_size * sizeof(double));
        melissa_free (global_variance);
    }
    else // rank == 0
    {
        MPI_Send (&(variance->mean_structure.increment), 1, MPI_INT, 0, rank, comm);
        MPI_Send (variance->mean_structure.mean, vect_size, MPI_DOUBLE, 0, comm_size+rank, comm);
        MPI_Send (variance->variance, vect_size, MPI_DOUBLE, 0, 2*comm_size+rank, comm);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function writes an array of variances structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *vars
 * variance structures to save, size nb_time_steps
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

void save_variance(variance_t *vars,
                   int         vect_size,
                   int         nb_time_steps,
                   FILE*       f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fwrite(vars[i].variance, sizeof(double), vect_size, f);
        save_mean (&vars[i].mean_structure, vect_size, 1, f);
    }
}

/**
 *******************************************************************************
 *
 * @ingroup save_stats
 *
 * This function reads an array of variances structures on disc
 *
 *******************************************************************************
 *
 * @param[in] *vars
 * variance structures to read, size nb_time_steps
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

void read_variance(variance_t *vars,
                   int         vect_size,
                   int         nb_time_steps,
                   FILE*       f)
{
    int i;
    for (i=0; i<nb_time_steps; i++)
    {
        fread(vars[i].variance, sizeof(double), vect_size, f);
        read_mean (&vars[i].mean_structure, vect_size, 1, f);
    }
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
    melissa_free (variance->mean_structure.mean);
    melissa_free (variance->variance);
}
