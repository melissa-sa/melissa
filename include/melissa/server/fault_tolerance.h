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
 * @file melissa/server/fault_tolerance.h
 * @author Terraz Théophile
 * @date 2017-30-06
 *
 **/

#ifndef FAULT_TOLERANCE_H
#define FAULT_TOLERANCE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <melissa/utils.h>
#include <melissa/vector.h>

/**
 *******************************************************************************
 *
 * @struct melissa_simulation_s
 *
 * Structure to store simulation informations for fault tolerance
 *
 *******************************************************************************/

struct melissa_simulation_s
{
    int    status;         /**< simulation status (0: no messages recieved, 1: at least one message recieved, 2: finished */
    int    last_time_step; /**< simulation status (1: not all messages recieved, 2: all messages recieved */
    int    timeout;        /**< 1 if timeout detected on this simulation */
    double last_message;   /**< time of the last recieved message from this simulation */
    char   job_id[255];    /**< simulation job ID */
    int    job_status;     /**< simulation job status */
    double *parameters;    /**< simulation parameter set */
};

typedef struct melissa_simulation_s melissa_simulation_t; /**< type corresponding to melissa_simulation_s */

melissa_simulation_t* add_simulation();

void free_simu_vector(vector_t v);

int check_timeouts (vector_t *simulations,
                    int       timeout_simu);

void send_timeouts (int       detected_timeouts,
                    vector_t *simulations,
                    void     *python_pusher);


int count_job_status(vector_t *simulations,
                      int       job_status);

#ifdef __cplusplus
}
#endif

#endif // FAULT_TOLERANCE_H
