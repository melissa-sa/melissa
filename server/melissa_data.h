/**
 *
 * @file melissa_data.h
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 **/

#ifndef MELISSA_DATA_H
#define MELISSA_DATA_H

#include "melissa_options.h"
#include "mean.h"
#include "variance.h"
#include "min_max.h"
#include "threshold.h"
#include "covariance.h"
#include "sobol.h"

/**
 *******************************************************************************
 *
 * @struct comm_data_s
 *
 * Structure to store communications parameters
 *
 *******************************************************************************/

struct comm_data_s
{
    int       rank;             /**< rank of the MPI process (0 if sequential)      */
    int       comm_size;        /**< size of the MPI communicator (1 if sequential) */
    int       client_comm_size; /**< size of the clients communicators              */
    int      *rcounts;          /**< counts for receiving datas                     */
    int      *rdispls;          /**< displacements for receiving datas              */
#ifdef BUILD_WITH_MPI
    MPI_Comm  comm;             /**< MPI communicator                               */
#endif // BUILD_WITH_MPI
};

typedef struct comm_data_s comm_data_t; /**< type corresponding to comm_data_s */

/**
 *******************************************************************************
 *
 * @struct melissa_data_s
 *
 * Structure to store global parameters
 *
 *******************************************************************************/

struct melissa_data_s
{
    int                  vect_size;     /**< local size of input vectors                               */
    melissa_options_t   *options;       /**< pointer to an option structure                            */
    int                  is_valid;      /**< 1 if the structure has been checked                       */
    mean_t              *means;         /**< array of mean structures, size nb_time_steps              */
    variance_t          *variances;     /**< array of variance structures, size nb_time_steps          */
    min_max_t           *min_max;       /**< array of min and max structures, size nb_time_steps       */
    int                **thresholds;    /**< array of threshold exceedance vectors, size nb_time_steps */
    sobol_array_t       *sobol_indices; /**< array of sobol array structures, size nb_time_steps       */
    void (*init_sobol)(sobol_array_t*, int, int);
    void (*read_sobol)(sobol_array_t*, int, int, int, FILE*);
    void (*save_sobol)(sobol_array_t*, int, int, int, FILE*);
    void (*increment_sobol)(sobol_array_t*, int, double**, int);
    void (*free_sobol)(sobol_array_t*, int);
    int                 *step_simu;     /**< iterations counter, size nb_groups                        */
};

typedef struct melissa_data_s melissa_data_t; /**< type corresponding to melissa_data_s */

void melissa_init_data (melissa_data_t    *data,
                        melissa_options_t *options,
                        int                vect_size);

void melissa_check_data (melissa_data_t *data);

void melissa_free_data (melissa_data_t *data);

long int mem_conso (melissa_options_t *options);

#endif // MELISSA_DATA_H
