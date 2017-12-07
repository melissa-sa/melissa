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

struct melissa_simulation_s
{
//    int32_t *time_steps;
    int      status;
    int      timeout;
    double   last_message;
};

typedef struct melissa_simulation_s melissa_simulation_t;

melissa_simulation_t* add_simulation(int id, int nb_time_steps);

void simu_push_to(vector_t *v,
                  int       pos,
                  int       nb_time_steps);

void free_simu_vector(vector_t v);

//int check_timeouts (int *simu_state, int *simu_timeouts, double *last_message_simu, int nb_simu);
int check_timeouts (vector_t *simulations);

void send_timeouts (int       detected_timeouts,
                    vector_t *simulations,
                    char*     txt_buffer,
                    void     *python_requester);

#endif // FAULT_TOLERANCE_H
