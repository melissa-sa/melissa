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
 * @file melissa/server/data.h
 * @author Terraz Théophile
 * @date 2017-15-01
 *
 **/

#ifndef MELISSA_DATA_H
#define MELISSA_DATA_H

#include <melissa/server/options.h>
#include <melissa/stats/covariance.h>
#include <melissa/stats/general_moments.h>
#include <melissa/stats/mean.h>
#include <melissa/stats/min_max.h>
#include <melissa/stats/quantile.h>
#include <melissa/stats/sobol.h>
#include <melissa/stats/threshold.h>
#include <melissa/stats/variance.h>
#include <melissa/utils.h>
#include <melissa/vector.h>

#ifdef __cplusplus
extern "C" {
#endif

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
    MPI_Comm  comm;             /**< MPI communicator                               */
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
    int                  vect_size;                              /**< local size of input vectors                                     */
    melissa_options_t   *options;                                /**< pointer to an option structure                                  */
    int                  is_valid;                               /**< 1 if the structure has been checked                             */
    int                  stats_init;                             /**< 1 if the stats has been allocated                               */
    int                  steps_init;                             /**< 1 if the simu steps vector has been allocated                   */
//    mean_t              *means;                                  /**< array of mean structures, size nb_time_steps                    */
//    variance_t          *variances;                              /**< array of variance structures, size nb_time_steps                */
    min_max_t           *min_max;                                /**< array of min and max structures, size nb_time_steps             */
    threshold_t        **thresholds;                             /**< array of threshold exceedance structures                        */
    quantile_t         **quantiles;                              /**< array of quantile structures, size nb_time_steps * nb_quantiles */
    moments_t           *moments;                                /**< array of genera moment structures, size nb_time_steps           */
    sobol_array_t       *sobol_indices;                          /**< array of sobol array structures, size nb_time_steps             */
    void (*init_sobol)(sobol_array_t*, int, int);                /**< pointer to Sobol initialization function                        */
    void (*read_sobol)(sobol_array_t*, int, int, int, FILE*);    /**< pointer to Sobol read function                                  */
    void (*save_sobol)(sobol_array_t*, int, int, int, FILE*);    /**< pointer to Sobol save function                                  */
    void (*increment_sobol)(sobol_array_t*, int, double**, int); /**< pointer to Sobol increment function                             */
    void (*free_sobol)(sobol_array_t*, int);                     /**< pointer to Sobol free function                                  */
    int                  nb_simu;                                /**< number of simulation that have sent a message                   */
    vector_t             step_simu;                              /**< vector of arrays of bits, size nb_groups                        */
};

typedef struct melissa_data_s melissa_data_t; /**< type corresponding to melissa_data_s */

void melissa_init_data (melissa_data_t    *data,
                        melissa_options_t *options,
                        int                vect_size);

void melissa_check_data (melissa_data_t *data);

void melissa_free_data (melissa_data_t *data);

//long int mem_conso (melissa_options_t *options);

#ifdef __cplusplus
}
#endif

#endif // MELISSA_DATA_H
