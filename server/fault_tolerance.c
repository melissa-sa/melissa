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
 * @file fault_tolerance.c
 * @author Terraz Théophile
 * @date 2017-30-06
 *
 **/

#include <string.h>
#include "fault_tolerance.h"
#include "zmq.h"

melissa_simulation_t* add_simulation(int id, int nb_time_steps)
{
    melissa_simulation_t *simu;

    simu = melissa_malloc (sizeof(melissa_simulation_t));

    simu->id = id;
//    simu->field_status = NULL;
//    simu->time_steps = melissa_calloc((nb_time_steps+31)/32, sizeof(int32_t*));
    simu->timeout = 0;
    simu->last_message = 0;

    return simu;
}

void simu_push_to(vector_t *v,
             int       pos,
             int       nb_time_steps)
{
    int i;

    if (pos >= v->size)
    {
        while (v->capacity <= pos)
        {
            resize_vector(v, v->capacity * 2);
        }

        for (i=v->size; i<pos; i++)
        {
            v->items[i] = add_simulation(i, nb_time_steps);
        }
        v->size = pos + 1;
    }
//    (melissa_simulation_t*)v->items[pos] = add_simulation(pos, nb_time_steps);
    v->items[pos] = add_simulation(pos, nb_time_steps);
}

void free_simu_vector(vector_t v)
{
    int i;
    melissa_simulation_t *simu;
    for (i=0; i<v.size; i++)
    {
        simu = v.items[i];
//        melissa_free (simu->time_steps);
        melissa_free (v.items[i]);
    }
    melissa_free (v.items);
}

//int check_timeouts (vector_t *simulations)
int check_timeouts (int *simu_state, int *simu_timeouts, double *last_message_simu, int nb_simu)
{
    int detected_timeouts = 0;
    int i;
    double timeout_simu = 120; // TODO: something more user friendly
    double current_time = melissa_get_time();
    for (i=0; i<nb_simu; i++)
    {
        if (simu_state[i] == 1)
        {
            if (last_message_simu[i] + timeout_simu < current_time)
            {
                simu_timeouts[i] = 1;
                detected_timeouts += 1;
                fprintf (stdout, "timeout detected on simulation group %d\n", i);
                last_message_simu[i] = 0;
                simu_state[i] = 0;
            }
        }
    }
    return detected_timeouts;
}

void send_timeouts (int   detected_timeouts,
                    int  *simu_timeouts,
                    int   nb_simu,
                    char* txt_buffer,
                    void *python_pusher)
{
    int i;

    if (detected_timeouts < 1)
    {
        sprintf(txt_buffer, "timeout -1");
        zmq_send(python_pusher, txt_buffer, strlen(txt_buffer), 0);
    }
    else
    {
        for (i=0; i<nb_simu; i++)
        {
            if (simu_timeouts[i] == 1)
            {
                sprintf(txt_buffer, "timeout %d", i);
                zmq_send(python_pusher, txt_buffer, strlen(txt_buffer), 0);
                simu_timeouts[i] == 0;
            }
        }
    }
    return;
}
