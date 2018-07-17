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
 * @file fault_tolerance.h
 * @author Terraz Théophile
 * @date 2017-30-06
 *
 **/

#ifndef FAULT_TOLERANCE_H
#define FAULT_TOLERANCE_H

#include <stdio.h>
#include <stdlib.h>
#include <melissa_utils.h>
#include <vector.h>

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
    int    status;       /**< simulation status (0: no messages recieved, 1: at least one message recieved, 2: finished */
    int    timeout;      /**< 1 if timeout detected on this simulation */
    double last_message; /**< time of the last recieved message from this simulation */
    char   job_id[255];  /**< simulation job ID */
    int    job_status;   /**< simulation job status */
};

typedef struct melissa_simulation_s melissa_simulation_t; /**< type corresponding to melissa_simulation_s */

melissa_simulation_t* add_simulation();

void free_simu_vector(vector_t v);

int check_timeouts (vector_t *simulations,
                    int       timeout_simu);

void send_timeouts (int       detected_timeouts,
                    vector_t *simulations,
                    void     *python_pusher);

void process_txt_message (char      msg[255],
                          vector_t *simulations);

#endif // FAULT_TOLERANCE_H
